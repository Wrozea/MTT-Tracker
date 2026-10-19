# Simulator

The simulator produces two things each frame: the ground truth (where every
target really is) and what a radar-like sensor reports. The tracker only ever
sees the sensor reports. Ground truth is for the dashboard and for scoring.

## Units and coordinates

- Meters and seconds throughout.
- x points east, y points north, as on a map seen from above.
- Angles and turn rates are counterclockwise-positive, so a positive turn rate
  is a left turn.
- A frame is 0.05 s (20 Hz). Frame `k` happens at time `k * 0.05`.

## Target motion

Each target flies at constant speed with a turn rate that changes only when a
scripted turn command says so. A turn rate of 0 is a straight line.

For a target with velocity `(vx, vy)` turning at rate `w`, after `dt` seconds:

    x'  = x + (vx * sin(w dt) - vy * (1 - cos(w dt))) / w
    y'  = y + (vx * (1 - cos(w dt)) + vy * sin(w dt)) / w
    vx' = vx * cos(w dt) - vy * sin(w dt)
    vy' = vx * sin(w dt) + vy * cos(w dt)

The velocity is rotated by the angle `w dt`, and the position moves along the
circular arc. These formulas are exact rather than a step-by-step
approximation, which is why `ManyShortStepsMatchOneLongStep` can require
agreement to 1e-9 m. When `w` is nearly 0 the code uses `x' = x + v dt`
instead, because dividing by a tiny `w` would lose precision.

Ground truth has no random motion. Targets follow their script exactly. Real
targets wander slightly, and the tracker's process noise must still allow for
that; the tracker does not know the script.

## Target lifecycle

A target is reported in ground truth from its spawn time until the first of:

- its despawn time, or
- the moment it leaves the sensor's coverage area.

Once retired, a target never returns. Scripted times take effect on the first
frame at or after them.

## Sensor model

Each frame, for every target inside the coverage area:

1. With probability `detection_probability` (default 0.9) the target is
   detected. Otherwise it is missed this frame.
2. A detected target is reported at its true position plus independent
   Gaussian noise on x and y, standard deviation `position_noise_stddev`
   (default 5 m).

Then, separately:

3. The number of false alarms (clutter) is drawn from a Poisson distribution
   with mean `clutter_rate` (default 5 per frame). Each is placed uniformly at
   random in the coverage area. Poisson count with uniform placement is the
   standard clutter model in tracking literature.
4. All reports are shuffled, so their order says nothing about which are real.

The sensor also records the origin of each detection (target id or clutter).
That label is for the dashboard and tests only. The tracker is not given it.

## Demo scenario

Coverage area: x and y from -500 m to 500 m.

| Id | Appears | Start position | Velocity (m/s) | What it tests |
|---|---|---|---|---|
| 1 | 0 s | (-450, -300) | (15, 0) | Right-angle crossing with target 4 at (0, -300) at t = 30 s |
| 2 | 0 s | (-400, 100) | (20, -5) | Shallow-angle crossing with target 3 |
| 3 | 0 s | (-400, 0) | (20, 5) | Meets target 2 exactly at (-200, 50) at t = 10 s |
| 4 | 0 s | (0, 300) | (0, -20) | Crosses target 1 |
| 5 | 5 s | (300, -400) | (0, 20) | 180-degree left turn from t = 10 s to t = 26 s, then leaves the area heading south |
| 6 | 20 s | (400, 400) | (-15, -10) | Appears mid-run, disappears at t = 50 s while still in view |

## Not modeled yet

- Range and bearing measurements. The sensor reports x/y directly, which keeps
  the Kalman filter linear. See the known-limitations section of the README.
- Noise that grows with distance from the sensor, as it would for a real
  radar.
- More than one detection from the same target in a frame.
- A random scenario generator (planned for the target-count control in
  milestone 9).
