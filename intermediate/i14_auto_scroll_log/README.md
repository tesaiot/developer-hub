# I14 — Auto-Scrolling Console Log

Console-style auto-scrolling log showing all sensor readings and system events in a green-on-dark terminal look. Includes line numbering, pause/resume, and clear controls.

## What it demonstrates
- Textarea widget as a console output
- Auto-scrolling with cursor position management
- Log buffer trimming to prevent memory overflow
- Pause/resume and clear functionality via button events
- Multi-sensor event formatting

## Hardware
- Board: KIT_PSE84_AI (all sensors), KIT_PSE84_EVAL_EPC2 (available sensors)
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Watch the console scroll; tap Pause to freeze, Clear to reset
