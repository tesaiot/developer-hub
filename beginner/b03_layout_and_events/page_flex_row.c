/**
 * @file    page_flex_row.c
 * @brief   Flex Row — 5 colored boxes in a horizontal flex layout
 *
 * Demonstrates LV_FLEX_FLOW_ROW, lv_obj_set_flex_flow, pad_column for gap.
 */

#include "pages.h"

void page_flex_row_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Horizontal flex row with 5 boxes.\n"
                                "Gap = 10 px between items.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Flex container */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 440, 100);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_pad_all(row, 5, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    /* Set flex flow: horizontal row */
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);

    /* 5 colored boxes with labels */
    static const struct { const char *text; uint32_t color; } s_boxes[] = {
        { "A", 0xF44336 },   /* Red    */
        { "B", 0xFF9800 },   /* Orange */
        { "C", 0x4CAF50 },   /* Green  */
        { "D", 0x2196F3 },   /* Blue   */
        { "E", 0xE040FB },   /* Purple */
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *box = lv_obj_create(row);
        lv_obj_set_size(box, 70, 70);
        lv_obj_set_style_bg_color(box, lv_color_hex(s_boxes[i].color), 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(box, 8, 0);
        lv_obj_set_style_border_width(box, 0, 0);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(box);
        lv_label_set_text(lbl, s_boxes[i].text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
    }

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);\n"
                                "lv_obj_set_style_pad_column(row, 10, 0);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 180);
}
