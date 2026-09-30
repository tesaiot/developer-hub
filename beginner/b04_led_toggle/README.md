# B14 — LED Toggle

Three color-coded buttons that toggle the on-board Red, Green, and Blue LEDs via IPC commands.

## What it demonstrates

- Sending IPC commands with `ipc_send_cmd(IPC_CMD_LED_TOGGLE, ...)`
- Passing per-button data via `lv_event_get_user_data()`
- Creating a horizontal row of styled buttons
- Button shadow effects for visual feedback

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None (uses on-board LEDs)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap each button to toggle the corresponding LED on the board
