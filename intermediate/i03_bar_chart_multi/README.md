# I03 — Multi-Series Bar Chart

Bar chart displaying normalized sensor values (0-100) side by side. Each bar represents a different sensor reading scaled to a common range for easy comparison.

## What it demonstrates
- LVGL bar chart with dynamic point count
- Normalizing disparate sensor ranges to a common 0-100 scale
- Compile-time sensor detection for adaptive layout
- Periodic chart refresh at 2 Hz

## Hardware
- Board: KIT_PSE84_AI (5 bars), KIT_PSE84_EVAL_EPC2 (2 bars)
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
