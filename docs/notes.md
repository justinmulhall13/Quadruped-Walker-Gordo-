# Build Notes

## Servo Calibration
- Use a servo tester or sweep sketch to find min/max pulse widths for your DS3218 batch
- Neutral (90 deg) should be physically centered before mounting legs

## PCA9685 I2C Address
- Default is 0x40, verify with I2C scanner sketch if it does not respond

## Power (no buck converters)
- The buck converters were skipped. The 2S LiPo is spliced and soldered straight to the power
  port on the Freenove breakout board, which converts on-board.
- Servo rail (PCA9685 V+) is fed from the breakout's 5V pin. All 8 servos drive the legs fine
  off this rail.
- Dropping the bucks took two boards and a lot of wiring out of the chassis.
- Avoid stalling all 8 servos at once.
- The battery splice has no fuse or disconnect in it yet — worth adding.

## WiFi
- Robot creates its own access point
- Connect phone or laptop to the "QuadrupedTuner" network (password 12345678)
- Open browser to 192.168.4.1
