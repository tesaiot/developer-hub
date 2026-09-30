# I10 — Gauge Cluster

Three analog-style arc gauges showing temperature, humidity, and pressure. Each gauge uses an LVGL arc widget with a center-aligned value label.

## What it demonstrates
- Arc widget as read-only gauge (knob hidden, clickable disabled)
- Range mapping for different sensor units
- Fallback to DPS368 temperature when SHT40 unavailable
- Clean card-based layout with color-coded borders

## Hardware
- Board: KIT_PSE84_AI (all 3 gauges), KIT_PSE84_EVAL_EPC2 (temperature only via DPS368 fallback)
- Sensors: DPS368, SHT40

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
