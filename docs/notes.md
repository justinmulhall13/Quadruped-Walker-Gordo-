# Build Notes

## Servo Calibration
- Use a servo tester or sweep sketch to find min/max pulse widths for your DS3218 batch
- Neutral (90 deg) should be physically centered before mounting legs

## PCA9685 I2C Address
- Default is 0x40, verify with I2C scanner sketch if it does not respond

## Buck Converter
- Set output to 5V before connecting anything
- DS3218 stall current is ~1.4A each, 8 servos = up to ~11A peak
- XL4015 is rated 5A max so avoid stalling all servos simultaneously

## WiFi
- Robot creates its own access point
- Connect phone or laptop to "Quadruped" network
- Open browser to 192.168.4.1
