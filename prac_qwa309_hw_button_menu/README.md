# QWA309 — Hardware Button Menu

นำทางเมนู LVGL ด้วยปุ่มกายภาพ SW6=Move SW5=Select (ไม่ใช้ touch) — headless/kiosk UX pattern

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Digital I/O + UI |
| Difficulty | beginner |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `gpio`, `button`, `menu`, `navigation`

## Files

- `main_example.c`
- `hw_button_menu_ui.c`
- `hw_button_menu_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

_Composed from qwa309 button_monitor + hmi menu pattern_
