# QWA309 — Pot → RGB Mixer

3 potentiometers เป็น R/G/B channel (>50% = เปิดสีนั้น) ผสมเป็น 1 ใน 8 สีของ DFR0522 matrix + แสดงบน LCD — รวม SAR pots + RGB I2C

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Analog+Actuator |
| Difficulty | intermediate |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `adc`, `pot`, `rgb`, `i2c`, `mixer`

## Files

- `main_example.c`
- `pot_rgb_mixer_ui.c`
- `pot_rgb_mixer_ui.h`
- `rgb_panel.c`
- `rgb_panel.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Composed from qwa309 pot_monitor + dfr0522_rgb_matrix_
