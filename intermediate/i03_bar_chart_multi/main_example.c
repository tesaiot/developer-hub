/**
 * I03 - Multi-Series Bar Chart
 *
 * Bar chart comparing normalized sensor values side by side.
 * Each bar represents a different sensor reading scaled to 0–100.
 */
#include "pse84_common.h"

#define UPDATE_MS  500

typedef struct {
    lv_obj_t          *chart;
    lv_chart_series_t *ser;
    lv_obj_t          *lbl_info;
    int                num_bars;
} bar_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    bar_ctx_t *ctx = (bar_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    int idx = 0;

#if BSP_HAS_BMI270
    {
        float ax = snap.bmi270.ax / 16384.0f;
        float mag = ax < 0 ? -ax : ax;
        int val = (int)(mag * 50.0f);          /* 0-2g → 0-100 */
        if (val > 100) val = 100;
        lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
    }
    idx++;
#endif

#if BSP_HAS_DPS368
    {
        float press = snap.dps368.pressure_x100 / 100.0f;
        int val = (int)((press - 950.0f) / 1.0f);  /* 950-1050 hPa → 0-100 */
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
    }
    idx++;
#endif

#if BSP_HAS_SHT40
    {
        float temp = snap.sht40.temperature_x100 / 100.0f;
        int val = (int)((temp - 10.0f) * 2.5f);  /* 10-50C → 0-100 */
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
    }
    idx++;
    {
        float hum = snap.sht40.humidity_x100 / 100.0f;
        int val = (int)hum;                       /* 0-100% direct */
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
    }
    idx++;
#endif

#if BSP_HAS_BMM350
    {
        float hdg = snap.bmm350.heading_x10 / 10.0f;
        int val = (int)(hdg / 3.6f);             /* 0-360 → 0-100 */
        if (val > 100) val = 100;
        lv_chart_set_value_by_id(ctx->chart, ctx->ser, idx, val);
    }
    idx++;
#endif

    lv_chart_refresh(ctx->chart);

    lv_label_set_text_fmt(ctx->lbl_info, "Sensors active: %d | Updated every 500 ms",
                          idx);
}

void example_main(lv_obj_t *parent)
{
    static bar_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

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
    /* กราฟแท่งหลายชุดข้อมูล */
    example_label_create(parent,
        "กราฟแท่งหลายชุดข้อมูล",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Count available bars */
    int nbars = 0;
#if BSP_HAS_BMI270
    nbars++;
#endif
#if BSP_HAS_DPS368
    nbars++;
#endif
#if BSP_HAS_SHT40
    nbars += 2;  /* temp + humidity */
#endif
#if BSP_HAS_BMM350
    nbars++;
#endif
    if (nbars == 0) nbars = 1;
    ctx.num_bars = nbars;

    /* Chart */
    ctx.chart = lv_chart_create(parent);
    lv_obj_set_size(ctx.chart, 760, 320);
    lv_chart_set_type(ctx.chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(ctx.chart, nbars);
    lv_chart_set_range(ctx.chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(ctx.chart, 5, 0);
    lv_obj_set_style_bg_color(ctx.chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.chart, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.chart, 1, 0);

    ctx.ser = lv_chart_add_series(ctx.chart, UI_COLOR_PRIMARY,
                                  LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize bars to zero */
    for (int i = 0; i < nbars; i++) {
        lv_chart_set_value_by_id(ctx.chart, ctx.ser, i, 0);
    }

    /* Label legend */
    const char *legend_text =
        ""
#if BSP_HAS_BMI270
        "Accel | "
#endif
#if BSP_HAS_DPS368
        "Pressure | "
#endif
#if BSP_HAS_SHT40
        "Temp | Humidity | "
#endif
#if BSP_HAS_BMM350
        "Heading"
#endif
        ;
    example_label_create(parent, legend_text, &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    ctx.lbl_info = example_label_create(parent, "Starting...",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
