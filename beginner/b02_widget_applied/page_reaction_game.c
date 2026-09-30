/**
 * @file    page_reaction_game.c
 * @brief   Random delay then tap as fast as possible — shows reaction time in ms
 */

#include "pages.h"

typedef enum {
    STATE_IDLE,
    STATE_WAITING,
    STATE_GO,
    STATE_RESULT,
} game_state_t;

static game_state_t s_state;
static lv_obj_t    *s_panel;
static lv_obj_t    *s_lbl_msg;
static lv_obj_t    *s_lbl_time;
static uint32_t     s_go_tick;
static uint32_t     s_delay_remain;

static void game_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_state != STATE_WAITING) return;

    if (s_delay_remain <= 50) {
        s_state   = STATE_GO;
        s_go_tick = lv_tick_get();
        lv_obj_set_style_bg_color(s_panel, UI_COLOR_SUCCESS, 0);
        lv_label_set_text(s_lbl_msg, "TAP NOW!");
    } else {
        s_delay_remain -= 50;
    }
}

static void panel_click_cb(lv_event_t *e)
{
    (void)e;

    switch (s_state) {
    case STATE_IDLE:
    case STATE_RESULT:
        s_state = STATE_WAITING;
        s_delay_remain = 1000 + (lv_tick_get() % 2000);
        lv_obj_set_style_bg_color(s_panel, UI_COLOR_ERROR, 0);
        lv_label_set_text(s_lbl_msg, "Wait for green...");
        lv_label_set_text(s_lbl_time, "");
        break;

    case STATE_WAITING:
        s_state = STATE_RESULT;
        lv_obj_set_style_bg_color(s_panel, UI_COLOR_WARNING, 0);
        lv_label_set_text(s_lbl_msg, "Too early! Tap to retry");
        lv_label_set_text(s_lbl_time, "");
        break;

    case STATE_GO: {
        uint32_t reaction_ms = lv_tick_get() - s_go_tick;
        s_state = STATE_RESULT;
        lv_obj_set_style_bg_color(s_panel, UI_COLOR_PRIMARY, 0);
        lv_label_set_text(s_lbl_msg, "Tap to play again");
        lv_label_set_text_fmt(s_lbl_time, "%"PRIu32" ms", reaction_ms);
        break;
    }
    }
}

void page_reaction_game_create(lv_obj_t *parent)
{
    s_state = STATE_IDLE;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Tappable panel */
    s_panel = lv_obj_create(parent);
    lv_obj_set_size(s_panel, 400, 220);
    lv_obj_set_style_bg_color(s_panel, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_panel, 20, 0);
    lv_obj_set_style_border_width(s_panel, 0, 0);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_panel, panel_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_set_flex_flow(s_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_panel, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(s_panel, 10, 0);

    s_lbl_msg  = example_label_create(s_panel, "Tap to Start",
                                      &lv_font_montserrat_24, lv_color_white());
    s_lbl_time = example_label_create(s_panel, "",
                                      &lv_font_montserrat_28, lv_color_white());

    /* 50 ms timer for delay countdown */
    lv_timer_create(game_timer_cb, 50, NULL);
}
