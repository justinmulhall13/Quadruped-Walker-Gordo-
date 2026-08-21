# Gordo

An 8-servo quadruped robot, driven from a phone or laptop over WiFi. Gordo walks, turns, backs up, and does a few pointless emotes.

![Gordo walking](media/gordo-walking.gif)

Full clip: [media/gordo-walking.mp4](media/gordo-walking.mp4)

This is my first solo robot build — my own frame, wiring, firmware, and gait. Everything here started from a pile of servos and a board, not from a kit or an existing gait library, and most of what is written below I learned by watching Gordo fail to walk and working out why.

## Hardware

| Part | Notes |
|---|---|
| ESP32-S3 | Runs the firmware and hosts the control page as a WiFi access point |
| PCA9685 | 16-channel PWM driver at `0x40`, 50 Hz, I2C on SDA 8 / SCL 9 |
| 8x DS3218 servos | 500–2500 µs pulse range, wider than standard |

Two degrees of freedom per leg: a **hip** (yaw) and a **knee**. That choice is the single most important fact about this robot, and the rest of this README is mostly a consequence of it.

| Channel | Joint | | Channel | Joint |
|---|---|---|---|---|
| 0 | Front-left knee | | 4 | Back-right knee |
| 1 | Front-left hip | | 5 | Back-right hip |
| 2 | Front-right knee | | 6 | Back-left knee |
| 3 | Front-right hip | | 7 | Back-left hip |

## What's in here

- `servo_tuner/` — the real firmware. Hosts a WiFi AP (`QuadrupedTuner`) and serves a control page at `192.168.4.1`: a drag joystick plus live, flash-backed knobs for trim, foot lift, reach, and stride. Every gait parameter can be tuned while the robot is walking, with no reflash.
- `quadruped/` — an earlier sketch with scripted poses and canned moves.
- `center_hold/` — holds all eight servos at 1500 µs neutral. Useful when assembling horns so every joint starts from a known place.

Flash with `arduino-cli`:

```
arduino-cli compile --fqbn esp32:esp32:esp32s3 servo_tuner
arduino-cli upload -p COM4 --fqbn esp32:esp32:esp32s3 servo_tuner
```

## What I learned

### A joint can only draw the shape it is shaped to draw

The hips are yaw joints, so a planted foot can only be dragged around an **arc** centred on its own hip. Spinning in place is therefore easy: when the robot turns, that arc *is* the path each foot should follow, nothing fights, and the push is clean. Walking in a straight line asks each planted foot to trace a **straight line** instead, which a single yaw joint cannot do. With three feet down, that's three incompatible arcs, and the foot with the least grip gives up and skids.

For a long time I treated this as a tuning problem and it never got better, because it was never a tuning problem. The skid scales as roughly the square of stride length, so halving the stride cut it by about four — but the only real fix is to give the foot a second component of motion. The knee changes a foot's *reach*, so sweeping the knee proportionally to the hip lets the foot travel something much closer to a straight line: front feet pull in as the body drives past them, back feet reach out. That ratio (`REACH_RATIO`, knee degrees per hip degree) is the difference between Gordo shuffling in place and Gordo actually walking, and it's the change that produced the video above.

### Two DOF per leg means you cannot shift weight

This is the honest limitation of the build, and if I did it again it's the thing I'd change first.

A walking robot is stable when its centre of mass sits inside the triangle formed by the three feet still on the ground. Animals and good quadruped robots handle this by *shifting their weight* — leaning onto the supporting legs before picking a foot up. That takes a joint that can move the body sideways over the legs, usually a third DOF at the hip.

Gordo has no such joint. With only yaw and knee, the body cannot translate laterally at all, so there is no way to move the centre of mass over the support triangle before lifting a leg. Gordo just picks the foot up and tips onto the diagonal for a moment. That's why the gait has to run slowly, keep the body dropped low, and never have two feet off the ground at once — all of it compensating for a weight shift that the mechanism cannot perform. The gait is not stable in the way a real crawl is; it's stable in the way a table with one short leg is stable.

### Direction is a sequence, not a sign

The last bug was a good one. Gordo drove forward well but could not back up at all. Everything about the maths inverts on its own when you reverse — the hip sweep and the reach correction both scale with a stride value that simply goes negative — so the commands looked right.

What doesn't invert is the **order the legs move in**. A crawl lifts legs in a sequence chosen so that the three feet left on the ground always make a usable support triangle, and that sequence depends on which way you're travelling. Running a forward order while moving backwards lifts exactly the wrong leg each time. Backing up is now the forward gait mirrored front-to-back, so the touchdown order reverses with the direction of travel.

### Other things that turned out to matter

- **The shape of the foot lift, not just its height.** A `sin` lift is only clear of the floor near mid-swing, so the toe scuffed at both ends of every step. A trapezoid — snap up, hold, ease down — fixed the shuffle at a lift height that had already been "high enough".
- **Servos need time, not just angles.** Commanding a bigger lift inside the same swing window does nothing; a DS3218 just lags behind. Slowing the cycle delivered the lift that the commands had been asking for all along.
- **Signs are worth deriving rather than guessing.** A knee offset that was meant to press a newly planted foot into the floor was *lifting* it by 4° instead, at the exact moment it should have been taking weight.
- **Tune live.** Nearly every number here came from adjusting it on a web page while the robot walked. Anything that requires a recompile to test will get tuned once and then left alone.

## Tuned values

Live values from the run in the video: straight trim `0.08`, foot lift `60°`, reach `2.3`, stride `30`. These live in flash on the ESP32 and override the source defaults, so a fresh board will need its own pass.

## If there's a next one

Three DOF per leg, so the thing can actually lean. Failing that, rubber feet — half of what's left is just carpet friction losing an argument with geometry.
