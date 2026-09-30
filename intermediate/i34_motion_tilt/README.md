# Motion Tilt Angles

Computes roll and pitch angles from the BMI270 accelerometer using `atan2` and displays a 2D bubble-level indicator alongside numeric readouts. Phi-split layout (left: visualization, right: values).

## What it demonstrates
- Tilt angle computation from raw accelerometer data
- `atan2f` for roll: `atan2(ay, az)` and pitch: `atan2(-ax, sqrt(ay^2 + az^2))`
- Raw-to-m/s^2 conversion: `raw / 16384 * 9.81`
- Change-gated rendering (`snap.bmi270_changed`)
- Visual dot that changes color by tilt magnitude (green/yellow/red)

## Hardware
- Board: KIT_PSE84_AI or KIT_PSE84_EVAL_EPC2 (both have BMI270)
- Sensors: BMI270 (accelerometer)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tilt the board and observe the dot move and angle values update
