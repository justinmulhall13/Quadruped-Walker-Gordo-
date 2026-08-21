# Gordo — 4-Legged Walking Quadruped Robot

A personal summer project and my first solo robot build: a fully custom quadruped with 2-DOF legs, a WiFi web controller, and a crawl gait tuned live from a browser. Frame, wiring, firmware, and gait are all my own — this did not start from a kit or an existing gait library.

![Gordo assembled](images/build_photo.jpg)

---

## It Walks

![Gordo walking](media/gordo_walk.gif)

Full clip: [media/gordo_walk.mp4](media/gordo_walk.mp4)

Walking under its own power on carpet, driven over WiFi. Each step pulls on the front feet and pushes off the back ones, and the body actually travels instead of shuffling in place.

The first walking prototype is still here for comparison: [media/gordo_walk_prototype.mp4](media/gordo_walk_prototype.mp4). That version scuffed through every stroke and scrubbed away most of its own travel. The difference between the two clips is not tuning — it is the stance geometry described below.

---

## What I Learned

This is the part of the project I actually care about. Most of it came from watching Gordo fail to walk and working out why.

### A joint can only draw the shape it is shaped to draw

The hips are yaw joints, so a planted foot can only be dragged around an **arc** centred on its own hip. Spinning in place is therefore easy — when the robot turns, that arc *is* the path each foot should follow, so nothing fights and the push is clean. Walking in a straight line asks each planted foot to trace a **straight line** instead, which a single yaw joint cannot do. With three feet down, that is three incompatible arcs, and the foot with the least grip gives up and skids.

I spent a long time treating this as a tuning problem, and it never improved, because it was never a tuning problem. The skid scales with roughly the square of stride length, so halving the stride cut it by about four — but the real fix is to give the foot a second component of motion. The knee changes a foot's *reach*, so sweeping the knee in proportion to the hip lets the foot travel something much closer to a straight line: front feet pull in as the body drives past them, back feet reach out. That ratio — knee degrees per hip degree — is the single change that turned the shuffle into a walk.

### Two DOF per leg means you cannot shift weight

This is the honest limitation of the build, and it is the first thing I would change on a second one.

A walking robot is stable while its centre of mass sits inside the triangle formed by the three feet still on the ground. Animals and good quadruped robots manage this by *shifting their weight* — leaning onto the supporting legs before picking a foot up. That requires a joint that can move the body sideways over the legs, usually a third degree of freedom at the hip.

Gordo has no such joint. With only a hip yaw and a knee, the body cannot translate laterally at all, so there is no way to move the centre of mass over the support triangle before lifting a leg. Gordo simply picks the foot up and tips onto the diagonal for a moment. Everything about the gait — slow cycle, body dropped low, never more than one foot in the air — is compensation for a lean the mechanism cannot perform. It is not stable the way a real crawl is stable; it is stable the way a table with one short leg is stable.

That constraint also rules out a trot. A trot moves diagonal pairs together and is a dynamically balanced gait: it depends on the body actively catching itself, which needs either a weight shift or closed-loop balance. With two DOF and no feedback, the only workable option is a static crawl — one leg in the air at a time, three always planted.

### Direction is a sequence, not a sign

Gordo drove forward well but could not back up at all. Every equation inverts on its own when you reverse, since the hip sweep and the reach correction both scale with a stride value that simply goes negative, so the commands all looked correct.

What does not invert is the **order the legs move in**. A crawl lifts legs in a sequence chosen so the three remaining feet always form a usable support triangle, and that sequence depends on the direction of travel. Running a forward order while moving backwards lifts exactly the wrong leg every time. Reverse is now the forward gait mirrored front-to-back, so the touchdown order reverses along with the direction.

### Smaller lessons

- **The shape of the foot lift matters as much as its height.** A sinusoidal lift is only clear of the floor near mid-swing, so the toe scuffed at both ends of every step. A trapezoid — snap up, hold, ease down — fixed the shuffle at a height that had already been "high enough".
- **Servos need time, not just angles.** Commanding a taller lift inside the same swing window does nothing; a DS3218 simply lags behind. Slowing the cycle delivered the lift the commands had been asking for all along.
- **Derive signs, do not guess them.** A knee offset meant to press a newly planted foot into the floor was *lifting* it by 4° instead, at exactly the moment it should have been taking weight.
- **Check the map against the hardware.** My original channel labels were rotated one corner off from the physical robot, which made a whole round of gait debugging meaningless.
- **Tune live.** Almost every number below was found by adjusting it on a web page while the robot walked. Anything that needs a recompile to test gets tuned once and then left alone.

---

## Current Status

- [x] CAD complete (Onshape)
- [x] All parts received
- [x] Hardware fully assembled
- [x] All 8 servos calibrated and tested
- [x] Wiring schematic complete (KiCad)
- [x] WiFi web controller built
- [x] Power supply wiring — running untethered off the 2S LiPo (direct to breakout, no bucks)
- [x] Firmware — PCA9685 init and servo sweep
- [x] First walking prototype — moves under its own power (rough)
- [x] **Gait engine — crawl gait with straight-line stance correction, walks and turns**
- [x] **Reverse — mirrored leg sequence**
- [x] **Live parameter tuning from the browser, saved to flash**
- [ ] Feet — design and test (currently bare printed leg ends, slipping on carpet)
- [ ] Port the tuned gait into a standalone flight sketch
- [ ] Inverse kinematics

---

## Hardware

| Component | Part | Notes |
|---|---|---|
| Microcontroller | ESP32-S3 Freenove + Breakout Board | WiFi AP hosted on-board |
| Servo Driver | PCA9685 PWM board | I2C address 0x40, 50 Hz |
| Servos | 8x DS3218 20kg | 2 DOF per leg, hip + knee, 500–2500 µs pulse range |
| Battery | DLG 7.4V 2200mAh 2S 30C LiPo | Spliced and soldered direct to breakout power port |
| Capacitor | 2200uF 10V electrolytic | Across PCA9685 V+ and GND |
| Charger | B3 2S balance charger | Charges via JST-XH balance port |
| CAD | Onshape | Fully modelled and finalized |

---

## Leg Configuration

4 legs, 2 DOF each (hip yaw + knee). All servos driven by the PCA9685 over I2C.

| Channel | Leg | Joint |
|---|---|---|
| 0 | Front Left | Knee |
| 1 | Front Left | Hip |
| 2 | Front Right | Knee |
| 3 | Front Right | Hip |
| 4 | Back Right | Knee |
| 5 | Back Right | Hip |
| 6 | Back Left | Knee |
| 7 | Back Left | Hip |

The knee is the *reach* joint: a positive knee offset swings the foot outward and up, which is why it serves as both the foot lift and the straight-line stance correction.

---

## Gait

A static crawl. Legs sit a quarter cycle apart in phase, one leg swings at a time, and the other three sweep together to drive the body.

| Parameter | Value | Notes |
|---|---|---|
| Cycle time | 1150 ms | Long enough for the servos to actually reach the commanded lift |
| Swing duty | 0.22 | Must stay below 0.25 or two legs leave the ground at once |
| Drive stride | 30° | Hip sweep for translation; scrub grows as roughly stride² |
| Turn stride | 55° | Full sweep — turning is the one case where the arc is the right path |
| Reach ratio | 2.3 | Knee degrees per hip degree, the straight-line stance correction |
| Foot lift | 60° | Trapezoidal profile, not sinusoidal |
| Body drop | 5° | Lower body, wider effective support |
| Straight trim | 0.08 | Differential stride bias that cancels the natural drift |

Reach, stride, lift, and trim are all adjustable from the web page while the robot is walking, and are saved to flash.

---

## Software

- C++ / Arduino, built and flashed with `arduino-cli`
- PCA9685 driven through the Adafruit PWM Servo Driver library
- WiFi access point: SSID **QuadrupedTuner**, password **12345678**
- Controller served at **192.168.4.1**

```
arduino-cli compile --fqbn esp32:esp32:esp32s3 firmware/servo_tuner
arduino-cli upload -p COM4 --fqbn esp32:esp32:esp32s3 firmware/servo_tuner
arduino-cli monitor -p COM4 --config baudrate=115200
```

### File Structure

```
firmware/
├── servo_tuner/    — the real firmware: gait engine, web controller, live tuning
├── quadruped/      — earlier sketch, scripted poses and canned moves
└── center_hold/    — holds all 8 servos at 1500 us neutral, for mounting horns
hardware/
├── Gordo-Wiring.pdf   — KiCad wiring schematic
└── wiring_diagram.md
images/
└── build_photo.jpg
media/
├── gordo_walk.gif / .mp4            — current gait
└── gordo_walk_prototype.gif / .mp4  — first walking prototype, for comparison
docs/
└── notes.md
mechanical/
└── onshape_link.md
```

The earlier `firmware/main`, `firmware/gait`, and `firmware/web_controller` skeletons have been removed — they were stubs with the wrong servo pulse range and a `// TODO` trot, and `servo_tuner` supersedes all three.

---

## Web Controller

Browser-based, served directly from the ESP32. Connect to the **QuadrupedTuner** network and open 192.168.4.1.

- **Joystick** — drag to drive, release to stop. Forward and back walk, left and right turn in place, and the two blend.
- **Straight trim** — cancels the drift that comes from four legs never pushing exactly equally.
- **Foot lift** — swing height, in degrees of knee.
- **Reach** — the straight-line stance correction. Set it to 0 to fall back to the old arc-only stance and watch the feet scrub.
- **Stride** — hip sweep per step.
- **Emotes** — wave, shimmy, worm.
- **Stop / Stand** — finishes the current cycle with all feet down, then stands.

Every knob writes through to flash, so tuned values survive a power cycle and a reflash.

---

## Wiring

I2C: GPIO8 (SDA), GPIO9 (SCL) on the ESP32-S3 to the PCA9685.

Power architecture, as actually built:

- 2S LiPo (7.4V) → spliced and soldered direct to the power port on the Freenove ESP32-S3 breakout board
- Breakout board's onboard regulator → ESP32 logic **and** the 5V pin
- Breakout 5V pin → PCA9685 V+ (servo rail) → 8x DS3218
- All grounds common
- 2200uF capacitor across PCA9685 V+ and GND

The buck converters in the original plan were dropped — the breakout's onboard regulator handles the conversion instead, which removed two boards and a good amount of wiring from the chassis. All 8 servos run off this rail and drive the legs fine.

Full schematic: [hardware/Gordo-Wiring.pdf](hardware/Gordo-Wiring.pdf) — **stale.** It still shows the original twin-buck plan and needs redrawing to match the direct-to-breakout wiring described above.

---

## Notes

- Do not stall all 8 servos simultaneously.
- The 2S LiPo is soldered straight to the breakout power port, so there is no fuse or disconnect in that path. Worth adding an inline fuse and a switch.
- Servo neutral (90 deg) physically centred before mounting legs.
- PCA9685 default I2C address 0x40 — verify with an I2C scanner if unresponsive.
- Never charge LiPo unattended. Use balance charger only. Storage voltage: 3.8V/cell.
- Battery cutoff: stop use at 7.0V total (3.5V/cell).

---

## If There's a Next One

Three DOF per leg, so the thing can actually lean. Failing that, rubber feet — half of what is left is carpet friction losing an argument with geometry.
