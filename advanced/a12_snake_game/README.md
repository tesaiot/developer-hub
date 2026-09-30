# A10 — Snake Game (Touch D-pad)

Production-derived Snake game adapted from the TESAIoT Game Console firmware.

## Controls

- **D-pad (Up/Down/Left/Right)** — Change direction (edge-triggered, prevents 180-degree reversal)
- **Tap Restart icon** — Restart game

## Key Patterns

- **Pre-allocated object pool**: 140 segment objects created at init, shown/hidden as needed
- **Grid-based movement**: 30x24 grid, 16px cells, 115ms tick rate
- **Head with orientation**: Eyes and tongue reposition based on movement direction
- **Food spawning**: Random placement with collision avoidance (up to 200 retries)
- **Edge-triggered input**: Direction changes only register on press edge, not while held
- **Score + Best + Length tracking**: All displayed in HUD

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Snake game logic, grid movement, and rendering |
| `game_common.c` | Shared touch input, scanline overlay, button factory |
| `game_common.h` | Game Boy palette defines, input state struct, API declarations |
