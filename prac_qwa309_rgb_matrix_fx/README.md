# QWA309 — RGB Matrix FX

เอฟเฟกต์แอนิเมชันบน DFR0522 8x16 (color cycle / pixel sweep / row wipe) auto-cycle + สถานะบน LCD

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | I2C Actuator |
| Difficulty | intermediate |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `i2c`, `rgb`, `dfr0522`, `animation`

## Files

- `main_example.c`
- `rgb_fx_ui.c`
- `rgb_fx_ui.h`
- `rgb_panel.c`
- `rgb_panel.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Composed on qwa309 dfr0522_rgb_matrix driver_
