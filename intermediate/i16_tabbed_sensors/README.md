# I16 — Tabbed Sensors

Tab view with one tab per sensor showing detailed readings. Each tab includes derived values: altitude estimate from pressure, dew point from temperature+humidity, cardinal direction from heading.

## What it demonstrates
- LVGL tabview widget with dynamic tab creation
- Per-sensor tabs with guard macros
- Derived sensor calculations (altitude, dew point, direction)
- Clean data row layout (label left, value right)

## Hardware
- Board: KIT_PSE84_AI (4 tabs), KIT_PSE84_EVAL_EPC2 (IMU + Compass tabs)
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tap tabs to switch between sensor views
