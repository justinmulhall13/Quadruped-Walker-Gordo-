# Wiring & Pinout

## Power
- Battery: 2S 7.4V LiPo, spliced and soldered direct to the Freenove breakout power port
- No buck converters — the breakout board's onboard regulator does the conversion
- Servo rail (PCA9685 V+) is fed from the breakout's 5V pin
- 2200uF electrolytic across PCA9685 V+ and GND

## I2C (ESP32-S3 to PCA9685)
| ESP32-S3 Pin | PCA9685 Pin |
|---|---|
| GPIO 8 (SDA) | SDA |
| GPIO 9 (SCL) | SCL |
| 3.3V | VCC |
| GND | GND |

## Servo Channels (PCA9685)
| Channel | Joint | Leg |
|---|---|---|
| 0 | Hip | Front Left |
| 1 | Knee | Front Left |
| 2 | Hip | Front Right |
| 3 | Knee | Front Right |
| 4 | Hip | Rear Left |
| 5 | Knee | Rear Left |
| 6 | Hip | Rear Right |
| 7 | Knee | Rear Right |

## Notes
- Servo power (V+) comes from the breakout's 5V pin, NOT from an ESP32 GPIO
- Common ground between ESP32, PCA9685, and servo rail
- No fuse or disconnect in the battery splice yet
