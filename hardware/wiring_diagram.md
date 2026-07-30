# Wiring & Pinout

## Power
- Battery: 2S 7.4V LiPo, spliced and soldered direct to the Freenove breakout power port
- No buck converters — the breakout board's onboard regulator does the conversion
- Servo rail (PCA9685 V+) is fed from the breakout's 5V pin
- 2200uF electrolytic across PCA9685 V+ and GND

> The servo rail is undersized: all 8 DS3218 draw through the breakout's onboard regulator,
> which is a logic supply, not a servo bus. This limits torque and is the leading suspect for
> the weak walking gait. Planned fix is a dedicated BEC or buck for the servo rail, rated for
> DS3218 stall current, with the ESP32 on its own feed and grounds common.

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
- Servo power (V+) currently comes from the breakout's 5V pin — this is the thing to change
- Common ground between ESP32, PCA9685, and servo rail
- No fuse or disconnect in the battery splice yet
