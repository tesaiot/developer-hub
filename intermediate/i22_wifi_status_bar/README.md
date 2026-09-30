# I19 — WiFi Status Bar

Top status bar pattern showing WiFi connection status (SSID + RSSI), NTP sync indicator, current time, and blinking sensor activity dots. Below is a content area with live sensor summary.

## What it demonstrates
- Status bar UI pattern for embedded dashboards
- WiFi Manager status polling
- NTP sync and time string display
- Sensor activity indicators with opacity animation
- Separation of status chrome from main content area

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270, DPS368, SHT40, BMM350

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
