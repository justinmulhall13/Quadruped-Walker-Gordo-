# Quadruped Robot

ESP32-S3 quadruped robot with 8 DS3218 servos controlled via PCA9685 PWM driver.

## Hardware

- **MCU**: Freenove ESP32-S3 WROOM on Freenove breakout board
- **FQBN**: `esp32:esp32:esp32s3`
- **Servo driver**: PCA9685 over I2C at address `0x40`
- **Servos**: 8x DS3218
  - Pulse range: 500µs – 2500µs
  - Operating voltage: 5V
  - Stall torque: 21 kg/cm

## Libraries

- Adafruit PWM Servo Driver Library
- Adafruit BusIO

## Commands

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32s3 quadruped

# Upload
arduino-cli upload -p COM4 --fqbn esp32:esp32:esp32s3 quadruped

# Serial monitor (115200 baud)
arduino-cli monitor -p COM4 --config baudrate=115200
```

## Servo Notes

The DS3218 uses a wider pulse range than the standard 1000–2000µs servo — always configure the PCA9685 with `setServoPulse` or equivalent using the 500–2500µs range to avoid mechanical binding at the extremes.
