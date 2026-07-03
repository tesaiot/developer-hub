# QWA309 — Header I/O Test

diagnostic: ทดสอบ Arduino header I/O ครบ (I2C 3V3, UART SCB9, SPI bit-bang, GPIO P13, PWM, ADC net, 4000T EZI2C) พร้อม console UI

| Field | Value |
| --- | --- |
| Board | TESAIoT_DEV_KIT |
| Board profile | `TESAIOT_DEV_KIT` |
| Domain | Header I/O / Diagnostic |
| Difficulty | advanced |

**Tags:** `tesaiot`, `qwa309`, `lvgl`, `i2c`, `uart`, `spi`, `gpio`, `pwm`, `diagnostic`

## Files

- `main_example.c`
- `header_tester_ui.c`
- `header_tester_ui.h`

## Build & run

```sh
# from tesaiot_dev_kit_master (ModusToolbox 3.8):
tools/install_episode.sh <this-folder>
make build   BOARD=TESAIOT_DEV_KIT TOOLCHAIN=GCC_ARM CONFIG=Debug
make program BOARD=TESAIOT_DEV_KIT MTB_PROBE_SERIAL=<kitprog3-serial>
```

> **Note:** full exercise needs header wiring / ESP32-S3 companion simulator + PSoC 4000T target

_Adopted from qwa309-training-base fw/examples/qwa309_header_hw_test_
