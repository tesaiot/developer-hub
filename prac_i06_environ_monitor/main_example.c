/**
 * I06 - Environmental Monitor (Practise)
 *
 * Temperature + Humidity + Pressure monitor with large readouts,
 * trend indicators, and comfort zone assessment.
 *
 * Ported from Developer Hub — uses direct app_sensor drivers instead of IPC.
 * Sensors: DPS368 + SHT4x
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"

#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"

#define UPDATE_MS  500

typedef struct {
    lv_obj_t *lbl_temp, *lbl_hum, *lbl_press;
    lv_obj_t *lbl_temp_trend, *lbl_hum_trend, *lbl_press_trend;
    lv_obj_t *lbl_comfort;
    lv_obj_t *bar_temp, *bar_hum, *bar_press;
    float     prev_temp, prev_hum, prev_press;
    bool      has_prev;
} env_ctx_t;

static const char *trend_arrow(float curr, float prev)
{
    float diff = curr - prev;
    if (diff > 0.2f) return LV_SYMBOL_UP;
    if (diff < -0.2f) return LV_SYMBOL_DOWN;
    return "=";
}

static const char *comfort_zone(float temp, float hum)
{
    if (temp >= 20.0f && temp <= 26.0f && hum >= 30.0f && hum <= 60.0f)
        return "Comfortable";
    if (temp > 30.0f || hum > 70.0f)
        return "Too Hot / Humid";
    if (temp < 18.0f)
        return "Too Cold";
    if (hum < 25.0f)
        return "Too Dry";
    return "Moderate";
}

static lv_color_t comfort_color(float temp, float hum)
{
    if (temp >= 20.0f && temp <= 26.0f && hum >= 30.0f && hum <= 60.0f)
        return UI_COLOR_SUCCESS;
    if (temp > 30.0f || hum > 70.0f)
        return UI_COLOR_ERROR;
    if (temp < 18.0f)
        return UI_COLOR_INFO;
    return UI_COLOR_WARNING;
}

static void timer_cb(lv_timer_t *t)
{
    env_ctx_t *ctx = (env_ctx_t *)lv_timer_get_user_data(t);

    float temp = 0, hum = 0, press = 0;

    /* SHT4x — temperature and humidity */
    {
        sht4x_sample_t sht_sample;
        if (sht4x_reader_poll(&sht_sample)) {
            temp = sht_sample.temperature_c;
            hum  = sht_sample.humidity_rh;
            lv_label_set_text_fmt(ctx->lbl_temp, "%.1f C", (double)temp);
            lv_label_set_text_fmt(ctx->lbl_hum, "%.1f %%", (double)hum);
            lv_bar_set_value(ctx->bar_temp, (int)temp, LV_ANIM_ON);
            lv_bar_set_value(ctx->bar_hum, (int)hum, LV_ANIM_ON);
        }
    }

    /* DPS368 — pressure (and fallback temperature) */
    {
        dps368_sample_t dps_sample;
        if (dps368_reader_poll(&dps_sample)) {
            press = dps_sample.pressure_hpa;
            lv_label_set_text_fmt(ctx->lbl_press, "%.1f hPa", (double)press);
            /* Map 950-1050 to 0-100 */
            int pbar = (int)((press - 950.0f));
            if (pbar < 0) pbar = 0; if (pbar > 100) pbar = 100;
            lv_bar_set_value(ctx->bar_press, pbar, LV_ANIM_ON);

            /* Use DPS368 temp as fallback if SHT4x gave no reading */
            if (temp == 0) {
                temp = dps_sample.temperature_c;
                lv_label_set_text_fmt(ctx->lbl_temp, "%.1f C", (double)temp);
                lv_bar_set_value(ctx->bar_temp, (int)temp, LV_ANIM_ON);
            }
        }
    }

    if (ctx->has_prev) {
        lv_label_set_text(ctx->lbl_temp_trend, trend_arrow(temp, ctx->prev_temp));
        lv_label_set_text(ctx->lbl_hum_trend, trend_arrow(hum, ctx->prev_hum));
        lv_label_set_text(ctx->lbl_press_trend, trend_arrow(press, ctx->prev_press));
    }
    ctx->prev_temp = temp;
    ctx->prev_hum  = hum;
    ctx->prev_press = press;
    ctx->has_prev = true;

    lv_label_set_text(ctx->lbl_comfort, comfort_zone(temp, hum));
    lv_obj_set_style_text_color(ctx->lbl_comfort, comfort_color(temp, hum), 0);
}

static lv_obj_t *make_env_card(lv_obj_t *parent, const char *title,
                                lv_color_t accent, env_ctx_t *ctx,
                                lv_obj_t **out_val, lv_obj_t **out_trend,
                                lv_obj_t **out_bar, int bar_min, int bar_max)
{
    lv_obj_t *card = example_card_create(parent, 240, 200,
                                         UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    example_label_create(card, title, &lv_font_montserrat_14, accent);
    *out_val = example_label_create(card, "--", &lv_font_montserrat_28,
                                    UI_COLOR_TEXT);
    *out_trend = example_label_create(card, "=", &lv_font_montserrat_16,
                                      UI_COLOR_TEXT_DIM);

    *out_bar = lv_bar_create(card);
    lv_obj_set_size(*out_bar, 200, 12);
    lv_bar_set_range(*out_bar, bar_min, bar_max);
    lv_bar_set_value(*out_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_color(*out_bar, accent, LV_PART_INDICATOR);

    return card;
}

void example_main(lv_obj_t *parent)
{
    static env_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize sensors */
    dps368_reader_init(&sensor_i2c_controller_hal_obj);
    sht4x_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 10, 0);

    example_label_create(parent, "Environmental Monitor",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    example_label_create(parent,
        "ตรวจวัดสิ่งแวดล้อม",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    /* Cards row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 770, 220);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    make_env_card(row, "Temperature", UI_COLOR_SHT40,
                  &ctx, &ctx.lbl_temp, &ctx.lbl_temp_trend,
                  &ctx.bar_temp, 0, 50);
    make_env_card(row, "Humidity", UI_COLOR_INFO,
                  &ctx, &ctx.lbl_hum, &ctx.lbl_hum_trend,
                  &ctx.bar_hum, 0, 100);
    make_env_card(row, "Pressure", UI_COLOR_DPS368,
                  &ctx, &ctx.lbl_press, &ctx.lbl_press_trend,
                  &ctx.bar_press, 0, 100);

    /* Comfort zone */
    lv_obj_t *ccard = example_card_create(parent, 770, 60,
                                          UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(ccard, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ccard, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(ccard, 20, 0);
    lv_obj_set_style_pad_column(ccard, 12, 0);

    example_label_create(ccard, "Comfort:", &lv_font_montserrat_20,
                         UI_COLOR_TEXT_DIM);
    ctx.lbl_comfort = example_label_create(ccard, "--",
                                           &lv_font_montserrat_20,
                                           UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
