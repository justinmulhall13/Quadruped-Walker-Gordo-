# Final Wiring & Pinout

## Power
- Battery: 2S 7.4V LiPo, spliced and soldered direct to the Freenove breakout power port
- The breakout board's onboard regulator provides the required conversion
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
| 0 | Knee | Front Left |
| 1 | Hip | Front Left |
| 2 | Knee | Front Right |
| 3 | Hip | Front Right |
| 4 | Knee | Back Right |
| 5 | Hip | Back Right |
| 6 | Knee | Back Left |
| 7 | Hip | Back Left |

## Notes
- Servo power (V+) comes from the breakout's 5V pin, NOT from an ESP32 GPIO
- Common ground between ESP32, PCA9685, and servo rail
