# I06 — Environmental Monitor

Temperature, humidity, and pressure environmental monitor with large readouts, trend arrows, progress bars, and a comfort zone indicator.

## What it demonstrates
- Multi-sensor environmental monitoring
- Trend detection (rising/falling/stable arrows)
- Comfort zone logic based on temperature and humidity ranges
- Bar widgets for visual range indication
- Fallback: uses DPS368 temp when SHT40 unavailable

## Hardware
- Board: KIT_PSE84_AI (full: temp+humidity+pressure), KIT_PSE84_EVAL_EPC2 (IMU only, limited display)
- Sensors: DPS368, SHT40

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
