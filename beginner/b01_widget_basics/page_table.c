/**
 * @file    page_table.c
 * @brief   Table — Static 4x4 data grid displaying sensor information
 */

#include "pages.h"

void page_table_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Sensor Data Table");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    /* Table widget */
    lv_obj_t *table = lv_table_create(parent);
    lv_obj_set_style_text_font(table, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(table, UI_COLOR_TEXT, 0);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 10);

    /* Column widths */
    lv_table_set_column_count(table, 4);
    lv_table_set_column_width(table, 0, 110);
    lv_table_set_column_width(table, 1, 90);
    lv_table_set_column_width(table, 2, 70);
    lv_table_set_column_width(table, 3, 90);

    /* Header row */
    lv_table_set_cell_value(table, 0, 0, "Sensor");
    lv_table_set_cell_value(table, 0, 1, "Value");
    lv_table_set_cell_value(table, 0, 2, "Unit");
    lv_table_set_cell_value(table, 0, 3, "Status");

    /* Data rows */
    lv_table_set_cell_value(table, 1, 0, "BMI270");
    lv_table_set_cell_value(table, 1, 1, "9.81");
    lv_table_set_cell_value(table, 1, 2, "m/s2");
    lv_table_set_cell_value(table, 1, 3, "Active");

    lv_table_set_cell_value(table, 2, 0, "DPS368");
    lv_table_set_cell_value(table, 2, 1, "1013.25");
    lv_table_set_cell_value(table, 2, 2, "hPa");
    lv_table_set_cell_value(table, 2, 3, "Active");

    lv_table_set_cell_value(table, 3, 0, "SHT40");
    lv_table_set_cell_value(table, 3, 1, "25.3");
    lv_table_set_cell_value(table, 3, 2, "C");
    lv_table_set_cell_value(table, 3, 3, "Active");

    /* Style the header row */
    lv_obj_set_style_bg_color(table, lv_color_hex(0x1A2A40), LV_PART_ITEMS);
    lv_obj_set_style_border_color(table, lv_color_hex(0x2A3A5C), LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Static sensor data display.\nReal data uses IPC from CM33.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_BOTTOM_MID, 0, -20);
}
