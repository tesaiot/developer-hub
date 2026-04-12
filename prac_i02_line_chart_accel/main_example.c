/**
 * I02 - Line Chart Acceleration (Practise)
 *
 * Real-time scrolling line chart plotting BMI270 X/Y/Z acceleration.
 * 100-point history with color-coded series.
 *
 * Ported from Developer Hub — uses direct app_sensor driver instead of IPC.
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#include "bmi270/bmi270_reader.h"
#include "sensor_bus.h"

#include <limits.h>

#define CHART_POINTS  100
#define UPDATE_MS     50

/* ── Autoscale: scan data arrays, adjust Y range with padding ────── */
static void autoscale_chart(lv_obj_t *chart,
                            lv_chart_series_t *s1,
                            lv_chart_series_t *s2,
                            lv_chart_series_t *s3,
                            int32_t min_half_range)
{
    uint32_t cnt = lv_chart_get_point_count(chart);
    int32_t *d1 = lv_chart_get_y_array(chart, s1);
    int32_t *d2 = lv_chart_get_y_array(chart, s2);
    int32_t *d3 = lv_chart_get_y_array(chart, s3);
    if (!d1 || !d2 || !d3) return;

    int32_t lo = INT32_MAX, hi = INT32_MIN;
    for (uint32_t i = 0; i < cnt; i++) {
        if (d1[i] != LV_CHART_POINT_NONE) { if (d1[i] < lo) lo = d1[i]; if (d1[i] > hi) hi = d1[i]; }
        if (d2[i] != LV_CHART_POINT_NONE) { if (d2[i] < lo) lo = d2[i]; if (d2[i] > hi) hi = d2[i]; }
        if (d3[i] != LV_CHART_POINT_NONE) { if (d3[i] < lo) lo = d3[i]; if (d3[i] > hi) hi = d3[i]; }
    }
    if (lo == INT32_MAX) return;

    int32_t mid = (hi + lo) / 2;
    if ((hi - lo) < min_half_range * 2) {
        lo = mid - min_half_range;
        hi = mid + min_half_range;
    }
    int32_t pad = (hi - lo) / 10;
    if (pad < 1) pad = 1;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, lo - pad, hi + pad);
}

typedef struct {
    lv_obj_t       *chart;
    lv_chart_series_t *ser_x, *ser_y, *ser_z;
    lv_obj_t       *lbl_x, *lbl_y, *lbl_z;
} chart_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    chart_ctx_t *ctx = (chart_ctx_t *)lv_timer_get_user_data(t);
    bmi270_sample_t bmi_sample;
    if (!bmi270_reader_poll(&bmi_sample)) return;

    float ax = bmi_sample.acc_g_x;
    float ay = bmi_sample.acc_g_y;
    float az = bmi_sample.acc_g_z;

    /* Chart expects int values; scale by 100 for 2-decimal precision */
    lv_chart_set_next_value(ctx->chart, ctx->ser_x, (int32_t)(ax * 100));
    lv_chart_set_next_value(ctx->chart, ctx->ser_y, (int32_t)(ay * 100));
    lv_chart_set_next_value(ctx->chart, ctx->ser_z, (int32_t)(az * 100));

    /* Autoscale Y-axis to fit data with 10% padding */
    autoscale_chart(ctx->chart, ctx->ser_x, ctx->ser_y, ctx->ser_z, 50);

    lv_label_set_text_fmt(ctx->lbl_x, "X: %.2f g", (double)ax);
    lv_label_set_text_fmt(ctx->lbl_y, "Y: %.2f g", (double)ay);
    lv_label_set_text_fmt(ctx->lbl_z, "Z: %.2f g", (double)az);
}

void example_main(lv_obj_t *parent)
{
    static chart_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize BMI270 sensor via direct I2C driver */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    /* Title */
    example_label_create(parent, "Accelerometer (BMI270)",
                         &lv_font_montserrat_24,
                         UI_COLOR_BMI270);
    example_label_create(parent,
        "กราฟเส้นความเร่ง",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Chart */
    ctx.chart = lv_chart_create(parent);
    lv_obj_set_size(ctx.chart, 760, 320);
    lv_chart_set_type(ctx.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx.chart, CHART_POINTS);
    lv_chart_set_range(ctx.chart, LV_CHART_AXIS_PRIMARY_Y, -200, 200);
    lv_chart_set_div_line_count(ctx.chart, 5, 8);
    lv_obj_set_style_bg_color(ctx.chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.chart, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.chart, 1, 0);
    lv_obj_set_style_line_width(ctx.chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(ctx.chart, 0, 0, LV_PART_INDICATOR);

    ctx.ser_x = lv_chart_add_series(ctx.chart, lv_color_hex(0xF44336),
                                    LV_CHART_AXIS_PRIMARY_Y);
    ctx.ser_y = lv_chart_add_series(ctx.chart, UI_COLOR_SUCCESS,
                                    LV_CHART_AXIS_PRIMARY_Y);
    ctx.ser_z = lv_chart_add_series(ctx.chart, UI_COLOR_INFO,
                                    LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize all points to zero */
    for (int i = 0; i < CHART_POINTS; i++) {
        lv_chart_set_next_value(ctx.chart, ctx.ser_x, 0);
        lv_chart_set_next_value(ctx.chart, ctx.ser_y, 0);
        lv_chart_set_next_value(ctx.chart, ctx.ser_z, 0);
    }

    /* Legend row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 760, 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    ctx.lbl_x = example_label_create(row, "X: --",
                                     &lv_font_montserrat_16,
                                     lv_color_hex(0xF44336));
    ctx.lbl_y = example_label_create(row, "Y: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_SUCCESS);
    ctx.lbl_z = example_label_create(row, "Z: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_INFO);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
