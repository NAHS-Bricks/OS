#ifndef NAHS_BRICKS_OS_H
#define NAHS_BRICKS_OS_H

#include <Arduino.h>
#include <nahs-Bricks-Lib-RTCmem.h>
#include <nahs-Bricks-Lib-FSmem.h>
#include <nahs-Bricks-Feature-All.h>

class NahsBricksOS {
    private:
        static const uint8_t version = 2;
        static const uint16_t copyrightYear = 2022;
        typedef struct {
            uint8_t channel;  // WiFi-Channel to be used
            uint8_t ap_mac[6];  // MAC-Address of AP to be used
            bool sketchMD5Requested;
            bool otaUpdateRequested;
        } _RTCdata;
        _RTCdata* RTCdata = RTCmem.registerData<_RTCdata>();
        JsonObject FSdata = FSmem.registerData("os");
        uint8_t _setupPin;
        bool _activatorEventReceived;
        bool _writeFSmemRequested;
        int configResetRequestsCount = 0;
    public:
        NahsBricksOS();
        void setSetupPin(uint8_t pin);
        void handover();
        void connectWifi();
        void waitWifi();
        DynamicJsonDocument transmitToBrickServer(DynamicJsonDocument out_json);
    public:  //used by BrickSetup
        void printRTCdata();
        void printFSdata();
        uint16_t getCopyrightYear();
        void setWifiSSID(String ssid);
        void setWifiPass(String pass);
        void setBrickServerURL(String host, long port);
        void requestFSmemWrite();
        void handleConfigResetRequest();
    private:
        void begin();
        void handleActivator();
        void handleActivatorNotFound();
        void handleOtaUpdate();
};

#if !defined(NO_GLOBAL_INSTANCES)
extern NahsBricksOS BricksOS;
#endif

#endif // NAHS_BRICKS_OS_H
