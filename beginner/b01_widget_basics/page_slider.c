/**
 * @file    page_slider.c
 * @brief   Slider — Value range selection with live value label
 */

#include "pages.h"

static lv_obj_t *s_lbl_val;

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(s_lbl_val, "Value: %" PRId32, val);
}

void page_slider_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Slider Control");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Slider */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 350);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x1A2A40), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Value label */
    s_lbl_val = lv_label_create(parent);
    lv_label_set_text(s_lbl_val, "Value: 50");
    lv_obj_set_style_text_font(s_lbl_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_lbl_val, LV_ALIGN_CENTER, 0, 40);

    /* Min/Max labels */
    lv_obj_t *lbl_min = lv_label_create(parent);
    lv_label_set_text(lbl_min, "0");
    lv_obj_set_style_text_font(lbl_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_min, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_min, LV_ALIGN_CENTER, -185, -45);

    lv_obj_t *lbl_max = lv_label_create(parent);
    lv_label_set_text(lbl_max, "100");
    lv_obj_set_style_text_font(lbl_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_max, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_max, LV_ALIGN_CENTER, 185, -45);
}
