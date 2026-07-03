# QWA309 — 4-Channel ADC Scope

plot ค่า pot 4 ตัว (P15.4-7, SAR 12-bit) เป็นเส้น scrolling บน LVGL chart 0-100% — analog oscilloscope

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Analog / Chart |
| Difficulty | intermediate |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `adc`, `pot`, `chart`, `scope`

## Files

- `main_example.c`
- `adc_scope_ui.c`
- `adc_scope_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Composed on qwa309 pot SAR setup_
