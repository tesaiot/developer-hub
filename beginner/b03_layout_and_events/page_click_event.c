/**
 * @file    page_click_event.c
 * @brief   Click Event — Button with click counter displayed in a label
 *
 * Demonstrates lv_obj_add_event_cb with LV_EVENT_CLICKED.
 */

#include "pages.h"

static lv_obj_t *s_click_label;
static int s_click_count;

static void click_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_click_count++;

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "Clicked! Count: %d", s_click_count);
    lv_label_set_text(s_click_label, buf);
}

void page_click_event_create(lv_obj_t *parent)
{
    s_click_count = 0;

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Tap the button below.\n"
                                "Each click increments the counter.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Click button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, 12, 0);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, LV_SYMBOL_OK " Click Me!");
    lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_btn);

    lv_obj_add_event_cb(btn, click_cb, LV_EVENT_CLICKED, NULL);

    /* Result label */
    s_click_label = lv_label_create(parent);
    lv_label_set_text(s_click_label, "Clicked! Count: 0");
    lv_obj_set_style_text_font(s_click_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_click_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_click_label, LV_ALIGN_TOP_MID, 0, 150);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_add_event_cb(btn, click_cb,\n"
                                "    LV_EVENT_CLICKED, NULL);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 210);
}
