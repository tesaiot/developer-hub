/*******************************************************************************
 * File Name: game_common.h
 *
 * Description: Shared game infrastructure for Developer Hub game examples.
 *              Game Boy 4-tone palette, touch-only input abstraction,
 *              scanline overlay, and common UI helpers.
 *
 *              Simplified from production game_common.h — no F310 joystick,
 *              no page_manager. Touch overlay controls only.
 *
 * Production source: game_common.h (BENTO Game Console)
 *
 *******************************************************************************/

#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * Game Boy 4-Tone Palette (Classic DMG green tones)
 *******************************************************************************/
#define GB_DARKEST   0x0F380F
#define GB_DARK      0x306230
#define GB_LIGHT     0x8BAC0F
#define GB_LIGHTEST  0x9BBC0F

/*******************************************************************************
 * F310 Joystick Deadzone (left stick, 0-255 centered at 0x80)
 *******************************************************************************/
#define GAME_STICK_DEADZONE  40   /* +/- from center (0x80) */

/*******************************************************************************
 * Unified Game Input State (touch + F310 joystick)
 *
 * Games poll this struct every tick via game_input_read().
 * Touch overlay buttons + F310 DirectInput merged via OR.
 *******************************************************************************/
typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool action;    /* A/B/X button - flap, confirm */
    bool restart;   /* Y / Restart button */
} game_input_state_t;

/*******************************************************************************
 * Color Helper
 *******************************************************************************/

/** Convert 24-bit hex to LVGL color. */
static inline lv_color_t gb_color(uint32_t hex)
{
    return lv_color_hex(hex);
}

/*******************************************************************************
 * Input API
 *******************************************************************************/

/** Read current touch state into unified input struct. */
void game_input_read(game_input_state_t *state);

/*******************************************************************************
 * Common Game UI Helpers
 *******************************************************************************/

/** Add CRT-style scanline overlay to an arena object. */
void game_add_lcd_scanlines(lv_obj_t *arena, lv_coord_t w, lv_coord_t h);

/** Create touch D-pad overlay (left margin of parent).
 *  @param parent     parent container
 *  @param arena_x    arena X position (controls go in left margin)
 *  @param arena_y    arena Y position
 *  @param arena_h    arena height (for vertical centering)
 *  @param full_dpad  true = 4-way (Snake), false = Up/Down only (Pong)
 */
void game_create_touch_dpad(lv_obj_t *parent, lv_coord_t arena_x,
                             lv_coord_t arena_y, lv_coord_t arena_h,
                             bool full_dpad);

/** Create touch Action ("A") button overlay (right margin).
 *  @param parent   parent container
 *  @param arena_x  arena X position
 *  @param arena_w  arena width
 *  @param arena_y  arena Y position
 *  @param arena_h  arena height
 */
void game_create_touch_action(lv_obj_t *parent, lv_coord_t arena_x,
                               lv_coord_t arena_w, lv_coord_t arena_y,
                               lv_coord_t arena_h);

/** Create touch Restart button overlay (top-right corner). */
void game_create_touch_restart(lv_obj_t *parent);

/** Clear all touch state flags (call when restarting or destroying). */
void game_touch_clear(void);

#endif /* GAME_COMMON_H */
