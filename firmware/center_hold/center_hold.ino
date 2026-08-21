#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

void setup() {
  Wire.begin(8, 9);
  Wire.setClock(100000);
  pca.begin();
  pca.setPWMFreq(50);

  // Hold all 8 servos at neutral (1500us)
  for (int ch = 0; ch < 8; ch++) {
    pca.writeMicroseconds(ch, 1500);
  }
}

void loop() {}
