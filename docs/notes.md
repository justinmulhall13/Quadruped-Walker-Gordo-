# Final Build & Operating Notes

## Servo Calibration
- The eight DS3218 servos are centered at neutral before the legs are mounted.
- The operating pulse range is calibrated to the installed servo batch and mechanical limits.

## PCA9685 I2C Address
- The servo driver uses the default `0x40` address.
- I2C uses GPIO 8 (SDA) and GPIO 9 (SCL).

## Power Architecture
- The 2S LiPo is spliced and soldered directly to the Freenove breakout power port.
- The breakout board performs the onboard voltage conversion.
- The breakout's 5 V pin feeds the PCA9685 servo rail.
- A 2200 µF capacitor is installed across PCA9685 V+ and ground.
- All eight servos operate from this rail; avoid driving them against mechanical stops.

## WiFi
- Gordo creates the `QuadrupedTuner` access point.
- Connect a phone or laptop with the password `12345678`.
- Open `http://192.168.4.1` in a browser.

## Battery Care
- Charge only through the JST-XH balance connector with the B3 balance charger.
- Never charge unattended.
- Stop use at 7.0 V total pack voltage and store at 3.8 V per cell.
