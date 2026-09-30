# I01 — Sensor Dashboard (Production-Derived)
Multi-sensor dashboard with autoscale charts and change-gated rendering.
Adapted from production `page_dashboard.c` (90%+ fidelity).

## Key Patterns
- **Autoscale chart**: Scans all data points, computes min/max, adds 10% padding
- **Change-gated rendering**: Only updates widgets when sensor data actually changed
- **BSP conditional compilation**: Compiles on AI Kit (all sensors) and Eva Kit (subset)
- **Module-static context**: All widget pointers in a single struct for clean lifecycle

## Board Compatibility
- AI Kit: Full dashboard (BMI270 + DPS368 + SHT40 + Charts)
- Eva Kit: Partial (BMI270 + CapSense + Charts, no DPS368/SHT40)

## Concepts
- `ipc_sensorhub_snapshot()` for sensor data access
- `lv_chart_set_next_value()` + `lv_chart_get_y_array()` for chart data
- `#if BSP_HAS_xxx` compile-time guards
- Altitude from barometric pressure (hypsometric formula)
- Dew point via Magnus-Tetens approximation
- Heat index via Rothfusz regression
- Comfort level classification

## How to use
1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
