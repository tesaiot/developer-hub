/**
 * @file    page_grid.c
 * @brief   Grid Layout — 3x3 numbered grid using LVGL 9 grid API
 *
 * Demonstrates lv_obj_set_grid_dsc_array, lv_obj_set_grid_cell.
 */

#include "pages.h"

void page_grid_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "3x3 grid of numbered cells.\n"
                                "Uses LVGL 9 grid descriptor arrays.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Grid descriptor arrays: 3 columns x 3 rows, each 120px */
    static int32_t s_col_dsc[] = { 120, 120, 120, LV_GRID_TEMPLATE_LAST };
    static int32_t s_row_dsc[] = { 120, 120, 120, LV_GRID_TEMPLATE_LAST };

    /* Grid container */
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 400, 400);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 5, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    /* Apply grid layout */
    lv_obj_set_grid_dsc_array(grid, s_col_dsc, s_row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);

    /* Cell colors */
    static const uint32_t s_colors[] = {
        0xF44336, 0xFF9800, 0xFFC107,
        0x4CAF50, 0x00BCD4, 0x2196F3,
        0x9C27B0, 0xE040FB, 0x607D8B,
    };

    /* Create 9 numbered cells */
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;

        lv_obj_t *cell = lv_obj_create(grid);
        lv_obj_set_style_bg_color(cell, lv_color_hex(s_colors[i]), 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(cell, 10, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        /* Place in grid cell */
        lv_obj_set_grid_cell(cell,
                             LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);

        /* Number label */
        char buf[4];
        lv_snprintf(buf, sizeof(buf), "%d", i + 1);
        lv_obj_t *lbl = lv_label_create(cell);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
    }

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_set_grid_dsc_array(grid, cols, rows);\n"
                                "lv_obj_set_grid_cell(cell, STRETCH, c, 1,\n"
                                "                     STRETCH, r, 1);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 470);
}
