/**
 * @file    page_value_changed.c
 * @brief   Value Changed — Slider with live label update
 *
 * Demonstrates LV_EVENT_VALUE_CHANGED callback on a slider widget.
 */

#include "pages.h"

static lv_obj_t *s_val_label;

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "Value: %d", (int)val);
    lv_label_set_text(s_val_label, buf);

    /* Change label color based on value range */
    if (val < 33) {
        lv_obj_set_style_text_color(s_val_label, UI_COLOR_INFO, 0);
    } else if (val < 66) {
        lv_obj_set_style_text_color(s_val_label, UI_COLOR_WARNING, 0);
    } else {
        lv_obj_set_style_text_color(s_val_label, UI_COLOR_ERROR, 0);
    }
}

void page_value_changed_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Drag the slider to see the value\n"
                                "update in real time below.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Slider */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 350);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 80);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x263B5A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_KNOB);

    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Value label */
    s_val_label = lv_label_create(parent);
    lv_label_set_text(s_val_label, "Value: 50");
    lv_obj_set_style_text_font(s_val_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_val_label, UI_COLOR_WARNING, 0);
    lv_obj_align(s_val_label, LV_ALIGN_TOP_MID, 0, 140);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_add_event_cb(slider, slider_cb,\n"
                                "    LV_EVENT_VALUE_CHANGED, NULL);\n"
                                "int32_t val = lv_slider_get_value(slider);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 210);
}
