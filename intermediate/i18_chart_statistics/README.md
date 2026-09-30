# I17 — Chart Statistics

Line chart of BMI270 X-axis acceleration with live min/max/average statistics computed over a 120-sample rolling window. Includes a reset button to clear the statistics.

## What it demonstrates
- Ring buffer for rolling statistics window
- Real-time min/max/average computation
- Line chart with statistical summary panel
- Reset button to clear accumulated data

## Hardware
- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2
- Sensors: BMI270

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Observe statistics updating; shake board to see range; tap Reset to restart
