/*******************************************************************************
 * A13 - Game Common Framework Demo
 *
 * Standalone demonstration of game_common infrastructure building blocks.
 * Shows all game framework features in one screen:
 *   - Game Boy 4-tone palette (4 colored rectangles)
 *   - CRT scanline overlay (300x200 panel)
 *   - Touch overlay controls (D-pad + Action + Restart)
 *   - Input state display (live button status)
 *   - "DOT MATRIX WITH STEREO SOUND" badge
 *
 * This is a teaching example that documents the reusable game framework
 * patterns used by Flappy Bird, Snake, Pong, and Space Shooter.
 *
 * Entry point: void example_main(lv_obj_t *parent)
 *
 *******************************************************************************/

#include "app_interface.h"
#include "tesaiot_thai.h"
#include "game_common.h"

/*******************************************************************************
 * Layout Constants
 *******************************************************************************/
#define DEMO_PANEL_W       300
#define DEMO_PANEL_H       200
#define PALETTE_SQ_W       100
#define PALETTE_SQ_H       50
#define INPUT_POLL_MS      50

/*******************************************************************************
 * State
 *******************************************************************************/
typedef struct {
    lv_obj_t *lbl_up;
    lv_obj_t *lbl_down;
    lv_obj_t *lbl_left;
    lv_obj_t *lbl_right;
    lv_obj_t *lbl_action;
    lv_obj_t *lbl_restart;
    lv_timer_t *timer;
} framework_state_t;

static framework_state_t s_fw;

/*******************************************************************************
 * Input display update
 *******************************************************************************/
static void update_input_label(lv_obj_t *lbl, const char *name, bool pressed)
{
    if (pressed) {
        lv_label_set_text_fmt(lbl, "%s: PRESSED", name);
        lv_obj_set_style_text_color(lbl, gb_color(GB_DARKEST), 0);
    } else {
        lv_label_set_text_fmt(lbl, "%s: ---", name);
        lv_obj_set_style_text_color(lbl, gb_color(GB_DARK), 0);
    }
}

static void input_poll_cb(lv_timer_t *timer)
{
    (void)timer;
    game_input_state_t input;
    game_input_read(&input);

    update_input_label(s_fw.lbl_up,      "UP",      input.up);
    update_input_label(s_fw.lbl_down,    "DOWN",    input.down);
    update_input_label(s_fw.lbl_left,    "LEFT",    input.left);
    update_input_label(s_fw.lbl_right,   "RIGHT",   input.right);
    update_input_label(s_fw.lbl_action,  "ACTION",  input.action);
    update_input_label(s_fw.lbl_restart, "RESTART", input.restart);
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    memset(&s_fw, 0, sizeof(s_fw));

    /* Dark background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Game Framework Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFC107), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* เฟรมเวิร์กเกม */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เฟรมเวิร์กเกม");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ----------------------------------------------------------------
     * Section 1: Game Boy 4-Tone Palette
     * ---------------------------------------------------------------- */
    lv_obj_t *pal_title = lv_label_create(parent);
    lv_label_set_text(pal_title, "Game Boy DMG Palette");
    lv_obj_set_style_text_color(pal_title, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(pal_title, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(pal_title, 16, 36);

    /* 4 colored rectangles in a row */
    static const struct { uint32_t color; const char *name; } palette[] = {
        { GB_DARKEST,  "DARKEST\n0x0F380F" },
        { GB_DARK,     "DARK\n0x306230" },
        { GB_LIGHT,    "LIGHT\n0x8BAC0F" },
        { GB_LIGHTEST, "LIGHTEST\n0x9BBC0F" },
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *sq = lv_obj_create(parent);
        lv_obj_set_size(sq, PALETTE_SQ_W, PALETTE_SQ_H);
        lv_obj_set_pos(sq, 16 + i * (PALETTE_SQ_W + 8), 56);
        lv_obj_set_style_bg_color(sq, gb_color(palette[i].color), 0);
        lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(sq, gb_color(GB_DARKEST), 0);
        lv_obj_set_style_border_width(sq, 2, 0);
        lv_obj_set_style_radius(sq, 4, 0);
        lv_obj_set_style_shadow_width(sq, 0, 0);
        lv_obj_set_style_pad_all(sq, 4, 0);
        lv_obj_clear_flag(sq, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(sq);
        lv_label_set_text(lbl, palette[i].name);
        /* Use contrasting text color */
        uint32_t txt_clr = (i < 2) ? GB_LIGHTEST : GB_DARKEST;
        lv_obj_set_style_text_color(lbl, gb_color(txt_clr), 0);
        lv_obj_center(lbl);
    }

    /* ----------------------------------------------------------------
     * Section 2: CRT Scanline Overlay Demo
     * ---------------------------------------------------------------- */
    lv_obj_t *crt_title = lv_label_create(parent);
    lv_label_set_text(crt_title, "CRT Scanline Overlay");
    lv_obj_set_style_text_color(crt_title, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(crt_title, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(crt_title, 16, 118);

    lv_obj_t *crt_panel = lv_obj_create(parent);
    lv_obj_set_size(crt_panel, DEMO_PANEL_W, DEMO_PANEL_H);
    lv_obj_set_pos(crt_panel, 16, 138);
    lv_obj_set_style_bg_color(crt_panel, gb_color(GB_LIGHT), 0);
    lv_obj_set_style_bg_opa(crt_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(crt_panel, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_width(crt_panel, 3, 0);
    lv_obj_set_style_radius(crt_panel, 8, 0);
    lv_obj_set_style_shadow_width(crt_panel, 0, 0);
    lv_obj_set_style_pad_all(crt_panel, 0, 0);
    lv_obj_clear_flag(crt_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Add the CRT scanlines */
    game_add_lcd_scanlines(crt_panel, DEMO_PANEL_W, DEMO_PANEL_H);

    /* "DOT MATRIX WITH STEREO SOUND" badge (Game Boy reference) */
    lv_obj_t *badge = lv_label_create(crt_panel);
    lv_label_set_text(badge, "DOT MATRIX WITH STEREO SOUND");
    lv_obj_set_style_text_color(badge, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_14, 0);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 16);

    /* Sample game objects in the CRT panel */
    lv_obj_t *demo_ship = lv_obj_create(crt_panel);
    lv_obj_set_size(demo_ship, 44, 20);
    lv_obj_set_style_bg_color(demo_ship, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(demo_ship, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(demo_ship, 1, 0);
    lv_obj_set_style_radius(demo_ship, 2, 0);
    lv_obj_clear_flag(demo_ship, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(demo_ship, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t *demo_enemy = lv_obj_create(crt_panel);
    lv_obj_set_size(demo_enemy, 26, 16);
    lv_obj_set_style_bg_color(demo_enemy, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_color(demo_enemy, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_width(demo_enemy, 1, 0);
    lv_obj_set_style_radius(demo_enemy, 2, 0);
    lv_obj_clear_flag(demo_enemy, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(demo_enemy, LV_ALIGN_CENTER, -40, 10);

    lv_obj_t *demo_bird = lv_obj_create(crt_panel);
    lv_obj_set_size(demo_bird, 24, 24);
    lv_obj_set_style_bg_color(demo_bird, gb_color(GB_DARKEST), 0);
    lv_obj_set_style_border_color(demo_bird, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_border_width(demo_bird, 2, 0);
    lv_obj_set_style_radius(demo_bird, 12, 0);
    lv_obj_clear_flag(demo_bird, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(demo_bird, LV_ALIGN_CENTER, 40, 10);

    /* Description text */
    lv_obj_t *crt_desc = lv_label_create(crt_panel);
    lv_label_set_text(crt_desc, "Scanlines: 1px @ 8px spacing, 20% opacity");
    lv_obj_set_style_text_color(crt_desc, gb_color(GB_DARKEST), 0);
    lv_obj_align(crt_desc, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* ----------------------------------------------------------------
     * Section 3: Touch Controls + Input State Display
     * ---------------------------------------------------------------- */
    lv_obj_t *input_title = lv_label_create(parent);
    lv_label_set_text(input_title, "Touch Input State (live)");
    lv_obj_set_style_text_color(input_title, gb_color(GB_LIGHTEST), 0);
    lv_obj_set_style_text_font(input_title, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(input_title, 340, 118);

    /* Input state card */
    lv_obj_t *input_card = lv_obj_create(parent);
    lv_obj_set_size(input_card, 130, DEMO_PANEL_H);
    lv_obj_set_pos(input_card, 340, 138);
    lv_obj_set_style_bg_color(input_card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(input_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(input_card, gb_color(GB_DARK), 0);
    lv_obj_set_style_border_width(input_card, 1, 0);
    lv_obj_set_style_radius(input_card, 8, 0);
    lv_obj_set_style_shadow_width(input_card, 0, 0);
    lv_obj_set_style_pad_all(input_card, 8, 0);
    lv_obj_set_flex_flow(input_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(input_card, 6, 0);
    lv_obj_clear_flag(input_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Create input state labels */
    s_fw.lbl_up = lv_label_create(input_card);
    s_fw.lbl_down = lv_label_create(input_card);
    s_fw.lbl_left = lv_label_create(input_card);
    s_fw.lbl_right = lv_label_create(input_card);
    s_fw.lbl_action = lv_label_create(input_card);
    s_fw.lbl_restart = lv_label_create(input_card);

    /* Initialize all labels */
    update_input_label(s_fw.lbl_up,      "UP",      false);
    update_input_label(s_fw.lbl_down,    "DOWN",    false);
    update_input_label(s_fw.lbl_left,    "LEFT",    false);
    update_input_label(s_fw.lbl_right,   "RIGHT",   false);
    update_input_label(s_fw.lbl_action,  "ACTION",  false);
    update_input_label(s_fw.lbl_restart, "RESTART", false);

    /* ----------------------------------------------------------------
     * Section 4: Framework Info
     * ---------------------------------------------------------------- */
    lv_obj_t *info = lv_label_create(parent);
    lv_label_set_text(info,
        "game_common.h / game_common.c provide:\n"
        "  - Game Boy 4-tone palette: GB_DARKEST..GB_LIGHTEST\n"
        "  - game_input_read() : touch overlay -> unified state\n"
        "  - game_add_lcd_scanlines() : CRT retro overlay\n"
        "  - game_create_touch_dpad() / action() / restart()");
    lv_obj_set_style_text_color(info, lv_color_hex(0xB0B0B0), 0);
    lv_obj_set_pos(info, 16, 350);

    /* Touch overlay controls (full D-pad + Action + Restart) */
    lv_obj_update_layout(parent);
    lv_coord_t px = lv_obj_get_x(crt_panel);
    lv_coord_t py = lv_obj_get_y(crt_panel);
    game_create_touch_dpad(parent, px, py, DEMO_PANEL_H, true);
    game_create_touch_action(parent, px, DEMO_PANEL_W, py, DEMO_PANEL_H);
    game_create_touch_restart(parent);

    /* Input polling timer */
    s_fw.timer = lv_timer_create(input_poll_cb, INPUT_POLL_MS, NULL);
}
