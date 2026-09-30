# A12 — Space Shooter (Touch Control)

Production-derived Space Shooter game adapted from the TESAIoT Game Console firmware (`page_game_shooter.c`, 467 lines).

## Controls

- **D-pad Left/Right** — Move ship horizontally (7px per tick)
- **Tap "A" button** — Fire bullet upward
- **Tap Restart icon** — Restart game

## Key Patterns

- **Entity pool pattern**: Pre-allocated bullet[6] and enemy[8] arrays with active/inactive flags. No dynamic allocation during gameplay.
- **AABB collision detection**: Axis-aligned bounding box checks for bullet-enemy, ship-enemy, and enemy-past-bottom.
- **Ship sprite**: 44x20 body with left wing, right wing, and cabin sub-objects (production-accurate).
- **Enemy sprite**: 26x16 body with dual eye sub-objects at positions (5,4) and (17,4).
- **Progressive difficulty**: Spawn cooldown timer, enemy speed range 1.8-3.4 px/tick.
- **3 lives**: Enemy passing bottom or colliding with ship costs a life.
- **Game Boy 4-tone palette**: GB_DARKEST, GB_DARK, GB_LIGHT, GB_LIGHTEST.
- **CRT scanline overlay**: Horizontal lines every 8px at 20% opacity.
- **Timer-driven game loop**: 20ms tick (50fps) for smooth physics.
- **Score + Best tracking**: Persistent best score across restarts.

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Space Shooter game logic, entity pools, collision, UI |
| `game_common.c` | Shared touch input, scanline overlay, button factory |
| `game_common.h` | Game Boy palette defines, input state struct, API declarations |
