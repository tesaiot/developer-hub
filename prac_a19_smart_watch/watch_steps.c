/**
 * watch_steps.c — Step counter screen for the smart watch example
 *
 * Displays a circular arc gauge (0-10000 steps) with the step count
 * in the center and estimated calories below.
 * Steps are simulated from IMU accelerometer magnitude spikes.
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "watch_screens.h"

/* Globals updated by main_example.c sensor timer */
extern lv_obj_t *g_steps_arc;
extern lv_obj_t *g_steps_label;
extern lv_obj_t *g_steps_cal_label;

lv_obj_t *watch_steps_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Steps");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_SUCCESS, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    /* Arc gauge */
    g_steps_arc = lv_arc_create(cont);
    lv_obj_set_size(g_steps_arc, 180, 180);
    lv_obj_align(g_steps_arc, LV_ALIGN_CENTER, 0, 4);
    lv_arc_set_range(g_steps_arc, 0, 10000);
    lv_arc_set_value(g_steps_arc, 0);
    lv_arc_set_bg_angles(g_steps_arc, 135, 45);
    lv_obj_set_style_arc_width(g_steps_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(g_steps_arc, lv_color_hex(0x1A2A4A), LV_PART_MAIN);
    lv_obj_set_style_arc_width(g_steps_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g_steps_arc, UI_COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_remove_style(g_steps_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(g_steps_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Step count in center of arc */
    g_steps_label = lv_label_create(cont);
    lv_label_set_text(g_steps_label, "0");
    lv_obj_set_style_text_font(g_steps_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(g_steps_label, lv_color_white(), 0);
    lv_obj_align(g_steps_label, LV_ALIGN_CENTER, 0, -6);

    /* Calorie estimation */
    g_steps_cal_label = lv_label_create(cont);
    lv_label_set_text(g_steps_cal_label, "0 kcal");
    lv_obj_set_style_text_font(g_steps_cal_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_steps_cal_label, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(g_steps_cal_label, LV_ALIGN_CENTER, 0, 22);

    /* Goal label */
    lv_obj_t *goal = lv_label_create(cont);
    lv_label_set_text(goal, "Goal: 10,000");
    lv_obj_set_style_text_font(goal, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(goal, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(goal, LV_ALIGN_BOTTOM_MID, 0, -20);

    return cont;
}
