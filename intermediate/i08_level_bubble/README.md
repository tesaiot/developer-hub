# I08 — Bubble Level

2D bubble level using BMI270 accelerometer. A colored bubble moves on a crosshair grid based on board tilt. The bubble turns green when level, yellow when close, and red when tilted.

## What it demonstrates
- Accelerometer tilt-to-position mapping
- Circular area with crosshair reference lines
- Color-coded level feedback (green/yellow/red)
- Smooth 30 Hz animation update

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tilt the board and watch the bubble move
