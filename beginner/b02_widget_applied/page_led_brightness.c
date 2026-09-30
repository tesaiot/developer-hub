/**
 * @file    page_led_brightness.c
 * @brief   Slider 0-255 controls a virtual LED, arc gauge shows brightness
 */

#include "pages.h"

static lv_obj_t *s_led;
static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_val;

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);

    /* Update LED brightness */
    lv_led_set_brightness(s_led, (uint8_t)val);

    /* Update arc gauge (map 0-255 to 0-100) */
    int32_t pct = (val * 100) / 255;
    lv_arc_set_value(s_arc, pct);

    /* Update label */
    lv_label_set_text_fmt(s_lbl_val, "Brightness: %"PRId32, val);
}

void page_led_brightness_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Virtual LED */
    s_led = lv_led_create(parent);
    lv_obj_set_size(s_led, 80, 80);
    lv_led_set_color(s_led, UI_COLOR_SUCCESS);
    lv_led_set_brightness(s_led, 128);

    /* Arc gauge (read-only) */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 180, 180);
    lv_arc_set_rotation(s_arc, 135);
    lv_arc_set_bg_angles(s_arc, 0, 270);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_value(s_arc, 50);
    lv_obj_remove_style(s_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(s_arc, UI_COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_MAIN);

    /* Value label */
    s_lbl_val = example_label_create(parent, "Brightness: 128",
                                     &lv_font_montserrat_20, UI_COLOR_TEXT);

    /* Slider 0-255 */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 350);
    lv_slider_set_range(slider, 0, 255);
    lv_slider_set_value(slider, 128, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
