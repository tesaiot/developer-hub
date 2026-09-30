# I15 — Tile Navigation

Multi-page tile view with horizontal swipe navigation between 4 pages: live sensors, acceleration chart, system status, and about. A page indicator at top shows the current page.

## What it demonstrates
- LVGL tileview widget for swipe-based navigation
- Multiple content pages with different layouts
- VALUE_CHANGED event to update page indicator
- Combining live data, charts, and static content across tiles

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270, DPS368, SHT40

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Swipe left/right to navigate between pages
