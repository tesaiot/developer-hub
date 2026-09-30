# CapSense Controls

Large circular indicators for BTN0/BTN1 touch state and a progress bar for the CapSense slider position. Change-gated updates at 100 ms.

## What it demonstrates
- CapSense button state (pressed/idle) with animated circle indicators
- CapSense slider (0-100%) with progress bar
- Change-gated rendering (`snap.capsense_changed`)
- BSP guard pattern with user-friendly fallback message

## Hardware
- Board: KIT_PSE84_EVAL_EPC2 only (requires CapSense)
- Sensors: CapSense (2 buttons + linear slider)

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Touch the CapSense buttons and slide along the slider
