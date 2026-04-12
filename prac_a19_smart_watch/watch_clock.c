/**
 * watch_clock.c — Digital clock screen for the smart watch example
 *
 * Displays HH:MM:SS in large font with date below.
 * Time is derived from FreeRTOS tick count (uptime-based).
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "watch_screens.h"

/* Globals updated by main_example.c clock timer */
extern lv_obj_t *g_clock_time_label;
extern lv_obj_t *g_clock_date_label;

lv_obj_t *watch_clock_create(lv_obj_t *parent)
{
    /* Container for vertical centering */
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(cont, 6, 0);

    /* Clock icon */
    lv_obj_t *icon = lv_label_create(cont);
    lv_label_set_text(icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);

    /* Time display — HH:MM:SS */
    g_clock_time_label = lv_label_create(cont);
    lv_label_set_text(g_clock_time_label, "00:00:00");
    lv_obj_set_style_text_font(g_clock_time_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(g_clock_time_label, lv_color_white(), 0);

    /* Date display */
    g_clock_date_label = lv_label_create(cont);
    lv_label_set_text(g_clock_date_label, "--- - Day 0");
    lv_obj_set_style_text_font(g_clock_date_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_clock_date_label, UI_COLOR_TEXT_DIM, 0);

    /* "BENTO Watch" branding */
    lv_obj_t *brand = lv_label_create(cont);
    lv_label_set_text(brand, "BENTO Watch");
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(brand, UI_COLOR_PRIMARY, 0);

    return cont;
}
