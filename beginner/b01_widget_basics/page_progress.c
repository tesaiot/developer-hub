/**
 * @file    page_progress.c
 * @brief   Progress Bar — Loading animation cycling 0 -> 100 -> 0
 */

#include "pages.h"

static lv_obj_t *s_bar;
static lv_obj_t *s_lbl_pct;
static int32_t s_value = 0;
static bool s_increasing = true;

static void progress_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (s_increasing) {
        s_value += 1;
        if (s_value >= 100) { s_value = 100; s_increasing = false; }
    } else {
        s_value -= 1;
        if (s_value <= 0) { s_value = 0; s_increasing = true; }
    }

    lv_bar_set_value(s_bar, s_value, LV_ANIM_ON);
    lv_label_set_text_fmt(s_lbl_pct, "%" PRId32 "%%", s_value);
}

void page_progress_create(lv_obj_t *parent)
{
    s_value = 0;
    s_increasing = true;

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Progress Bar");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Bar widget */
    s_bar = lv_bar_create(parent);
    lv_obj_set_size(s_bar, 350, 24);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x1A2A40), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 12, 0);
    lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, -10);

    /* Percentage label */
    s_lbl_pct = lv_label_create(parent);
    lv_label_set_text(s_lbl_pct, "0%");
    lv_obj_set_style_text_font(s_lbl_pct, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_pct, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_lbl_pct, LV_ALIGN_CENTER, 0, 30);

    /* Timer: 30ms period for smooth animation */
    lv_timer_create(progress_timer_cb, 30, NULL);
}
