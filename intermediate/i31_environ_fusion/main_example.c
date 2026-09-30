/**
 * Environment Sensor Fusion
 *
 * Fuses DPS368 barometric pressure and SHT40 humidity/temperature into
 * derived metrics: altitude, dew point, heat index, and comfort level.
 * Three arc gauges plus a derived-values detail card.
 *
 * Adapted from production page_environ.c derived metrics.
 */
#include "pse84_common.h"

#if !(BSP_HAS_DPS368 || BSP_HAS_SHT40)
#error "This example requires DPS368 or SHT40 (AI Kit)"
#endif

#define UPDATE_MS  500

typedef struct {
    lv_obj_t *arc_press, *arc_temp, *arc_hum;
    lv_obj_t *lbl_press, *lbl_temp, *lbl_hum;
    lv_obj_t *lbl_alt, *lbl_dew, *lbl_hi, *lbl_comfort;
} fusion_ctx_t;

/* ── Derived calculations (production-accurate) ─────────────── */

static float calc_altitude(float pressure_hpa)
{
    /* Hypsometric formula: h = 44330 * (1 - (P/P0)^0.190295) */
    if (pressure_hpa < 100.0f) return 0.0f;
    return 44330.0f * (1.0f - powf(pressure_hpa / 1013.25f, 0.190295f));
}

static float calc_dew_point(float temp_c, float rh_pct)
{
    /* Magnus-Tetens: gamma = (a*T)/(b+T) + ln(RH/100)
     *   a = 17.27, b = 237.7 */
    if (rh_pct < 1.0f) rh_pct = 1.0f;
    float a = 17.27f, b = 237.7f;
    float gamma = (a * temp_c) / (b + temp_c) + logf(rh_pct / 100.0f);
    return (b * gamma) / (a - gamma);
}

static float calc_heat_index(float temp_c, float rh_pct)
{
    /* Rothfusz regression (NWS) - works for T >= 80 F and RH >= 40%.
     * Below thresholds, return simple temperature. */
    float tf = temp_c * 9.0f / 5.0f + 32.0f; /* Celsius to Fahrenheit */
    if (tf < 80.0f || rh_pct < 40.0f) return temp_c;

    float hi = -42.379f
        + 2.04901523f * tf
        + 10.14333127f * rh_pct
        - 0.22475541f * tf * rh_pct
        - 0.00683783f * tf * tf
        - 0.05481717f * rh_pct * rh_pct
        + 0.00122874f * tf * tf * rh_pct
        + 0.00085282f * tf * rh_pct * rh_pct
        - 0.00000199f * tf * tf * rh_pct * rh_pct;

    /* Convert back to Celsius */
    return (hi - 32.0f) * 5.0f / 9.0f;
}

static const char *comfort_label(float temp_c, float rh_pct)
{
    if (temp_c < 10.0f) return "Cold";
    if (temp_c > 35.0f) return "Hot";
    if (rh_pct < 20.0f) return "Dry";
    if (rh_pct > 70.0f) return "Humid";
    if (temp_c >= 20.0f && temp_c <= 26.0f &&
        rh_pct >= 30.0f && rh_pct <= 60.0f)
        return "Comfortable";
    return "Acceptable";
}

static lv_color_t comfort_color(float temp_c, float rh_pct)
{
    if (temp_c < 10.0f) return UI_COLOR_INFO;
    if (temp_c > 35.0f) return UI_COLOR_ERROR;
    if (rh_pct < 20.0f || rh_pct > 70.0f) return UI_COLOR_WARNING;
    if (temp_c >= 20.0f && temp_c <= 26.0f &&
        rh_pct >= 30.0f && rh_pct <= 60.0f)
        return UI_COLOR_SUCCESS;
    return UI_COLOR_PRIMARY;
}

/* ── Timer callback ─────────────────────────────────────────── */

static void timer_cb(lv_timer_t *t)
{
    fusion_ctx_t *ctx = (fusion_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float temp_c = 0, rh_pct = 0, press_hpa = 0;

#if BSP_HAS_DPS368
    if (snap.dps368_changed) {
        press_hpa = snap.dps368.pressure_x100 / 100.0f;
        /* Map 900-1100 to arc 0-1000 */
        int pv = (int)((press_hpa - 900.0f) * 5.0f);
        if (pv < 0) pv = 0; if (pv > 1000) pv = 1000;
        lv_arc_set_value(ctx->arc_press, pv);
        lv_label_set_text_fmt(ctx->lbl_press, "%.1f\nhPa", (double)press_hpa);
        temp_c = snap.dps368.temperature_x100 / 100.0f;
    }
#endif

#if BSP_HAS_SHT40
    if (snap.sht40_changed) {
        temp_c  = snap.sht40.temperature_x100 / 100.0f;
        rh_pct  = snap.sht40.humidity_x100 / 100.0f;

        /* Temperature arc: -10 to 50 mapped to 0-1000 */
        int tv = (int)((temp_c + 10.0f) * 1000.0f / 60.0f);
        if (tv < 0) tv = 0; if (tv > 1000) tv = 1000;
        lv_arc_set_value(ctx->arc_temp, tv);
        lv_label_set_text_fmt(ctx->lbl_temp, "%.1f\n°C",
                              (double)temp_c);

        /* Humidity arc: 0-100 mapped to 0-1000 */
        int hv = (int)(rh_pct * 10.0f);
        if (hv < 0) hv = 0; if (hv > 1000) hv = 1000;
        lv_arc_set_value(ctx->arc_hum, hv);
        lv_label_set_text_fmt(ctx->lbl_hum, "%.1f\n%%RH", (double)rh_pct);
    }
#endif

    /* Derived metrics */
    float alt = 0, dew = 0, hi = 0;
#if BSP_HAS_DPS368
    alt = calc_altitude(press_hpa);
    lv_label_set_text_fmt(ctx->lbl_alt, "Altitude: %.1f m", (double)alt);
#else
    lv_label_set_text(ctx->lbl_alt, "Altitude: N/A");
#endif

#if BSP_HAS_SHT40
    dew = calc_dew_point(temp_c, rh_pct);
    hi  = calc_heat_index(temp_c, rh_pct);
    lv_label_set_text_fmt(ctx->lbl_dew, "Dew point: %.1f C", (double)dew);
    lv_label_set_text_fmt(ctx->lbl_hi,  "Heat index: %.1f C", (double)hi);
    lv_label_set_text(ctx->lbl_comfort, comfort_label(temp_c, rh_pct));
    lv_obj_set_style_text_color(ctx->lbl_comfort,
                                comfort_color(temp_c, rh_pct), 0);
#else
    lv_label_set_text(ctx->lbl_dew, "Dew point: N/A");
    lv_label_set_text(ctx->lbl_hi,  "Heat index: N/A");
    lv_label_set_text(ctx->lbl_comfort, "--");
#endif
    (void)alt; (void)dew; (void)hi;
}

/* ── Arc gauge helper ───────────────────────────────────────── */

static lv_obj_t *make_arc_gauge(lv_obj_t *parent, lv_color_t accent,
                                 fusion_ctx_t *ctx, lv_obj_t **out_arc,
                                 lv_obj_t **out_lbl)
{
    lv_obj_t *card = example_card_create(parent, 140, 160, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 6, 0);

    *out_arc = lv_arc_create(card);
    lv_obj_set_size(*out_arc, 120, 120);
    lv_arc_set_range(*out_arc, 0, 1000);
    lv_arc_set_value(*out_arc, 0);
    lv_arc_set_bg_angles(*out_arc, 135, 405);
    lv_obj_set_style_arc_width(*out_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(*out_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(*out_arc, lv_color_hex(0x1A3050), LV_PART_MAIN);
    lv_obj_set_style_arc_color(*out_arc, accent, LV_PART_INDICATOR);
    lv_obj_clear_flag(*out_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(*out_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(*out_arc, 0, LV_PART_KNOB);

    *out_lbl = lv_label_create(*out_arc);
    lv_label_set_text(*out_lbl, "--");
    lv_obj_set_style_text_font(*out_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(*out_lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_align(*out_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(*out_lbl);

    return card;
}

/* ── Entry point ────────────────────────────────────────────── */

void example_main(lv_obj_t *parent)
{
    static fusion_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 10, 0);

    example_label_create(parent, "Environment Fusion",
                         &lv_font_montserrat_24, UI_COLOR_PRIMARY);
    /* รวมข้อมูลสิ่งแวดล้อม */
    example_label_create(parent,
        "รวมข้อมูลสิ่งแวดล้อม",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Arc gauges row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 460, 180);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    make_arc_gauge(row, UI_COLOR_DPS368, &ctx, &ctx.arc_press, &ctx.lbl_press);
    make_arc_gauge(row, UI_COLOR_SHT40,  &ctx, &ctx.arc_temp,  &ctx.lbl_temp);
    make_arc_gauge(row, UI_COLOR_INFO,   &ctx, &ctx.arc_hum,   &ctx.lbl_hum);

    /* Arc labels */
    lv_obj_t *lbl_row = lv_obj_create(parent);
    lv_obj_set_size(lbl_row, 460, 24);
    lv_obj_set_flex_flow(lbl_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lbl_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(lbl_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lbl_row, 0, 0);
    lv_obj_set_style_pad_all(lbl_row, 0, 0);

    example_label_create(lbl_row, "Pressure", &lv_font_montserrat_14,
                         UI_COLOR_DPS368);
    example_label_create(lbl_row, "Temperature", &lv_font_montserrat_14,
                         UI_COLOR_SHT40);
    example_label_create(lbl_row, "Humidity", &lv_font_montserrat_14,
                         UI_COLOR_INFO);

    /* Derived values card */
    lv_obj_t *dcard = example_card_create(parent, 460, 160, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(dcard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(dcard, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(dcard, 14, 0);
    lv_obj_set_style_pad_row(dcard, 6, 0);

    example_label_create(dcard, "Derived Metrics",
                         &lv_font_montserrat_16, UI_COLOR_PRIMARY);

    ctx.lbl_alt = example_label_create(dcard, "Altitude: --",
                                       &lv_font_montserrat_14, UI_COLOR_TEXT);
    ctx.lbl_dew = example_label_create(dcard, "Dew point: --",
                                       &lv_font_montserrat_14, UI_COLOR_TEXT);
    ctx.lbl_hi  = example_label_create(dcard, "Heat index: --",
                                       &lv_font_montserrat_14, UI_COLOR_TEXT);

    /* Comfort zone row */
    lv_obj_t *ccard = example_card_create(parent, 460, 50, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(ccard, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ccard, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(ccard, 16, 0);
    lv_obj_set_style_pad_column(ccard, 10, 0);

    example_label_create(ccard, "Comfort:",
                         &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    ctx.lbl_comfort = example_label_create(ccard, "--",
                                           &lv_font_montserrat_20, UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
