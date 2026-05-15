#include "gait.h"
#include <Adafruit_PWMServoDriver.h>

extern Adafruit_PWMServoDriver pca;

#define SERVO_MIN 150
#define SERVO_MAX 600

void setServo(uint8_t channel, int angle) {
  int pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pca.setPWM(channel, 0, pulse);
}

void standStill() {
  for (int i = 0; i < 8; i++) setServo(i, 90);
}

void sitDown() {
  // TODO
}

void walkForward() {
  // TODO: trot gait - FL+RR move together, FR+RL move together
}

void walkBackward() {
  // TODO
}

void turnLeft() {
  // TODO
}

void turnRight() {
  // TODO
}
