/**
 * @file    page_flex_col.c
 * @brief   Flex Column — 5 colored boxes in a vertical flex layout
 *
 * Demonstrates LV_FLEX_FLOW_COLUMN, lv_obj_set_style_pad_row for gap.
 */

#include "pages.h"

void page_flex_col_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Vertical flex column with 5 boxes.\n"
                                "Gap = 8 px between items.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Flex container */
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 200, 500);
    lv_obj_align(col, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 1, 0);
    lv_obj_set_style_border_color(col, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_pad_all(col, 5, 0);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    /* Set flex flow: vertical column */
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 8, 0);

    /* 5 colored boxes */
    static const struct { const char *text; uint32_t color; } s_boxes[] = {
        { "Row 1", 0xF44336 },
        { "Row 2", 0xFF9800 },
        { "Row 3", 0x4CAF50 },
        { "Row 4", 0x2196F3 },
        { "Row 5", 0xE040FB },
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *box = lv_obj_create(col);
        lv_obj_set_size(box, 180, 80);
        lv_obj_set_style_bg_color(box, lv_color_hex(s_boxes[i].color), 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(box, 8, 0);
        lv_obj_set_style_border_width(box, 0, 0);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(box);
        lv_label_set_text(lbl, s_boxes[i].text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
    }

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);\n"
                                "lv_obj_set_style_pad_row(col, 8, 0);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 570);
}
