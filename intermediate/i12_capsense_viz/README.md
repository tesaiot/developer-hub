# I12 — CapSense Visualization

Visualizes CapSense button presses with circular indicators and the slider position with a color-changing progress bar. Includes press event counters.

## What it demonstrates
- CapSense button state from IPC snapshot
- CapSense slider position (0-100) visualization
- Edge detection for press counting
- Color-changing bar based on slider position range
- Animated visual feedback at 20 Hz

## Hardware
- Board: KIT_PSE84_EVAL_EPC2 only (requires CapSense)
- Sensors: CapSense (buttons + slider)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Touch the CapSense buttons and slide along the slider
