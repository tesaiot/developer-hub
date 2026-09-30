# I09 — Data Logger

Logs all available sensor readings with timestamps into a scrollable list. Supports start/stop/clear controls and auto-scrolls to the latest entry. Holds up to 200 entries before rotating out the oldest.

## What it demonstrates
- Timestamped sensor logging via `ipc_sensorhub_get_time_str()`
- Dynamic LVGL object creation and deletion for list management
- Start/stop toggle and clear button event handling
- Auto-scrolling list with entry count display

## Hardware
- Board: KIT_PSE84_AI (all sensors), KIT_PSE84_EVAL_EPC2 (available sensors)
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Logging starts automatically; use Stop/Start to control, Clear to reset
