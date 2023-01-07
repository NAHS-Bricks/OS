#ifndef NAHS_BRICKS_OS_BRICKSETUP_H
#define NAHS_BRICKS_OS_BRICKSETUP_H

#include <Arduino.h>

class NahsBricksOSBrickSetup {
    private:
        static const uint8_t _featureSubmenuOffset = 10;
    public:
        NahsBricksOSBrickSetup();
        void handover();
    private:
        void showMenu();
        void showBrickInfo();
        void showConfig();
        void showRuntimeData();
        void setIdent();
        void configWifi();
        void testWifi();
        void configBricksServer();
        void testBricksServer();
        void saveConfig();
        void webSetupHandler();
        void webSetupHandlerNotFound();
};

#if !defined(NO_GLOBAL_INSTANCES)
extern NahsBricksOSBrickSetup BrickSetup;
#endif

#endif // NAHS_BRICKS_OS_BRICKSETUP_H
