/**
 * watch_sensors.c — Sensor data screen for the smart watch example
 *
 * Shows IMU accelerometer (X/Y/Z) with colored values.
 * Temperature from DPS368 and humidity from SHT4x.
 * Data is updated by the sensor timer in main_example.c.
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "watch_screens.h"

/* Globals updated by main_example.c sensor timer */
extern lv_obj_t *g_sensor_ax_label;
extern lv_obj_t *g_sensor_ay_label;
extern lv_obj_t *g_sensor_az_label;
extern lv_obj_t *g_sensor_temp_label;
extern lv_obj_t *g_sensor_hum_label;

lv_obj_t *watch_sensors_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(cont, 4, 0);

    /* Section title */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, LV_SYMBOL_REFRESH " IMU");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_BMI270, 0);

    /* Accel X */
    g_sensor_ax_label = lv_label_create(cont);
    lv_label_set_text(g_sensor_ax_label, "X: --- g");
    lv_obj_set_style_text_font(g_sensor_ax_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_sensor_ax_label, UI_COLOR_TEXT, 0);

    /* Accel Y */
    g_sensor_ay_label = lv_label_create(cont);
    lv_label_set_text(g_sensor_ay_label, "Y: --- g");
    lv_obj_set_style_text_font(g_sensor_ay_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_sensor_ay_label, UI_COLOR_TEXT, 0);

    /* Accel Z */
    g_sensor_az_label = lv_label_create(cont);
    lv_label_set_text(g_sensor_az_label, "Z: --- g");
    lv_obj_set_style_text_font(g_sensor_az_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_sensor_az_label, UI_COLOR_TEXT, 0);

    /* Separator */
    lv_obj_t *sep = lv_obj_create(cont);
    lv_obj_remove_style_all(sep);
    lv_obj_set_size(sep, 120, 1);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);

    /* Temperature (DPS368) */
    g_sensor_temp_label = lv_label_create(cont);
    lv_label_set_text(g_sensor_temp_label, "Temp: --- C");
    lv_obj_set_style_text_font(g_sensor_temp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_sensor_temp_label, UI_COLOR_DPS368, 0);

    /* Humidity (SHT4x) */
    g_sensor_hum_label = lv_label_create(cont);
    lv_label_set_text(g_sensor_hum_label, "Hum: --- %");
    lv_obj_set_style_text_font(g_sensor_hum_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_sensor_hum_label, UI_COLOR_SHT40, 0);

    return cont;
}
