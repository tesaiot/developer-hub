/**
 * @file    page_textarea.c
 * @brief   Text Input — Text area with placeholder and character count
 */

#include "pages.h"

static lv_obj_t *s_lbl_count;

static void textarea_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    const char *text = lv_textarea_get_text(ta);
    uint32_t len = (uint32_t)strlen(text);
    lv_label_set_text_fmt(s_lbl_count, "Characters: %" PRIu32 " / 128", len);
}

void page_textarea_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Text Input");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Text area */
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(ta, "Type something here...");
    lv_textarea_set_max_length(ta, 128);
    lv_textarea_set_one_line(ta, false);
    lv_obj_set_size(ta, 380, 120);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(ta, textarea_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Character count label */
    s_lbl_count = lv_label_create(parent);
    lv_label_set_text(s_lbl_count, "Characters: 0 / 128");
    lv_obj_set_style_text_font(s_lbl_count, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_count, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_count, LV_ALIGN_CENTER, 0, 30);

    /* Keyboard */
    lv_obj_t *kb = lv_keyboard_create(parent);
    lv_obj_set_size(kb, DISPLAY_WIDTH - 40, 220);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_keyboard_set_textarea(kb, ta);
}
