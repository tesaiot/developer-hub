# QWA309 — Push Button Monitor

อ่านปุ่มกด SW9 (P17.5) และ SW10 (P17.7) แบบ active-low pull-up แสดงสถานะกด/ปล่อย + นับจำนวนครั้งบน LVGL

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Digital I/O |
| Difficulty | beginner |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `gpio`, `button`, `switch`

## Files

- `main_example.c`
- `button_monitor_ui.c`
- `button_monitor_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Adopted from qwa309-training-base fw/examples/button_monitor_
