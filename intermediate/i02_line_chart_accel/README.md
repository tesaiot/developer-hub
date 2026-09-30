# I02 — Line Chart Acceleration

Real-time scrolling line chart plotting BMI270 X/Y/Z acceleration with a 100-point rolling history. Each axis is color-coded (Red/Green/Blue) with current values displayed below the chart.

## What it demonstrates
- LVGL chart widget with multiple line series
- Scrolling time-series data at 20 Hz
- Sensor scaling (raw int16 to g-force)
- Color-coded legend with live numeric readout

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
