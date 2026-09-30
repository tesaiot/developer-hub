/**
 * @file    page_hello.c
 * @brief   Hello World — Label creation and text styling demo
 */

#include "pages.h"

void page_hello_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Hello World");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Large greeting */
    lv_obj_t *lbl_hello = lv_label_create(parent);
    lv_label_set_text(lbl_hello, "Hello BENTO!");
    lv_obj_set_style_text_font(lbl_hello, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_hello, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_hello, LV_ALIGN_CENTER, 0, -40);

    /* Styled sub-text */
    lv_obj_t *lbl_sub = lv_label_create(parent);
    lv_label_set_text(lbl_sub, "PSoC Edge E84 - LVGL 9.2");
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 10);

    /* Colored labels row */
    static const struct { const char *text; uint32_t color; } s_colors[] = {
        { "Success", 0x4CAF50 },
        { "Warning", 0xFF9800 },
        { "Error",   0xF44336 },
        { "Info",    0x2196F3 },
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, s_colors[i].text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(s_colors[i].color), 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, (i - 2) * 90 + 45, 60);
    }
}
