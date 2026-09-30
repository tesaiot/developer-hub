# A11 — Pong Game (Player vs AI)

Production-derived Pong game adapted from the TESAIoT Game Console firmware.

## Controls

- **D-pad Up/Down** — Move player paddle (continuous hold for smooth movement)
- **Tap Restart icon** — Restart game

## Key Patterns

- **60fps physics**: 16ms timer tick for smooth ball and paddle movement
- **AI opponent**: Right paddle tracks ball at 3.2px/frame with 5px deadzone
- **Progressive difficulty**: Ball accelerates on each paddle hit (+0.15 player, +0.12 AI)
- **Deflection physics**: Ball angle changes based on where it hits the paddle (normalized to paddle height)
- **Score to 10**: First to 10 points wins, then game over with restart prompt
- **Ball shine sprite**: 4x4 inner highlight on the ball for visual polish

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Pong game logic, physics, AI, and rendering |
| `game_common.c` | Shared touch input, scanline overlay, button factory |
| `game_common.h` | Game Boy palette defines, input state struct, API declarations |
