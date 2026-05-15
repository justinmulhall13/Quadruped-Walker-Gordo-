#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Quadruped";
const char* password = "robot1234";

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);
WebServer server(80);

#define SERVO_FREQ 50
#define SERVO_MIN 150
#define SERVO_MAX 600

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  pca.begin();
  pca.setPWMFreq(SERVO_FREQ);

  WiFi.softAP(ssid, password);
  Serial.println("IP: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.begin();
}

void loop() {
  server.handleClient();
}

void setServo(uint8_t channel, int angle) {
  int pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pca.setPWM(channel, 0, pulse);
}

void handleRoot() {
  server.send(200, "text/html", "Controller placeholder");
}

void handleMove() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    Serial.println("Command: " + cmd);
  }
  server.send(200, "text/plain", "OK");
}
