/**
 * I18 - Chart Statistics (Practise)
 *
 * Shows min/max/avg statistics for BMI270 acceleration data with a
 * line chart and live statistical readouts.
 *
 * Ported from Developer Hub — uses direct app_sensor driver instead of IPC.
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#include "bmi270/bmi270_reader.h"
#include "sensor_bus.h"

#include <math.h>

#define UPDATE_MS      100
#define CHART_POINTS   120
#define STATS_WINDOW   120   /* same as chart */

typedef struct {
    lv_obj_t          *chart;
    lv_chart_series_t *ser_ax;
    lv_obj_t          *lbl_cur, *lbl_min, *lbl_max, *lbl_avg;
    lv_obj_t          *lbl_samples;
    float              history[STATS_WINDOW];
    int                idx;
    int                count;
} stats_ctx_t;

static void update_stats(stats_ctx_t *ctx)
{
    if (ctx->count == 0) return;

    int n = (ctx->count < STATS_WINDOW) ? ctx->count : STATS_WINDOW;
    float mn = 999.0f, mx = -999.0f, sum = 0.0f;
    for (int i = 0; i < n; i++) {
        float v = ctx->history[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        sum += v;
    }
    float avg = sum / n;

    lv_label_set_text_fmt(ctx->lbl_min, "Min: %.3f g", (double)mn);
    lv_label_set_text_fmt(ctx->lbl_max, "Max: %.3f g", (double)mx);
    lv_label_set_text_fmt(ctx->lbl_avg, "Avg: %.3f g", (double)avg);
    lv_label_set_text_fmt(ctx->lbl_samples, "Samples: %d", n);
}

static void timer_cb(lv_timer_t *t)
{
    stats_ctx_t *ctx = (stats_ctx_t *)lv_timer_get_user_data(t);
    bmi270_sample_t bmi_sample;
    if (!bmi270_reader_poll(&bmi_sample)) return;

    float ax = bmi_sample.acc_g_x;

    /* Store in ring buffer */
    ctx->history[ctx->idx] = ax;
    ctx->idx = (ctx->idx + 1) % STATS_WINDOW;
    if (ctx->count < STATS_WINDOW) ctx->count++;

    /* Update chart */
    lv_chart_set_next_value(ctx->chart, ctx->ser_ax, (int32_t)(ax * 1000));

    /* Update labels */
    lv_label_set_text_fmt(ctx->lbl_cur, "Current: %.3f g", (double)ax);
    update_stats(ctx);
}

static void btn_reset_cb(lv_event_t *e)
{
    stats_ctx_t *ctx = (stats_ctx_t *)lv_event_get_user_data(e);
    ctx->count = 0;
    ctx->idx = 0;
    memset(ctx->history, 0, sizeof(ctx->history));
    lv_label_set_text(ctx->lbl_min, "Min: --");
    lv_label_set_text(ctx->lbl_max, "Max: --");
    lv_label_set_text(ctx->lbl_avg, "Avg: --");
    lv_label_set_text(ctx->lbl_samples, "Samples: 0");
}

void example_main(lv_obj_t *parent)
{
    static stats_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize BMI270 sensor via direct I2C driver */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 6, 0);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 776, 40);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    example_label_create(hdr, "Acceleration Statistics",
                         &lv_font_montserrat_20,
                         UI_COLOR_PRIMARY);

    lv_obj_t *btn_rst = lv_btn_create(hdr);
    lv_obj_set_size(btn_rst, 80, 32);
    lv_obj_set_style_bg_color(btn_rst, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_radius(btn_rst, 6, 0);
    lv_obj_t *lbl_r = lv_label_create(btn_rst);
    lv_label_set_text(lbl_r, "Reset");
    lv_obj_set_style_text_font(lbl_r, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_r, lv_color_white(), 0);
    lv_obj_center(lbl_r);
    lv_obj_add_event_cb(btn_rst, btn_reset_cb, LV_EVENT_CLICKED, &ctx);

    /* Chart */
    ctx.chart = lv_chart_create(parent);
    lv_obj_set_size(ctx.chart, 776, 240);
    lv_chart_set_type(ctx.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx.chart, CHART_POINTS);
    lv_chart_set_range(ctx.chart, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);
    lv_chart_set_div_line_count(ctx.chart, 5, 8);
    lv_obj_set_style_bg_color(ctx.chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.chart, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.chart, 1, 0);
    lv_obj_set_style_line_width(ctx.chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(ctx.chart, 0, 0, LV_PART_INDICATOR);

    ctx.ser_ax = lv_chart_add_series(ctx.chart,
                                     UI_COLOR_ERROR,
                                     LV_CHART_AXIS_PRIMARY_Y);

    /* Statistics panel */
    lv_obj_t *stats = example_card_create(parent, 776, 130,
                                           UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(stats, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(stats, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(stats, 12, 0);

    /* Stat cards */
    lv_color_t colors[] = {
        UI_COLOR_TEXT,
        UI_COLOR_INFO,
        UI_COLOR_ERROR,
        UI_COLOR_SUCCESS,
    };
    lv_obj_t **lbls[] = { &ctx.lbl_cur, &ctx.lbl_min, &ctx.lbl_max, &ctx.lbl_avg };
    const char *inits[] = { "Current: --", "Min: --", "Max: --", "Avg: --" };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *sc = lv_obj_create(stats);
        lv_obj_set_size(sc, 170, 70);
        lv_obj_set_style_bg_color(sc, lv_color_hex(0x0A1628), 0);
        lv_obj_set_style_bg_opa(sc, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(sc, 8, 0);
        lv_obj_set_style_border_color(sc, colors[i], 0);
        lv_obj_set_style_border_width(sc, 1, 0);
        lv_obj_set_flex_flow(sc, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(sc, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        *lbls[i] = example_label_create(sc, inits[i], &lv_font_montserrat_16,
                                        colors[i]);
    }

    ctx.lbl_samples = example_label_create(parent, "Samples: 0",
                                           &lv_font_montserrat_14,
                                           UI_COLOR_TEXT_DIM);
    example_label_create(parent,
        "สถิติกราฟ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
