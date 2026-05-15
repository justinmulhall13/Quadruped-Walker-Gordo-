# Quadruped Robot

A 4-legged walking robot controlled over WiFi via a browser-based interface. Built around an ESP32-S3 with custom gait logic and 8 servo legs.

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32-S3 Freenove + Breakout Board |
| Servo Driver | PCA9685 PWM Board (I2C) |
| Servos | 8x MG996R (2 per leg) |
| Battery | 2S 7.4V 2000mAh Li-ion (XT30) |
| Power Regulation | XL4015 Buck Converter |
| CAD | Onshape |

## Leg Configuration

4 legs, each with 2 DOF:
- Joint 1: Hip (lateral swing)
- Joint 2: Knee (vertical lift)

## Software

- ESP32-S3 programmed in C++ via Arduino IDE
- PCA9685 controlled over I2C
- WiFi-hosted web controller for directional input and preset moves
- Gait logic handles trot sequence coordination across all 4 legs

## Repo Structure

```
quadruped-robot/
├── firmware/
│   ├── main/               # Main Arduino sketch
│   ├── gait/               # Gait sequencing logic
│   └── web_controller/     # HTML/JS controller served over WiFi
├── mechanical/
│   └── onshape_link.md     # Link to Onshape CAD
├── hardware/
│   └── wiring_diagram.md   # Wiring notes and pinout
└── docs/
    └── notes.md            # Build notes
```

## Control Interface

The ESP32 hosts a simple web page over WiFi. Connect to the robot's network and open the IP in a browser to access directional controls and preset movement commands.

## Status

- [x] Mechanical design (Onshape)
- [ ] Wiring and power setup
- [ ] PCA9685 servo calibration
- [ ] Basic servo sweep test
- [ ] Gait implementation
- [ ] WiFi web controller
