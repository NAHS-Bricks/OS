#include <Arduino.h>
#include <nahs-Bricks-OS.h>
// include all features of brick

void setup() {
  // Now register all the features under All
  // Note: the order of registration is the same as the features are handled internally by All

  // Set Brick-Specific stuff
  BricksOS.setSetupPin(D7);
  FeatureAll.setBrickType(5);

  // Set Brick-Specific (feature related) stuff

  // Finally hand over to BrickOS
  BricksOS.handover();
}

void loop() {
  // Not used on Bricks
}