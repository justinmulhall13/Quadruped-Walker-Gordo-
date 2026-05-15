# Wiring & Pinout

## Power
- Battery: 2S 7.4V Li-ion, XT30 connector
- Buck converter (XL4015) steps down to 5V for PCA9685 and servos
- ESP32-S3 powered via 3.3V onboard regulator from breakout board

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
- Servo power (V+) comes directly from buck converter output, NOT from ESP32
- Common ground between ESP32, PCA9685, and servo rail
