/*******************************************************************************
 * File Name: game_common.c
 *
 * Description: Shared game infrastructure for Developer Hub game examples.
 *              Touch-only input (no F310 joystick, no page_manager).
 *              Game Boy theme helpers and CRT scanline overlay.
 *
 * Production source: game_common.c (BENTO Game Console)
 *
 *******************************************************************************/

#include "game_common.h"
#include <string.h>

/*******************************************************************************
 * Touch State — set by overlay callbacks, read by game_input_read()
 *******************************************************************************/
#define TOUCH_UP      (1U << 0)
#define TOUCH_DOWN    (1U << 1)
#define TOUCH_LEFT    (1U << 2)
#define TOUCH_RIGHT   (1U << 3)
#define TOUCH_ACTION  (1U << 4)
#define TOUCH_RESTART (1U << 5)

static volatile uint8_t s_touch_flags = 0;

void game_touch_clear(void) { s_touch_flags = 0; }

/*******************************************************************************
 * Control Dimensions
 *******************************************************************************/
#define CTRL_BTN_SZ    66     /* D-pad button size (large thumb target) */
#define CTRL_GAP       20     /* gap between D-pad buttons */
#define CTRL_OPA       LV_OPA_40
#define CTRL_OPA_PR    LV_OPA_90
#define CTRL_ACT_SZ    80     /* Action button (circle, prominent) */
#define CTRL_RST_SZ    48     /* Restart button */

/*******************************************************************************
 * Input: Touch flags -> game_input_state_t
 *******************************************************************************/
void game_input_read(game_input_state_t *state)
{
    uint8_t tf;

    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));

    tf = s_touch_flags;
    if (tf & TOUCH_UP)      state->up      = true;
    if (tf & TOUCH_DOWN)    state->down    = true;
    if (tf & TOUCH_LEFT)    state->left    = true;
    if (tf & TOUCH_RIGHT)   state->right   = true;
    if (tf & TOUCH_ACTION)  state->action  = true;
    if (tf & TOUCH_RESTART) state->restart = true;
}

/*******************************************************************************
 * CRT Scanline Overlay
 *******************************************************************************/
void game_add_lcd_scanlines(lv_obj_t *arena, lv_coord_t width, lv_coord_t height)
{
    lv_coord_t y;

    for (y = 2; y < height; y += 8) {
        lv_obj_t *line = lv_obj_create(arena);
        lv_obj_set_size(line, width, 1);
        lv_obj_set_pos(line, 0, y);
        lv_obj_set_style_bg_color(line, gb_color(GB_DARKEST), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_20, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    }
}

/*******************************************************************************
 * Touch Overlay — Press/Release Callbacks
 *
 * LV_EVENT_PRESSING  → keep flag set (fires every frame while held)
 * LV_EVENT_RELEASED / PRESS_LOST → clear flag
 *******************************************************************************/
static void touch_press_cb(lv_event_t *e)
{
    uint8_t flag = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    s_touch_flags |= flag;
}

static void touch_release_cb(lv_event_t *e)
{
    uint8_t flag = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    s_touch_flags &= ~flag;
}

/* For restart: edge-triggered (PRESSED, not PRESSING) */
static void touch_press_once_cb(lv_event_t *e)
{
    uint8_t flag = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    s_touch_flags |= flag;
}

/*******************************************************************************
 * Overlay Button Factory
 *******************************************************************************/
static lv_obj_t *make_touch_btn(lv_obj_t *parent, const char *icon_txt,
                                 lv_coord_t sz, uint8_t touch_flag,
                                 bool continuous)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, sz, sz);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(btn, CTRL_OPA, 0);
    lv_obj_set_style_bg_opa(btn, CTRL_OPA_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, sz / 4, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_70, 0);
    lv_obj_set_style_border_width(btn, 3, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, icon_txt);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(lbl, LV_OPA_80, 0);
    lv_obj_set_style_text_opa(lbl, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_center(lbl);

    void *ud = (void *)(uintptr_t)touch_flag;

    if (continuous) {
        lv_obj_add_event_cb(btn, touch_press_cb, LV_EVENT_PRESSING, ud);
    } else {
        lv_obj_add_event_cb(btn, touch_press_once_cb, LV_EVENT_PRESSED, ud);
    }
    lv_obj_add_event_cb(btn, touch_release_cb, LV_EVENT_RELEASED, ud);
    lv_obj_add_event_cb(btn, touch_release_cb, LV_EVENT_PRESS_LOST, ud);

    return btn;
}

/*******************************************************************************
 * game_create_touch_dpad — D-pad in left margin of parent
 *******************************************************************************/
void game_create_touch_dpad(lv_obj_t *parent, lv_coord_t arena_x,
                             lv_coord_t arena_y, lv_coord_t arena_h,
                             bool full_dpad)
{
    lv_coord_t left_cx = arena_x / 2;
    lv_coord_t mid_y = arena_y + arena_h / 2 + 40;

    /* Up */
    lv_obj_t *up = make_touch_btn(parent, LV_SYMBOL_UP, CTRL_BTN_SZ,
                                   TOUCH_UP, true);
    lv_obj_set_pos(up, left_cx - CTRL_BTN_SZ / 2,
                    mid_y - CTRL_BTN_SZ - CTRL_GAP - CTRL_BTN_SZ / 2);

    /* Down */
    lv_obj_t *dn = make_touch_btn(parent, LV_SYMBOL_DOWN, CTRL_BTN_SZ,
                                   TOUCH_DOWN, true);
    lv_obj_set_pos(dn, left_cx - CTRL_BTN_SZ / 2,
                    mid_y + CTRL_GAP + CTRL_BTN_SZ / 2);

    if (full_dpad) {
        /* Left */
        lv_obj_t *lt = make_touch_btn(parent, LV_SYMBOL_LEFT, CTRL_BTN_SZ,
                                       TOUCH_LEFT, true);
        lv_obj_set_pos(lt, left_cx - CTRL_BTN_SZ - CTRL_GAP / 2,
                        mid_y - CTRL_BTN_SZ / 2);

        /* Right */
        lv_obj_t *rt = make_touch_btn(parent, LV_SYMBOL_RIGHT, CTRL_BTN_SZ,
                                       TOUCH_RIGHT, true);
        lv_obj_set_pos(rt, left_cx + CTRL_GAP / 2,
                        mid_y - CTRL_BTN_SZ / 2);
    }
}

/*******************************************************************************
 * game_create_touch_action — "A" button in right margin
 *******************************************************************************/
void game_create_touch_action(lv_obj_t *parent, lv_coord_t arena_x,
                               lv_coord_t arena_w, lv_coord_t arena_y,
                               lv_coord_t arena_h)
{
    lv_coord_t pw = lv_obj_get_width(parent);
    lv_coord_t right_cx = arena_x + arena_w + (pw - arena_x - arena_w) / 2;
    lv_coord_t mid_y = arena_y + arena_h / 2 + 40;

    lv_obj_t *act = make_touch_btn(parent, "A", CTRL_ACT_SZ, TOUCH_ACTION, false);
    lv_obj_set_style_radius(act, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(act, 0),
                                &lv_font_montserrat_24, 0);
    lv_obj_set_pos(act, right_cx - CTRL_ACT_SZ / 2, mid_y - CTRL_ACT_SZ / 2);
}

/*******************************************************************************
 * game_create_touch_restart — Restart icon (top-right corner)
 *******************************************************************************/
void game_create_touch_restart(lv_obj_t *parent)
{
    lv_coord_t pw = lv_obj_get_width(parent);
    lv_obj_t *rst = make_touch_btn(parent, LV_SYMBOL_REFRESH, CTRL_RST_SZ,
                                    TOUCH_RESTART, false);
    lv_obj_set_pos(rst, pw - CTRL_RST_SZ - 8, 8);
}
