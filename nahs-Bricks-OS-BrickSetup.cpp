#include <nahs-Bricks-OS-BrickSetup.h>
#include <nahs-Bricks-OS.h>
#include <nahs-Bricks-Lib-SerHelp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

const char *ap_ssid = "BrickSetup";
const char *ap_psk = "helloworld!";
IPAddress ap_local_IP(192, 168, 123, 1);
IPAddress ap_gateway(192, 168, 123, 254);
IPAddress ap_subnet(255, 255, 255, 0);
ESP8266WebServer setupServer(80);

NahsBricksOSBrickSetup::NahsBricksOSBrickSetup() {
}

//------------------------------------------
// handover
void NahsBricksOSBrickSetup::handover() {
    Serial.begin(115200);
    Serial.println();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    Serial.print("Starting webSetup Server: ");
    Serial.println(WiFi.softAP(ap_ssid, ap_psk) ? "OK": "Failed");
    delay(100);
    Serial.print("Configuring webSetup Server: ");
    Serial.println(WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet)? "OK": "Failed");
    
    setupServer.on("/", std::bind(&NahsBricksOSBrickSetup::webSetupHandler, this));
    setupServer.onNotFound(std::bind(&NahsBricksOSBrickSetup::webSetupHandlerNotFound, this));
    setupServer.begin();

    Serial.println("\n=== to enter webSetup ===");
    Serial.print("Connect to SSID: ");
    Serial.println(ap_ssid);
    Serial.print("with password: ");
    Serial.println(ap_psk);
    Serial.print("and navigate to: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("\n=== or continue via Serial ===");
    Serial.println("hit <enter>");

    while (true) {
        Serial.println();
        Serial.print("Select: ");
        String input_str = "";
        while (true) {
          setupServer.handleClient();
          input_str = SerHelp.readLine(false);
          if (input_str != String('\n')) break;
        }
        uint8_t input = input_str.toInt();
        bool invalidInput = false;

        if (input >= _featureSubmenuOffset) {
          invalidInput = !FeatureAll.handoverBrickSetupToFeature(input - _featureSubmenuOffset);
        }
        else {
          switch(input) {
            case 1:
              showBrickInfo();
              break;
            case 2:
              showConfig();
              break;
            case 3:
              showRuntimeData();
              break;
            case 4:
              setIdent();
              break;
            case 5:
              configWifi();
              break;
            case 6:
              testWifi();
              break;
            case 7:
              configBricksServer();
              break;
            case 8:
              testBricksServer();
              break;
            case 9:
              saveConfig();
              break;
            default:
              invalidInput = true;
          }
        }

        if (invalidInput) {
          Serial.println("Invalid Input!");
          showMenu();
        }
    }
    
}

//------------------------------------------
// showMenu
void NahsBricksOSBrickSetup::showMenu() {
    Serial.println(" 1) Show Brick Info");
    Serial.println(" 2) Show Config");
    Serial.println(" 3) Show RuntimeData");
    Serial.println(" 4) Set Identity");
    Serial.println(" 5) Configure WiFi");
    Serial.println(" 6) Test WiFi");
    Serial.println(" 7) Configure Bricks-Server");
    Serial.println(" 8) Connect to Bricks-Server");
    Serial.println(" 9) Save Config");
    Serial.println("Feature Submenus:");
    FeatureAll.printBrickSetupFeatureMenu(_featureSubmenuOffset);
}

void NahsBricksOSBrickSetup::showBrickInfo() {
  Serial.println();
  Serial.print("BrickID: ");
  Serial.println(WiFi.macAddress());
  Serial.print("BrickType: ");
  Serial.println(FeatureAll.getBrickType());
  Serial.print("RTCmem used: ");
  Serial.print(RTCmem.getSpaceUsed());
  Serial.print("/");
  Serial.println(RTCmem.getSpaceTotal());
  Serial.print("SketchMD5: ");
  Serial.println(ESP.getSketchMD5());
  Serial.println("Features:");
  FeatureAll.printBrickSetupFeatureList();
  Serial.println("Versions:");
  FeatureAll.printBrickSetupVersionList();
  Serial.println();
  Serial.println("NAHS-Bricks-OS created by NiJO's");
  Serial.println("For more information visit my website: nijos.de");
  Serial.print("Copyright (");
  Serial.print(BricksOS.getCopyrightYear());
  Serial.println(") NiJO's");
}

void NahsBricksOSBrickSetup::showConfig() {
  Serial.println();
  BricksOS.printFSdata();
  FeatureAll.printFSdata();
}

void NahsBricksOSBrickSetup::showRuntimeData() {
  Serial.println();
  BricksOS.printRTCdata();
  FeatureAll.printRTCdata();
}

void NahsBricksOSBrickSetup::setIdent() {
  Serial.println();
  Serial.println("Ident: ");
  BricksOS.setIdent(SerHelp.readLine());
}

void NahsBricksOSBrickSetup::configWifi() {
  Serial.println();
  Serial.print("SSID: ");
  BricksOS.setWifiSSID(SerHelp.readLine());
  Serial.print("Password: ");
  BricksOS.setWifiPass(SerHelp.readLine());
}

void NahsBricksOSBrickSetup::testWifi() {
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi allready connected. Stopping it");
    WiFi.mode(WIFI_OFF);
    while (WiFi.status() == WL_CONNECTED) {
      Serial.print('.');
      delay(200);
    }
    Serial.println('.');
  }
  Serial.print("Connecting WiFi");
  BricksOS.connectWifi();
 
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
    if (i++ >= 20) break;
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println(" Success");
  else {
    Serial.println(" Failed");
    WiFi.mode(WIFI_OFF);
  }
}

void NahsBricksOSBrickSetup::configBricksServer() {
  Serial.println();
  Serial.print("Server: ");
  String host = SerHelp.readLine();
  Serial.print("Port: ");
  long port = SerHelp.readLine().toInt();
  BricksOS.setBrickServerURL(host, port);
}

void NahsBricksOSBrickSetup::testBricksServer() {
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Setup WiFi first!");
    return;
  }
  Serial.print("Sending Test Data... ");
  DynamicJsonDocument out_json(1024);
  out_json["test"] = "val";
  DynamicJsonDocument in_json = BricksOS.transmitToBrickServer(out_json);

  if(in_json.containsKey("s")) {
    if(in_json["s"] == 0) Serial.println("Success");
    else Serial.println("Failed");
  } else Serial.println("Failed");
}

void NahsBricksOSBrickSetup::saveConfig() {
  if (FSmem.write()) Serial.println("Config saved!");
  else Serial.println("Errors on saving config...");
}

const char brickSetup_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>BrickSetup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>BrickSetup</h1>
  <p>Configure Bricks-Server connection</p>
  <form action="/" method="post">
    SSID: <input type="text" name="ssid"><br />
    PSK: <input type="password" name="psk"><br />
    Server: <input type="text" name="server"><br />
    Port: <input type="text" name="port" value="8081"><br />
    Ident: <input type="text" name="ident"><br />
    <input type="submit" value="SAVE">
  </form>
</body></html>)rawliteral";

const char brickSetup_saved_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>BrickSetup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>BrickSetup</h1>
  <p>Configuration saved!<br />Reset your Brick in boot-mode now.</p>
</body></html>)rawliteral";

void NahsBricksOSBrickSetup::webSetupHandler() {
  if (setupServer.method() == HTTP_POST) {
    BricksOS.setWifiSSID(setupServer.arg("ssid"));
    BricksOS.setWifiPass(setupServer.arg("psk"));
    BricksOS.setBrickServerURL(setupServer.arg("server"), setupServer.arg("port").toInt());
    BricksOS.setIdent(setupServer.arg("ident"));
    FSmem.write();
    RTCmem.destroy();
    setupServer.send(200, "text/html", brickSetup_saved_html);
  }
  else {
    setupServer.send(200, "text/html", brickSetup_index_html);
  }
}

/*
helper to handle webSetup requests to wrong path/url
*/
void NahsBricksOSBrickSetup::webSetupHandlerNotFound() {
    setupServer.sendHeader("Location", String("/"), true);
    setupServer.send(302, "text/plain", "");
}


#if !defined(NO_GLOBAL_INSTANCES)
NahsBricksOSBrickSetup BrickSetup;
#endif
