# a18 — Multi-Screen Smart Watch

A production-derived smart watch example featuring a 280x280 circular watch face
with 4 swipeable screens. Uses LVGL tileview for horizontal swipe navigation
and dot indicators to show the active screen.

## Screens

| Screen  | Content                                           |
|---------|---------------------------------------------------|
| Clock   | Digital HH:MM:SS, date, uptime-based via FreeRTOS |
| Sensors | BMI270 accel X/Y/Z, DPS368 temp, SHT40 humidity   |
| Steps   | Arc gauge (0-10000), step count, calorie estimate  |
| Weather | Temperature, humidity, barometric altitude          |

## Files

| File              | Purpose                            |
|-------------------|------------------------------------|
| `main_example.c`  | Entry point, tileview, timers      |
| `watch_screens.h` | Forward declarations               |
| `watch_clock.c`   | Clock screen UI                    |
| `watch_sensors.c` | Sensor data screen                 |
| `watch_steps.c`   | Step counter with arc gauge        |
| `watch_weather.c` | Weather info with altitude         |

## BSP Support

- **AI Kit**: Full functionality (all sensors available)
- **Eva Kit**: IMU only; DPS368/SHT40 sections show "N/A"

## Key Techniques

- `lv_tileview_create()` for swipe-based screen navigation
- `LV_EVENT_VALUE_CHANGED` on tileview to update dot indicators
- Circular container with `lv_obj_set_style_clip_corner(true)`
- BSP feature guards (`BSP_HAS_DPS368`, `BSP_HAS_SHT40`)
- Step simulation from IMU accelerometer magnitude
- Barometric altitude using ISA pressure formula
