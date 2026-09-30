# I07 — Motion Detector

Detects motion from BMI270 accelerometer magnitude changes. Shows a live alert indicator, motion event counter, and a scrollable event history log with timestamps.

## What it demonstrates
- Acceleration magnitude calculation with threshold detection
- Cooldown timer to prevent duplicate triggers
- Scrollable event log (textarea widget)
- Status indicator with color change (green=calm, red=motion)

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Shake the board to trigger motion events
