/**
 * I10 - Gauge Cluster (Practise)
 *
 * Three analog-style arc gauges showing temperature, humidity, and pressure.
 * Uses LVGL arc widgets with value labels.
 *
 * Ported from Developer Hub — uses direct app_sensor drivers instead of IPC.
 * Reads DPS368 (pressure + temperature) and SHT4x (temperature + humidity).
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"
#include "sensor_bus.h"

#define UPDATE_MS  300

typedef struct {
    lv_obj_t *arc_temp, *arc_hum, *arc_press;
    lv_obj_t *lbl_temp, *lbl_hum, *lbl_press;
} gauge_ctx_t;

static lv_obj_t *make_gauge(lv_obj_t *parent, const char *title,
                             lv_color_t color, int min, int max,
                             lv_obj_t **out_arc, lv_obj_t **out_lbl)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 240, 260);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cont, 12, 0);
    lv_obj_set_style_border_color(cont, color, 0);
    lv_obj_set_style_border_width(cont, 2, 0);
    lv_obj_set_style_pad_all(cont, 8, 0);

    example_label_create(cont, title, &lv_font_montserrat_14, color);

    /* Arc gauge */
    *out_arc = lv_arc_create(cont);
    lv_obj_set_size(*out_arc, 180, 180);
    lv_arc_set_range(*out_arc, min, max);
    lv_arc_set_value(*out_arc, min);
    lv_arc_set_bg_angles(*out_arc, 135, 405);
    lv_obj_set_style_arc_width(*out_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(*out_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(*out_arc, lv_color_hex(0x1A3050), LV_PART_MAIN);
    lv_obj_set_style_arc_color(*out_arc, color, LV_PART_INDICATOR);
    lv_obj_clear_flag(*out_arc, LV_OBJ_FLAG_CLICKABLE);
    /* Hide knob */
    lv_obj_set_style_bg_opa(*out_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(*out_arc, 0, LV_PART_KNOB);

    /* Center label */
    *out_lbl = lv_label_create(*out_arc);
    lv_label_set_text(*out_lbl, "--");
    lv_obj_set_style_text_font(*out_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(*out_lbl, UI_COLOR_TEXT, 0);
    lv_obj_center(*out_lbl);

    return cont;
}

static void timer_cb(lv_timer_t *t)
{
    gauge_ctx_t *ctx = (gauge_ctx_t *)lv_timer_get_user_data(t);

    /* Read SHT4x for temperature and humidity */
    sht4x_sample_t sht_sample;
    if (sht4x_reader_poll(&sht_sample)) {
        float temp = sht_sample.temperature_c;
        float hum  = sht_sample.humidity_rh;
        lv_arc_set_value(ctx->arc_temp, (int)temp);
        lv_label_set_text_fmt(ctx->lbl_temp, "%.1f C", (double)temp);
        lv_arc_set_value(ctx->arc_hum, (int)hum);
        lv_label_set_text_fmt(ctx->lbl_hum, "%.0f %%", (double)hum);
    }

    /* Read DPS368 for pressure */
    dps368_sample_t dps_sample;
    if (dps368_reader_poll(&dps_sample)) {
        float press = dps_sample.pressure_hpa;
        int pval = (int)press;
        lv_arc_set_value(ctx->arc_press, pval);
        lv_label_set_text_fmt(ctx->lbl_press, "%.0f\nhPa", (double)press);
    }
}

void example_main(lv_obj_t *parent)
{
    static gauge_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize sensors via direct I2C drivers */
    dps368_reader_init(&sensor_i2c_controller_hal_obj);
    sht4x_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 10, 0);

    example_label_create(parent, "Gauge Cluster",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    example_label_create(parent,
        "กลุ่มมาตรวัด",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Gauges row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 770, 280);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    make_gauge(row, "Temperature", UI_COLOR_SHT40,
               -10, 50, &ctx.arc_temp, &ctx.lbl_temp);

    make_gauge(row, "Humidity", UI_COLOR_INFO,
               0, 100, &ctx.arc_hum, &ctx.lbl_hum);

    make_gauge(row, "Pressure", UI_COLOR_DPS368,
               950, 1050, &ctx.arc_press, &ctx.lbl_press);

    /* Footer */
    example_label_create(parent, "Real-time environmental gauges via direct sensor drivers",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
