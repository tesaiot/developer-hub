# I05 — WiFi Scanner

Scans for nearby WiFi networks and displays results in a scrollable list with SSID, signal strength bars, RSSI in dBm, channel number, and security type.

## What it demonstrates
- WiFi Manager scan API (async start/ready/result pattern)
- Dynamic list population with network entries
- RSSI-to-signal-bar conversion with color coding
- Button event handling for manual scan trigger
- Scrollable list widget

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: None (uses WiFi radio)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tap the Scan button to discover nearby networks
