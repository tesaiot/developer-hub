/**
 * @file    page_checkbox.c
 * @brief   Checkbox — Multiple option selection with state display
 */

#include "pages.h"

static lv_obj_t *s_lbl_status;

static void checkbox_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
    const char *text = lv_checkbox_get_text(cb);
    lv_label_set_text_fmt(s_lbl_status, "%s: %s", text, checked ? "ON" : "OFF");
}

void page_checkbox_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Checkbox Options");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Checkboxes */
    static const char *s_labels[] = {
        "Theme Dark", "Show Grid", "Enable Sound", "Auto Save"
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *cb = lv_checkbox_create(parent);
        lv_checkbox_set_text(cb, s_labels[i]);
        lv_obj_set_style_text_color(cb, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(cb, &lv_font_montserrat_16, 0);
        lv_obj_align(cb, LV_ALIGN_TOP_LEFT, 40, 60 + i * 50);
        lv_obj_add_event_cb(cb, checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);

        /* Pre-check first option */
        if (i == 0) {
            lv_obj_add_state(cb, LV_STATE_CHECKED);
        }
    }

    /* Status label */
    s_lbl_status = lv_label_create(parent);
    lv_label_set_text(s_lbl_status, "Tap a checkbox");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_status, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_BOTTOM_MID, 0, -20);
}
