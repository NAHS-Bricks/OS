#include <nahs-Bricks-OS-BrickSetup.h>
#include <nahs-Bricks-OS.h>
#include <nahs-Bricks-Lib-SerHelp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

NahsBricksOSBrickSetup::NahsBricksOSBrickSetup() {
}

//------------------------------------------
// handover
void NahsBricksOSBrickSetup::handover() {
    Serial.begin(115200);
    while (true) {
        Serial.println();
        Serial.print("Select: ");
        uint8_t input = SerHelp.readLine().toInt();
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
              configWifi();
              break;
            case 5:
              testWifi();
              break;
            case 6:
              configBricksServer();
              break;
            case 7:
              testBricksServer();
              break;
            case 8:
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
    Serial.println(" 4) Configure WiFi");
    Serial.println(" 5) Test WiFi");
    Serial.println(" 6) Configure Bricks-Server");
    Serial.println(" 7) Connect to Bricks-Server");
    Serial.println(" 8) Save Config");
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


#if !defined(NO_GLOBAL_INSTANCES)
NahsBricksOSBrickSetup BrickSetup;
#endif
