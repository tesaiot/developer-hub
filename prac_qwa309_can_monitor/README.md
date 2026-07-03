# QWA309 — CAN Bus Monitor

CANFD0 Classic CAN 2.0A @ 500 kbps (P16.2 RX / P16.3 TX, SN65HVD230) บน CM55 แบบ polled — TX heartbeat 1Hz + RX frame table บน LVGL

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | CAN Bus |
| Difficulty | advanced |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `can`, `canfd`, `500kbps`

## Files

- `main_example.c`
- `can_monitor_ui.c`
- `can_monitor_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

> **Note:** TX self-verifiable via serial; RX table needs a CAN peer / USB-CAN analyzer

_Ported from qwa309-training-base fw/examples/canbus_test (CM33->CM55, polled)_
