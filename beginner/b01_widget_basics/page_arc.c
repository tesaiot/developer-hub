/**
 * @file    page_arc.c
 * @brief   Arc Gauge — Circular gauge with sweep animation (0-100)
 */

#include "pages.h"

static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_val;
static int32_t s_value = 0;
static bool s_increasing = true;

static void arc_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (s_increasing) {
        s_value += 2;
        if (s_value >= 100) { s_value = 100; s_increasing = false; }
    } else {
        s_value -= 2;
        if (s_value <= 0) { s_value = 0; s_increasing = true; }
    }

    lv_arc_set_value(s_arc, s_value);
    lv_label_set_text_fmt(s_lbl_val, "%" PRId32 "%%", s_value);
}

void page_arc_create(lv_obj_t *parent)
{
    s_value = 0;
    s_increasing = true;

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Arc Gauge");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Arc widget */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 200, 200);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_value(s_arc, 0);
    lv_obj_set_style_arc_color(s_arc, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1A2A40), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_MAIN);
    lv_obj_remove_style(s_arc, NULL, LV_PART_KNOB);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, -10);

    /* Value label (centered inside arc) */
    s_lbl_val = lv_label_create(parent);
    lv_label_set_text(s_lbl_val, "0%");
    lv_obj_set_style_text_font(s_lbl_val, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_lbl_val, LV_ALIGN_CENTER, 0, -10);

    /* Animation timer: 50ms period */
    lv_timer_create(arc_timer_cb, 50, NULL);
}
