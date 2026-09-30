# I20 — Automation Rules

Rule engine UI with 4 pre-configured automation rules. Each rule evaluates a sensor condition (e.g., "IF Accel > 1.5g") and shows a green/red indicator. A global alert shows how many rules are currently triggered.

## What it demonstrates
- Rule engine pattern: source + operator + threshold evaluation
- Multiple sensor sources via IPC snapshot
- Real-time rule evaluation at 5 Hz
- Color-coded status indicators per rule
- Global alert aggregation across all rules

## Pre-configured rules
1. Accel magnitude > 1.5 g (shake detection)
2. Temperature > 35 C (overheat alert)
3. Humidity > 70% (high moisture alert)
4. Pressure < 990 hPa (low pressure/storm warning)

## Hardware
- Board: KIT_PSE84_AI (all rules), KIT_PSE84_EVAL_EPC2 (Rule 1 only unless DPS368 present)
- Sensors: BMI270, DPS368, SHT40

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Shake the board to trigger Rule 1; breathe on sensor for temperature/humidity rules
