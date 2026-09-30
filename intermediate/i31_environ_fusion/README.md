# Environment Sensor Fusion

Fuses DPS368 barometric pressure and SHT40 humidity/temperature into derived environmental metrics. Three arc gauges for raw readings plus a detail card with altitude, dew point, heat index, and comfort level assessment.

## What it demonstrates
- DPS368 pressure and SHT40 temperature/humidity from IPC snapshot
- Barometric altitude: hypsometric formula `44330 * (1 - (P/1013.25)^0.190295)`
- Dew point: Magnus-Tetens formula
- Heat index: Rothfusz 9-coefficient regression (NWS standard)
- Comfort level classification (Cold/Hot/Dry/Humid/Comfortable/Acceptable)
- Arc gauge widgets as read-only indicators
- Change-gated rendering per sensor

## Hardware
- Board: KIT_PSE84_AI (both DPS368 and SHT40)
- Partially works on Eva Kit (no DPS368/SHT40, shows N/A for missing sensors)
- Sensors: DPS368 (barometric pressure), SHT40 (humidity + temperature)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Observe real-time environmental readings and derived metrics
