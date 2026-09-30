# A09 — Flappy Bird (Touch Control)

Production-derived Flappy Bird game adapted from the TESAIoT Game Console firmware.

## Controls

- **Tap Arena** or **Tap "A" button** — Flap (gain altitude)
- **Tap Restart icon** — Restart game

## Key Patterns

- **Game Boy 4-tone palette**: GB_DARKEST, GB_DARK, GB_LIGHT, GB_LIGHTEST
- **CRT scanline overlay**: Horizontal lines every 8px at 20% opacity
- **Timer-driven game loop**: 20ms tick (50fps) for smooth physics
- **Pre-allocated pipe pool**: 3 pipe pairs reused via recycling
- **Bird sprite**: Body + wing (animated) + eye + beak sub-objects
- **Gravity physics**: 0.42 acceleration, -6.8 flap velocity
- **Score + Best tracking**: Persistent best score across restarts

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Flappy Bird game logic and UI (entry point: `example_main`) |
| `game_common.c` | Shared touch input, scanline overlay, button factory |
| `game_common.h` | Game Boy palette defines, input state struct, API declarations |
