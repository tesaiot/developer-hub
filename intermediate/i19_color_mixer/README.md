# I18 — RGB Color Mixer

Three sliders for Red, Green, and Blue channels that mix a color displayed in a large preview box. Shows the hex color code with automatic contrast-adjusted text.

## What it demonstrates
- LVGL slider widget with VALUE_CHANGED event callback
- Real-time color preview from slider values
- Hex color code formatting
- Luminance-based text contrast (black on light, white on dark)
- Interactive touch UI without sensor dependency

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Drag the R/G/B sliders to mix colors
