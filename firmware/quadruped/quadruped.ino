#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

// Channel mapping
#define FL_KNEE  0
#define FL_HIP   1
#define FR_KNEE  2
#define FR_HIP   3
#define BR_KNEE  4
#define BR_HIP   5
#define BL_KNEE  6
#define BL_HIP   7

#define NUM_SERVOS  8
#define NEUTRAL_US  1500
#define US_PER_DEG  7.407f
#define MOVE_MS     600

static int angleToUs(float deg) {
  return constrain(NEUTRAL_US + (int)(deg * US_PER_DEG), 500, 2500);
}

static void goToPose(const float angles[NUM_SERVOS], int ms = MOVE_MS) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    pca.writeMicroseconds(i, angleToUs(angles[i]));
  }
  delay(ms);
}

// ---------- poses & actions ----------

const float LAY_DOWN[NUM_SERVOS] = {
  [FL_KNEE] =  -21,
  [FL_HIP]  =   84,
  [FR_KNEE] =   -5,
  [FR_HIP]  =  -99,
  [BR_KNEE] =   26,
  [BR_HIP]  =  114,
  [BL_KNEE] =   -6,
  [BL_HIP]  =  -84,
};

const float STAND[NUM_SERVOS] = {
  [FL_KNEE] = -103,
  [FL_HIP]  =   33,
  [FR_KNEE] =   77,
  [FR_HIP]  =  -43,
  [BR_KNEE] =  101,
  [BR_HIP]  =   63,
  [BL_KNEE] =  -78,
  [BL_HIP]  =  -39,
};

static void doSpinLeft() {
  float p1a[NUM_SERVOS], p1b[NUM_SERVOS], p1c[NUM_SERVOS];
  float p2a[NUM_SERVOS], p2b[NUM_SERVOS], p2c[NUM_SERVOS];
  memcpy(p1a, STAND, sizeof(STAND)); memcpy(p1b, STAND, sizeof(STAND)); memcpy(p1c, STAND, sizeof(STAND));
  memcpy(p2a, STAND, sizeof(STAND)); memcpy(p2b, STAND, sizeof(STAND)); memcpy(p2c, STAND, sizeof(STAND));

  // Pair 1: BL + FR diagonal
  p1a[BL_KNEE] = -28;  p1a[FR_KNEE] =  36;
  p1b[BL_KNEE] = -28;  p1b[FR_KNEE] =  36;  p1b[BL_HIP] = 104; p1b[FR_HIP] = 135;
  p1c[BL_HIP]  = 104;  p1c[FR_HIP]  = 135;

  // Pair 2: FL + BR diagonal
  p2a[FL_KNEE] =  8;   p2a[BR_KNEE] = -20;
  p2b[FL_KNEE] =  8;   p2b[BR_KNEE] = -20;  p2b[FL_HIP] =  26; p2b[BR_HIP] =  25;
  p2c[FL_HIP]  = 26;   p2c[BR_HIP]  =  25;

  const int S = MOVE_MS;
  goToPose(p1a, S); goToPose(p1b, S); goToPose(p1c, S); goToPose(STAND, S);
  goToPose(p2a, S); goToPose(p2b, S); goToPose(p2c, S); goToPose(STAND, S);
}

static void doWave() {
  float wavePrep[NUM_SERVOS], waveA[NUM_SERVOS], waveB[NUM_SERVOS];
  memcpy(wavePrep, STAND, sizeof(STAND));
  memcpy(waveA,    STAND, sizeof(STAND));
  memcpy(waveB,    STAND, sizeof(STAND));

  wavePrep[BL_KNEE] = -47;           // shift counterbalance first
  waveA[BL_KNEE]    = -47;  waveA[FR_KNEE] = -69;  // BL shifted + FR up
  waveB[BL_KNEE]    = -47;  waveB[FR_KNEE] = -26;  // BL shifted + FR down

  const int STEP_MS  = 400;
  const int TOTAL_MS = 2500;

  goToPose(wavePrep, STEP_MS);       // BL knee shifts, settle before lifting

  unsigned long start = millis();
  bool high = true;
  while (millis() - start < TOTAL_MS) {
    goToPose(high ? waveA : waveB, STEP_MS);
    high = !high;
  }

  goToPose(wavePrep, STEP_MS);      // FR knee back to stand first, BL still shifted
  goToPose(STAND);                  // then BL knee returns
}

// ---------- setup / loop ----------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(8, 9);
  Wire.setClock(100000);
  pca.begin();
  pca.setPWMFreq(50);
  Serial.println("Ready");

  goToPose(STAND);
}

void loop() {
  // wave, dance, etc. will go here
}
