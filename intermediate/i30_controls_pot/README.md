# Potentiometer Arc Gauge

Large arc gauge (0-100%) driven by the on-board potentiometer, with a detail card showing raw ADC value, calculated voltage, and percentage. Change-gated updates at 100 ms.

## What it demonstrates
- Potentiometer reading from IPC snapshot (`percent_x10`, `raw`)
- Arc widget as a read-only gauge with dynamic color gradient
- Voltage calculation from raw ADC: `raw * 3.3 / 65535`
- BSP guard pattern with fallback message

## Hardware
- Board: KIT_PSE84_EVAL_EPC2 only (requires Potentiometer)
- Sensors: Potentiometer (SAR ADC)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Turn the potentiometer and watch the arc and values update
