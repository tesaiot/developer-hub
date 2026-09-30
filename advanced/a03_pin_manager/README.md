# A03 — PIN Manager

4-digit PIN entry system with numeric keypad, hash-based verification, brute-force protection with lockout timer, and real-time PIN strength analysis.

## What it demonstrates

- State machine: SET -> CONFIRM -> ENTER -> VERIFY/LOCKOUT
- Touch-based numeric keypad with LVGL button events
- PIN strength analysis with animated progress bar
- Lockout timer using `lv_timer_create` for countdown
- Hash-based PIN storage pattern (never stores plaintext)
- Multi-panel layout with info sidebar

## Hardware

- Board: KIT_PSE84_AI, KIT_PSE84_EVAL_EPC2

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Set a 4-digit PIN using the on-screen keypad
4. Confirm the PIN, then test entry with correct/incorrect PINs
5. After 3 failures, the keypad locks for 30 seconds
