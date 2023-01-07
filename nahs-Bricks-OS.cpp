#include <nahs-Bricks-OS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <nahs-Bricks-OS-BrickSetup.h>
#include <nahs-Bricks-Lib-SerHelp.h>

ESP8266WebServer server(80);

/*
ISR that listens to falling-edges during BrickSetup
*/
ICACHE_RAM_ATTR void configResetISR() {
    noInterrupts();
    BricksOS.handleConfigResetRequest();
    interrupts();
}

NahsBricksOS::NahsBricksOS() {
    _writeFSmemRequested = false;
}

/*
Sets Brick-Specific pin used to detect setup-mode
*/
void NahsBricksOS::setSetupPin(uint8_t pin) {
    _setupPin = pin;
}

/*
used to handover the main-process to BrickOS, this function is never returning
*/
void NahsBricksOS::handover() {
    //------------------------------------------
    // initialize variables on all features
    begin();

    //------------------------------------------
    // execute otaUpdate if requested
    if (RTCdata->otaUpdateRequested) handleOtaUpdate();

    //------------------------------------------
    // Telling feature-all which version of BrickOS is running
    FeatureAll.setOsVersion(version);

    //------------------------------------------
    // check if BrickSetup needs to be entered
    pinMode(_setupPin, INPUT_PULLUP);
    delay(1);
    if (digitalRead(_setupPin) == LOW || FSdata["ssid"] == "" || FSdata["url"] == "") {
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);
        attachInterrupt(digitalPinToInterrupt(_setupPin), configResetISR, FALLING);
        BrickSetup.handover();
    }

    //------------------------------------------
    // start all backgroud processes
    connectWifi();
    FeatureAll.start();

    //------------------------------------------
    // prepare json document to be transmitted to BrickServer
    DynamicJsonDocument out_json(2048);
    FeatureAll.deliver(&out_json);

    //------------------------------------------
    // deliver ident if it is set and Brick just initalized
    if (!RTCmem.isValid() && FSdata["id"] != "") {
        out_json.getOrAddMember("id").set(FSdata["id"]);
    }

    //------------------------------------------
    // deliver sketchMD5 if requested
    if (RTCdata->sketchMD5Requested) {
        RTCdata->sketchMD5Requested = false;
        out_json.getOrAddMember("m").set(ESP.getSketchMD5());
    }

    //------------------------------------------
    // wait for wifi
    waitWifi();

    //------------------------------------------
    // submit data
    DynamicJsonDocument in_json = transmitToBrickServer(out_json);

    //------------------------------------------
    // process feedback from BrickServer
    FeatureAll.feedback(&in_json);

    //------------------------------------------
    // evaluate own requests
    if (in_json.containsKey("r")) {
        for (JsonVariant value : in_json.getMember("r").as<JsonArray>()) {
            switch(value.as<uint8_t>()) {
                case 11:
                    RTCdata->sketchMD5Requested = true;
                    break;
                case 12:
                    RTCdata->otaUpdateRequested = true;
                    break;
            }
        }
    }

    //------------------------------------------
    // write RTCmem
    RTCmem.write();

    //------------------------------------------
    // write FSmem
    if (_writeFSmemRequested) {
        _writeFSmemRequested = false;
        FSmem.write();
    }

    //------------------------------------------
    // end all features
    FeatureAll.end();

    //------------------------------------------
    // start up the Activator
    server.on("/", std::bind(&NahsBricksOS::handleActivator, this));
    server.onNotFound(std::bind(&NahsBricksOS::handleActivatorNotFound, this));
    server.begin();

    //------------------------------------------
    // now wait if any Activator events come in
    _activatorEventReceived = false;
    for (uint8_t i = 0; i < FeatureAll.getDelay(); ++i) {
        server.handleClient();
        if (_activatorEventReceived) {
            while (server.client().connected()) yield();
            break;
        }
        delay(1000);
    }

    //------------------------------------------
    // write RTCmem again just in case an activator changed some content
    RTCmem.write();

    //------------------------------------------
    // write FSmem again just in case an activator changed some content
    if (_writeFSmemRequested) {
        FSmem.write();
    }

    //------------------------------------------
    // lets self-reset to start over again
    ESP.restart();
}

/*
helper to start the WiFi Connection
*/
void NahsBricksOS::connectWifi() {
    WiFi.forceSleepWake();  // power up wifi module
    delay(1);
    WiFi.persistent(false);  // disable wifi persistence (this will not automatically store and load wifi connection from flash)
    WiFi.mode(WIFI_STA);  // set station mode
    if(RTCmem.isValid()) {
        // Try connecting to previous used AP
        WiFi.begin(FSdata["ssid"].as<const char*>(), FSdata["pass"].as<const char*>(), RTCdata->channel, RTCdata->ap_mac, true);
    }
    else {
        // Connect with WiFi-Discover
        WiFi.begin(FSdata["ssid"].as<const char*>(), FSdata["pass"].as<const char*>());
    }
}

/*
helper that waits for WiFi Connection (and saves connection info for later runs)
*/
void NahsBricksOS::waitWifi() {
    if(RTCmem.isValid()) { // WiFi quick connect is tried
        uint8_t retries = 0;
        while (WiFi.status() != WL_CONNECTED) {
            ++retries;
            if (retries > 200) {
                // Quick connect is not working, reset and try normal connect
                WiFi.disconnect();
                delay(10);
                WiFi.forceSleepBegin();
                delay(10);
                WiFi.forceSleepWake();
                delay(10);
                WiFi.begin(FSdata["ssid"].as<const char*>(), FSdata["pass"].as<const char*>());
                break;
            }
            delay(10);
        }
    }
    while (WiFi.status() != WL_CONNECTED) delay(10);

    // save AP info for later use
    RTCdata->channel = WiFi.channel();
    memcpy(RTCdata->ap_mac, WiFi.BSSID(), 6);
}

/*
helper to transmit a json_document to BrickServer and receive the answer also as json_document
*/
DynamicJsonDocument NahsBricksOS::transmitToBrickServer(DynamicJsonDocument out_json) {
    String httpPayload;
    httpPayload.reserve(2048);  // Prevent RAM fragmentation
    if (out_json.isNull()) httpPayload = "{}";
    else serializeJson(out_json, httpPayload);
    WiFiClient client;
    HTTPClient http;
    http.begin(client, FSdata["url"].as<String>());
    http.addHeader("Content-Type", "application/json");
    http.POST(httpPayload);
    DynamicJsonDocument in_json(1024);
    deserializeJson(in_json, http.getStream());
    http.end();
    return in_json;
}

/*
prints it's own RuntimeData to Serial
*/
void NahsBricksOS::printRTCdata() {
    Serial.println("os:");
    Serial.print("  sketchMD5Requested: ");
    SerHelp.printlnBool(RTCdata->sketchMD5Requested);
    Serial.print("  otaUpdateRequested: ");
    SerHelp.printlnBool(RTCdata->otaUpdateRequested);
    Serial.println();
}

/*
prints it's own Config to Serial
*/
void NahsBricksOS::printFSdata() {
    Serial.println("os:");
    Serial.print("  WiFi-SSID: ");
    Serial.println(FSdata["ssid"].as<String>());
    Serial.print("  WiFi-Pass: ");
    Serial.println(FSdata["pass"].as<String>());
    Serial.print("  BrickServer-URL: ");
    Serial.println(FSdata["url"].as<String>());
    Serial.print("  Ident: ");
    Serial.println(FSdata["id"].as<String>());
    Serial.println();
}

/*
helper to return copyright year
*/
uint16_t NahsBricksOS::getCopyrightYear() {
    return copyrightYear;
}

/*
helper to set SSID
*/
void NahsBricksOS::setWifiSSID(String ssid) {
    FSdata["ssid"] = ssid;
}

/*
helper to set WiFi Password
*/
void NahsBricksOS::setWifiPass(String pass) {
    FSdata["pass"] = pass;
}

/*
helper to set BrickServer's URL
*/
void NahsBricksOS::setBrickServerURL(String host, long port) {
    FSdata["url"] = "http://" + host + ":" + String(port);
}

/*
helper to set Identity-String of Brick
*/
void NahsBricksOS::setIdent(String ident) {
    FSdata["id"] = ident;
}

/*
helper for Features to request a FSmem write
*/
void NahsBricksOS::requestFSmemWrite() {
    _writeFSmemRequested = true;
}

/*
helper that destroys Brick's configuration if called for four times
*/
void NahsBricksOS::handleConfigResetRequest() {
    configResetRequestsCount++;
    //------------------------------------------
    // debounce the input signal
    unsigned long ms = millis() + 400; while (ms > millis()) {}

    //------------------------------------------
    // blink the onboard led twice to show the reset request counter incremented
    for (uint8_t i = 0; i < 2; ++i) {
        ms = millis() + 50; while (ms > millis()) {}
        digitalWrite(LED_BUILTIN, LOW);
        ms = millis() + 50; while (ms > millis()) {}
        digitalWrite(LED_BUILTIN, HIGH);
    }

    //------------------------------------------
    // if reset request counter is 4 or higher light up long to show the config is now destroyed and the brick is going to restart
    if (configResetRequestsCount >= 4) {
        FSmem.destroy();
        FSmem.write();
        RTCmem.destroy();
        ms = millis() + 50; while (ms > millis()) {}
        digitalWrite(LED_BUILTIN, LOW);
        ms = millis() + 1500; while (ms > millis()) {}
        digitalWrite(LED_BUILTIN, HIGH);
        ESP.restart();
    }
}

/*
helper that initializes all variables of features
*/
void NahsBricksOS::begin() {
    if (!FSdata.containsKey("ssid")) FSdata["ssid"] = "";
    if (!FSdata.containsKey("pass")) FSdata["pass"] = "";
    if (!FSdata.containsKey("url")) FSdata["url"] = "";
    if (!FSdata.containsKey("id")) FSdata["id"] = "";
    if (!RTCmem.isValid()) {
        RTCdata->sketchMD5Requested = false;
        RTCdata->otaUpdateRequested = false;
    }
    FeatureAll.begin();
}

/*
helper that allows async data receiving from BrickServer
*/
void NahsBricksOS::handleActivator() {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/json", "{\"s\": 2, \"m\": \"wrong method\"}");
    }
    else {
        DynamicJsonDocument in_json(1024);
        deserializeJson(in_json, server.arg("plain"));
        FeatureAll.feedback(&in_json);
        server.send(200, "text/json", "{\"s\": 0}");
        _activatorEventReceived = true;
    }
}

/*
helper to handle Activator requests to wrong path/url
*/
void NahsBricksOS::handleActivatorNotFound() {
    server.send(404, "text/json", "{\"s\": 1, \"m\": \"wrong url\"}");
}


/*
helper to handle firmware otaUpdate
*/
void NahsBricksOS::handleOtaUpdate() {
    RTCdata->otaUpdateRequested = false;

    //------------------------------------------
    // connect and wait for WiFi in one go, as nothing can be done in between
    connectWifi();
    waitWifi();

    //------------------------------------------
    // invalidating all RTC data to be sure the next boot is initializing all variables from scratch
    RTCmem.destroy();

    //------------------------------------------
    // executing the OTA Update
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    WiFiClient client;
    ESPhttpUpdate.update(client, FSdata["url"].as<String>() + "/ota");

    //------------------------------------------
    // rebooting the ESP
    ESP.restart();
}


//------------------------------------------
// globally predefined variable
#if !defined(NO_GLOBAL_INSTANCES)
NahsBricksOS BricksOS;
#endif
