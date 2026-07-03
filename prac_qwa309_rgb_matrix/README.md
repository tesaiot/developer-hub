# QWA309 — DFR0522 RGB Dot Matrix

ควบคุม DFRobot DFR0522 RGB matrix 8x16 (I2C 0x10) บน bus 3.3V ร่วมกับ display แสดง clear/fill/pixel/pattern ผ่าน LVGL UI

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | I2C Actuator |
| Difficulty | intermediate |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `i2c`, `rgb`, `dfr0522`, `matrix`

## Files

- `main_example.c`
- `rgb_matrix_ui.c`
- `rgb_matrix_ui.h`
- `rgb_panel.c`
- `rgb_panel.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Adopted from qwa309-training-base fw/examples/dfr0522_rgb_matrix_
