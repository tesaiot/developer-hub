# I13 — Potentiometer Control

Potentiometer value simultaneously controlling an arc gauge, linear bar indicator, and a color preview box that transitions from blue through green to red as the value increases.

## What it demonstrates
- Potentiometer reading from IPC snapshot (`percent_x10`)
- Arc widget as read-only gauge with dynamic color
- Color gradient computation from percentage
- Multiple widgets driven from a single input source

## Hardware
- Board: KIT_PSE84_EVAL_EPC2 only (requires Potentiometer)
- Sensors: Potentiometer

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Turn the potentiometer and watch all indicators respond
