# Build Notes

## Servo Calibration
- Use a servo tester or sweep sketch to find min/max pulse widths for your DS3218 batch
- Neutral (90 deg) should be physically centered before mounting legs

## PCA9685 I2C Address
- Default is 0x40, verify with I2C scanner sketch if it does not respond

## Power (no buck converters)
- The buck converters were skipped. The 2S LiPo is spliced and soldered straight to the power
  port on the Freenove breakout board, which converts on-board.
- Servo rail (PCA9685 V+) is fed from the breakout's 5V pin.
- DS3218 stall current is ~1.4-2A each, 8 servos = up to ~11-16A peak. The breakout's onboard
  regulator is nowhere near this, so the servos are current-limited under load — the likely
  cause of the weak gait in the walking clip.
- DS3218 makes its rated 20kg at 6.8V; at 5V it is meaningfully weaker again.
- Fix to try: dedicated BEC or buck for the servo rail sized for stall current, ESP32 on its
  own feed, grounds common. Add an inline fuse and a disconnect to the battery splice.

## WiFi
- Robot creates its own access point
- Connect phone or laptop to "Quadruped" network
- Open browser to 192.168.4.1
