# Gordo — 4-Legged Walking Quadruped Robot

A personal summer project: a fully custom quadruped walking robot with WiFi-based web controller, trot gait, and 2-DOF legs.

![Gordo assembled](images/build_photo.png)

---

## Current Status

- [x] CAD complete (Onshape)
- [x] All parts received
- [x] Hardware fully assembled
- [x] All 8 servos calibrated and tested
- [x] Wiring schematic complete (KiCad)
- [x] WiFi web controller built
- [ ] Power supply wiring (parts incoming)
- [ ] Firmware — PCA9685 init and servo sweep
- [ ] Firmware — inverse kinematics
- [ ] Firmware — trot gait engine
- [ ] Firmware — web controller integration

---

## Hardware

| Component | Part | Notes |
|---|---|---|
| Microcontroller | ESP32-S3 Freenove + Breakout Board | WiFi AP hosted on-board |
| Servo Driver | PCA9685 PWM board | I2C address 0x40 |
| Servos | 8x DS3218 20kg | 2 DOF per leg, hip + knee |
| Battery | DLG 7.4V 2200mAh 2S 30C LiPo | Dean's T connector |
| Buck Converter (x2) | XINGYHENG 20A 300W | 7.4V in — one to 3.3V logic, one to 5V servo |
| Capacitor | 2200uF 10V electrolytic | Across PCA9685 V+ and GND |
| Connectors | Elechawk Dean's T pigtails 14AWG | Battery to buck converter inputs |
| Charger | B3 2S balance charger | Charges via JST-XH balance port |
| CAD | Onshape | Fully modelled and finalized |

---

## Leg Configuration

4 legs, 2 DOF each (hip + knee). All servos driven by PCA9685 over I2C.

| Channel | Leg | Joint |
|---|---|---|
| 0 | Front Left | Hip |
| 1 | Front Left | Knee |
| 2 | Front Right | Hip |
| 3 | Front Right | Knee |
| 4 | Rear Left | Hip |
| 5 | Rear Left | Knee |
| 6 | Rear Right | Hip |
| 7 | Rear Right | Knee |

---

## Wiring

I2C: GPIO8 (SDA), GPIO9 (SCL) on ESP32-S3 to PCA9685

Power architecture:
- 2S LiPo → Buck 1 (3.3V) → ESP32 VIN
- 2S LiPo → Buck 2 (5V) → PCA9685 V+ (servo rail)
- All grounds common
- 2200uF capacitor across PCA9685 V+ and GND

Full schematic: [hardware/Gordo-Wiring.pdf](hardware/Gordo-Wiring.pdf)

---

## Software Stack

- C++ via Arduino IDE
- PCA9685 via Adafruit PWM Servo Driver library
- WiFi access point: SSID Quadruped, password robot1234
- Web controller served at 192.168.4.1
- Planned gait: trot (FL+RR move together, FR+RL move together)

### File Structure

\`\`\`
firmware/
├── main.ino        — setup, loop, WiFi AP, web server routes
├── gait.h          — gait function declarations
└── gait.cpp        — trot sequencer, stance/swing logic
hardware/
└── Gordo-Wiring.pdf — KiCad wiring schematic
images/
└── build_photo.png  — assembled robot photo
\`\`\`

---

## Web Controller

Browser-based controller served directly from the ESP32. Connect to the Quadruped WiFi network and open 192.168.4.1 in any browser.

Controls:
- D-pad: forward, back, left, right
- Rotate: CW and CCW
- Speed: slow, medium, fast
- Presets: stand, sit, wave, dance

---

## Notes

- DS3218 stall current ~2A each — 8 servos = up to 16A peak. Do not stall all simultaneously.
- Set buck converter output voltages before connecting anything.
- Servo neutral (90 deg) physically centred before mounting legs.
- PCA9685 default I2C address 0x40 — verify with I2C scanner if unresponsive.
- Never charge LiPo unattended. Use balance charger only. Storage voltage: 3.8V/cell.
- Battery cutoff: stop use at 7.0V total (3.5V/cell).
