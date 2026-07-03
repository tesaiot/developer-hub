# QWA309 — Potentiometer Monitor

อ่าน 4 potentiometers (P15.4–P15.7) ผ่าน AUTANALOG SAR ADC 12-bit (Vref 1.8V) แสดงเป็น bar + แรงดัน + เปอร์เซ็นต์ real-time — practise แรกที่ใช้ ADC จริงบน TESAIoT Dev Kit

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Analog / ADC |
| Difficulty | intermediate |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `potentiometer`, `adc`, `autanalog`, `sar`

## Files

- `main_example.c`
- `pot_monitor_ui.c`
- `pot_monitor_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

> **Note:** Uses BSP-provided autonomous_analog_init / CYBSP_SAR_ADC_gpio_ch_cfg. Confirm SAR channel GPIO4-7 map to P15.4-P15.7 on QWA309 design.modus before promoting to PASS.

_Ported from qwa309-training-base fw/examples/pot_monitor (HW-validated on QWA309)._
