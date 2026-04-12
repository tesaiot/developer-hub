/**
 * I03 - Multi-Series Bar Chart (Practise)
 *
 * Bar chart comparing normalized sensor values side by side.
 * Each bar represents a different sensor reading scaled to 0-100.
 *
 * Ported from Developer Hub — uses direct app_sensor drivers instead of IPC.
 * Sensors: BMI270 + DPS368 + SHT4x + BMM350
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"

#include "bmi270/bmi270_reader.h"
#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"
#include "bmm350/bmm350_reader.h"

#define UPDATE_MS  500
#define NUM_BARS   5   /* accel, pressure, temp, humidity, heading */

typedef struct {
    lv_obj_t          *chart;
    lv_chart_series_t *ser;
    lv_obj_t          *lbl_info;
} bar_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    bar_ctx_t *ctx = (bar_ctx_t *)lv_timer_get_user_data(t);
    int idx = 0;
    int active = 0;

    /* BMI270 — accel magnitude */
    {
        bmi270_sample_t bmi_sample;
        if (bmi270_reader_poll(&bmi_sample)) {
            float mag = bmi_sample.acc_g_x < 0 ? -bmi_sample.acc_g_x : bmi_sample.acc_g_x;
            int val = (int)(mag * 50.0f);          /* 0-2g -> 0-100 */
            if (val > 100) val = 100;
            lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
            active++;
        }
        idx++;
    }

    /* DPS368 — pressure */
    {
        dps368_sample_t dps_sample;
        if (dps368_reader_poll(&dps_sample)) {
            float press = dps_sample.pressure_hpa;
            int val = (int)((press - 950.0f) / 1.0f);  /* 950-1050 hPa -> 0-100 */
            if (val < 0) val = 0;
            if (val > 100) val = 100;
            lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
            active++;
        }
        idx++;
    }

    /* SHT4x — temperature */
    {
        sht4x_sample_t sht_sample;
        if (sht4x_reader_poll(&sht_sample)) {
            int val_t = (int)((sht_sample.temperature_c - 10.0f) * 2.5f);  /* 10-50C -> 0-100 */
            if (val_t < 0) val_t = 0;
            if (val_t > 100) val_t = 100;
            lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val_t);
            idx++;

            int val_h = (int)sht_sample.humidity_rh;                       /* 0-100% direct */
            if (val_h < 0) val_h = 0;
            if (val_h > 100) val_h = 100;
            lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val_h);
            idx++;
            active += 2;
        } else {
            idx += 2;
        }
    }

    /* BMM350 — heading */
    {
        bmm350_sample_t bmm_sample;
        if (bmm350_reader_poll(&bmm_sample)) {
            float hdg = bmm_sample.heading_deg;
            int val = (int)(hdg / 3.6f);             /* 0-360 -> 0-100 */
            if (val > 100) val = 100;
            lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
            active++;
        }
        idx++;
    }

    lv_chart_refresh(ctx->chart);

    lv_label_set_text_fmt(ctx->lbl_info, "Sensors active: %d | Updated every 500 ms",
                          active);
}

void example_main(lv_obj_t *parent)
{
    static bar_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize all 4 sensors */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);
    dps368_reader_init(&sensor_i2c_controller_hal_obj);
    sht4x_reader_init(&sensor_i2c_controller_hal_obj);
    bmm350_reader_init(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    example_label_create(parent, "Multi-Sensor Bar Chart",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    example_label_create(parent,
        "กราฟแท่งหลายชุดข้อมูล",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    /* Chart */
    ctx.chart = lv_chart_create(parent);
    lv_obj_set_size(ctx.chart, 760, 320);
    lv_chart_set_type(ctx.chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(ctx.chart, NUM_BARS);
    lv_chart_set_range(ctx.chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(ctx.chart, 5, 0);
    lv_obj_set_style_bg_color(ctx.chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.chart, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.chart, 1, 0);

    ctx.ser = lv_chart_add_series(ctx.chart, UI_COLOR_PRIMARY,
                                  LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize bars to zero */
    for (int i = 0; i < NUM_BARS; i++) {
        lv_chart_set_value_by_id(ctx.chart, ctx.ser, i, 0);
    }

    /* Label legend */
    example_label_create(parent, "Accel | Pressure | Temp | Humidity | Heading",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    ctx.lbl_info = example_label_create(parent, "Starting...",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
