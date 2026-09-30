/**
 * @file    page_switch_led.c
 * @brief   Switch toggles a virtual LED on/off with state label
 */

#include "pages.h"

static lv_obj_t *s_led;
static lv_obj_t *s_lbl_state;

static void switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (on) {
        lv_led_on(s_led);
        lv_label_set_text(s_lbl_state, "LED: ON");
        lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_SUCCESS, 0);
    } else {
        lv_led_off(s_led);
        lv_label_set_text(s_lbl_state, "LED: OFF");
        lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_ERROR, 0);
    }
}

void page_switch_led_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 25, 0);

    /* Virtual LED */
    s_led = lv_led_create(parent);
    lv_obj_set_size(s_led, 100, 100);
    lv_led_set_color(s_led, UI_COLOR_PRIMARY);
    lv_led_off(s_led);

    /* State label */
    s_lbl_state = example_label_create(parent, "LED: OFF",
                                       &lv_font_montserrat_24, UI_COLOR_ERROR);

    /* Toggle switch */
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 80, 40);
    lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
