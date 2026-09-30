# A13 — Game Common Framework Demo

Standalone demonstration of the `game_common` infrastructure used by all TESAIoT game examples (Flappy Bird, Snake, Pong, Space Shooter).

## What It Shows

- **Game Boy 4-tone palette**: Four colored rectangles displaying GB_DARKEST (0x0F380F), GB_DARK (0x306230), GB_LIGHT (0x8BAC0F), GB_LIGHTEST (0x9BBC0F)
- **CRT scanline overlay**: 300x200 panel with horizontal scanlines at 8px spacing, 20% opacity
- **Touch overlay controls**: Full D-pad (Up/Down/Left/Right), Action button, and Restart button
- **Live input state**: Real-time display of which touch buttons are currently pressed
- **"DOT MATRIX WITH STEREO SOUND"** badge (Game Boy reference)
- **Sample sprites**: Ship, enemy, and bird objects rendered in the CRT panel

## Key Patterns

- `game_input_read()`: Reads touch overlay state into a unified `game_input_state_t` struct
- `game_add_lcd_scanlines()`: Adds CRT-style lines to any LVGL object
- `game_create_touch_dpad()`: Creates transparent D-pad buttons in the left margin
- `game_create_touch_action()`: Creates "A" button in the right margin
- `game_create_touch_restart()`: Creates refresh icon in the top-right corner
- `game_touch_clear()`: Resets all touch flags (call on restart/destroy)

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Framework demo: palette, scanlines, controls, input display |
| `game_common.c` | Shared touch input, scanline overlay, button factory |
| `game_common.h` | Game Boy palette defines, input state struct, API declarations |
