/**
 * @file    page_button.c
 * @brief   Button — Click counter with styled button and live display
 */

#include "pages.h"

static int32_t s_press_count = 0;
static lv_obj_t *s_lbl_count;

static void btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_press_count++;
    lv_label_set_text_fmt(s_lbl_count, "Clicked: %" PRId32, s_press_count);
}

void page_button_create(lv_obj_t *parent)
{
    s_press_count = 0;

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Button Demo");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Count label */
    s_lbl_count = lv_label_create(parent);
    lv_label_set_text(s_lbl_count, "Clicked: 0");
    lv_obj_set_style_text_font(s_lbl_count, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_count, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_lbl_count, LV_ALIGN_CENTER, 0, -30);

    /* Click button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, 12, 0);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, LV_SYMBOL_PLUS " Click Me!");
    lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_btn);

    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, NULL);
}
