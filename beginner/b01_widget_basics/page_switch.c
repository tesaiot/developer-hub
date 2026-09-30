/**
 * @file    page_switch.c
 * @brief   Switch — On/Off toggle with state label
 */

#include "pages.h"

static lv_obj_t *s_lbl_state;

static void switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    lv_label_set_text(s_lbl_state, on ? "State: ON" : "State: OFF");
    lv_obj_set_style_text_color(s_lbl_state,
        on ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
}

void page_switch_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Switch Toggle");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Tap the switch to toggle");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 40);

    /* Switch */
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 80, 40);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(sw, UI_COLOR_SUCCESS, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* State label */
    s_lbl_state = lv_label_create(parent);
    lv_label_set_text(s_lbl_state, "State: OFF");
    lv_obj_set_style_text_font(s_lbl_state, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_ERROR, 0);
    lv_obj_align(s_lbl_state, LV_ALIGN_CENTER, 0, 50);
}
