/**
 * @file    page_long_press.c
 * @brief   Long Press — Detect short click vs long press on a single button
 *
 * Demonstrates LV_EVENT_CLICKED and LV_EVENT_LONG_PRESSED with different
 * visual feedback for each.
 */

#include "pages.h"

static lv_obj_t *s_lp_label;
static lv_obj_t *s_lp_btn;

static void short_click_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_label_set_text(s_lp_label, "Short!");
    lv_obj_set_style_text_color(s_lp_label, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_color(s_lp_btn, UI_COLOR_SUCCESS, 0);
}

static void long_press_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_label_set_text(s_lp_label, "Long Press!");
    lv_obj_set_style_text_color(s_lp_label, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_color(s_lp_btn, UI_COLOR_ERROR, 0);
}

void page_long_press_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Tap quickly for short click.\n"
                                "Hold >400ms for long press.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Button */
    s_lp_btn = lv_btn_create(parent);
    lv_obj_set_size(s_lp_btn, 220, 70);
    lv_obj_align(s_lp_btn, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(s_lp_btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_lp_btn, 14, 0);

    lv_obj_t *lbl_btn = lv_label_create(s_lp_btn);
    lv_label_set_text(lbl_btn, LV_SYMBOL_PAUSE " Press Me");
    lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_btn);

    /* Register both event types on same object */
    lv_obj_add_event_cb(s_lp_btn, short_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_lp_btn, long_press_cb, LV_EVENT_LONG_PRESSED, NULL);

    /* Result label */
    s_lp_label = lv_label_create(parent);
    lv_label_set_text(s_lp_label, "Waiting...");
    lv_obj_set_style_text_font(s_lp_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lp_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lp_label, LV_ALIGN_TOP_MID, 0, 160);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "/* Two callbacks on one button */\n"
                                "lv_obj_add_event_cb(btn, short_cb,\n"
                                "    LV_EVENT_CLICKED, NULL);\n"
                                "lv_obj_add_event_cb(btn, long_cb,\n"
                                "    LV_EVENT_LONG_PRESSED, NULL);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 220);
}
