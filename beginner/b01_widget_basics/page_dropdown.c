/**
 * @file    page_dropdown.c
 * @brief   Dropdown — Selection menu with live feedback label
 */

#include "pages.h"

static lv_obj_t *s_lbl_sel;

static void dropdown_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    char buf[64];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    lv_label_set_text_fmt(s_lbl_sel, "Selected: %s", buf);
}

void page_dropdown_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Dropdown Menu");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Dropdown */
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd,
        "Accelerometer\n"
        "Gyroscope\n"
        "Barometer\n"
        "Magnetometer\n"
        "Temperature");
    lv_obj_set_width(dd, 300);
    lv_obj_align(dd, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(dd, dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Selection label */
    s_lbl_sel = lv_label_create(parent);
    lv_label_set_text(s_lbl_sel, "Selected: Accelerometer");
    lv_obj_set_style_text_font(s_lbl_sel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_sel, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_sel, LV_ALIGN_CENTER, 0, 40);
}
