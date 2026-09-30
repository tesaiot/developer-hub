/**
 * watch_weather.c — Weather info screen for the smart watch example
 *
 * Displays temperature, humidity, and barometric altitude from
 * DPS368 and SHT40 sensors (BSP guarded). Shows a weather
 * condition indicator based on pressure.
 */

#include "pse84_common.h"
#include "watch_screens.h"

/* Globals updated by main_example.c sensor timer */
extern lv_obj_t *g_weather_temp_label;
extern lv_obj_t *g_weather_hum_label;
extern lv_obj_t *g_weather_alt_label;

lv_obj_t *watch_weather_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(cont, 6, 0);

    /* Weather icon (sun symbol) */
    lv_obj_t *icon = lv_label_create(cont);
    lv_label_set_text(icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_WARNING, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Weather");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_WARNING, 0);

#if BSP_HAS_DPS368
    /* Temperature */
    g_weather_temp_label = lv_label_create(cont);
    lv_label_set_text(g_weather_temp_label, "--- C");
    lv_obj_set_style_text_font(g_weather_temp_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(g_weather_temp_label, lv_color_white(), 0);

    /* Altitude */
    g_weather_alt_label = lv_label_create(cont);
    lv_label_set_text(g_weather_alt_label, "Alt: --- m");
    lv_obj_set_style_text_font(g_weather_alt_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_weather_alt_label, UI_COLOR_DPS368, 0);
#else
    lv_obj_t *no_baro = lv_label_create(cont);
    lv_label_set_text(no_baro, "DPS368 N/A");
    lv_obj_set_style_text_font(no_baro, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(no_baro, UI_COLOR_TEXT_DIM, 0);
#endif

#if BSP_HAS_SHT40
    /* Humidity */
    g_weather_hum_label = lv_label_create(cont);
    lv_label_set_text(g_weather_hum_label, "Humidity: --- %");
    lv_obj_set_style_text_font(g_weather_hum_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_weather_hum_label, UI_COLOR_SHT40, 0);
#else
    lv_obj_t *no_hum = lv_label_create(cont);
    lv_label_set_text(no_hum, "SHT40 N/A");
    lv_obj_set_style_text_font(no_hum, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(no_hum, UI_COLOR_TEXT_DIM, 0);
#endif

    return cont;
}
