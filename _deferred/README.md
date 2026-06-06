# Deferred Practise Examples (game framework)

These 5 game examples are **not yet buildable as firmware** on the
`tesaiot_bento_kit_master` host — they are archived here, not removed. They still
run in the PC simulator and keep their original catalogue names (`a10`, `a12`–`a15`).

| Folder | Needs `game_common` | Needs `usb_hid_joystick` |
| --- | :---: | :---: |
| prac_a10_flappy_bird | ✓ | ✓ |
| prac_a12_snake_game | ✓ | ✓ |
| prac_a13_pong_game | ✓ | ✓ |
| prac_a14_game_shooter | ✓ | ✓ |
| prac_a15_game_framework | ✓ | — |

**Why deferred:** `game_common.{h,c}` exists only in the PC simulator and must be
ported to firmware; `usb_hid_joystick` has no firmware `.c` (it needs a USB host stack).

**To re-enable:** the easiest first win is `prac_a15_game_framework` (no USB HID).
Once the game framework is ported into the host and a game's dependencies resolve,
move the folder back up to the top level and it builds like any other example.
