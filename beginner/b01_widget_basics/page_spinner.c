/**
 * @file    page_spinner.c
 * @brief   Spinner — Indeterminate loading animation
 */

#include "pages.h"

void page_spinner_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Spinner");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Spinner widget */
    lv_obj_t *spinner = lv_spinner_create(parent);
    lv_obj_set_size(spinner, 120, 120);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -20);
    lv_spinner_set_anim_params(spinner, 1000, 270);
    lv_obj_set_style_arc_color(spinner, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x1A2A40), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_MAIN);

    /* Loading text */
    lv_obj_t *lbl_load = lv_label_create(parent);
    lv_label_set_text(lbl_load, "Loading...");
    lv_obj_set_style_text_font(lbl_load, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_load, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_load, LV_ALIGN_CENTER, 0, 60);

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Spinners indicate indeterminate\nloading operations.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_CENTER, 0, 100);
}
