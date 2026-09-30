/**
 * I20 - Automation Rules
 *
 * Rule engine UI: configure IF sensor > threshold THEN change indicator color.
 * Supports 4 configurable rules with real-time evaluation.
 */
#include "pse84_common.h"

#include <math.h>

#define UPDATE_MS     200
#define MAX_RULES     4

typedef enum {
    SOURCE_ACCEL_MAG = 0,
    SOURCE_TEMPERATURE,
    SOURCE_HUMIDITY,
    SOURCE_PRESSURE,
    SOURCE_COUNT
} rule_source_t;

typedef enum {
    OP_GREATER = 0,
    OP_LESS,
    OP_COUNT
} rule_op_t;

static const char *source_names[] = {"Accel(g)", "Temp(C)", "Humidity(%)", "Press(hPa)"};
static const char *op_names[] = {">", "<"};

typedef struct {
    rule_source_t source;
    rule_op_t     op;
    float         threshold;
    bool          active;
    bool          triggered;
} rule_t;

typedef struct {
    rule_t    rules[MAX_RULES];
    lv_obj_t *lbl_status[MAX_RULES];
    lv_obj_t *indicator[MAX_RULES];
    lv_obj_t *lbl_value[MAX_RULES];
    lv_obj_t *lbl_global;
} auto_ctx_t;

static float get_source_value(rule_source_t src, sensorhub_snapshot_t *snap)
{
    switch (src) {
#if BSP_HAS_BMI270
        case SOURCE_ACCEL_MAG: {
            float ax = snap->bmi270.ax / 16384.0f;
            float ay = snap->bmi270.ay / 16384.0f;
            float az = snap->bmi270.az / 16384.0f;
            return sqrtf(ax * ax + ay * ay + az * az);
        }
#endif
#if BSP_HAS_SHT40
        case SOURCE_TEMPERATURE:
            return snap->sht40.temperature_x100 / 100.0f;
        case SOURCE_HUMIDITY:
            return snap->sht40.humidity_x100 / 100.0f;
#elif BSP_HAS_DPS368
        case SOURCE_TEMPERATURE:
            return snap->dps368.temperature_x100 / 100.0f;
#endif
#if BSP_HAS_DPS368
        case SOURCE_PRESSURE:
            return snap->dps368.pressure_x100 / 100.0f;
#endif
        default:
            return 0.0f;
    }
}

static bool evaluate_rule(rule_t *r, float value)
{
    if (!r->active) return false;
    switch (r->op) {
        case OP_GREATER: return value > r->threshold;
        case OP_LESS:    return value < r->threshold;
        default:         return false;
    }
}

static void timer_cb(lv_timer_t *t)
{
    auto_ctx_t *ctx = (auto_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    int triggered_count = 0;

    for (int i = 0; i < MAX_RULES; i++) {
        rule_t *r = &ctx->rules[i];
        if (!r->active) {
            lv_label_set_text(ctx->lbl_status[i], "Disabled");
            lv_obj_set_style_text_color(ctx->lbl_status[i],
                                        UI_COLOR_TEXT_DIM, 0);
            lv_obj_set_style_bg_color(ctx->indicator[i],
                                      lv_color_hex(0x1A3050), 0);
            lv_label_set_text(ctx->lbl_value[i], "--");
            continue;
        }

        float val = get_source_value(r->source, &snap);
        bool fired = evaluate_rule(r, val);
        r->triggered = fired;

        lv_label_set_text_fmt(ctx->lbl_value[i], "%.2f", (double)val);

        if (fired) {
            triggered_count++;
            lv_label_set_text(ctx->lbl_status[i], "TRIGGERED");
            lv_obj_set_style_text_color(ctx->lbl_status[i],
                                        UI_COLOR_ERROR, 0);
            lv_obj_set_style_bg_color(ctx->indicator[i],
                                      UI_COLOR_ERROR, 0);
        } else {
            lv_label_set_text(ctx->lbl_status[i], "Normal");
            lv_obj_set_style_text_color(ctx->lbl_status[i],
                                        UI_COLOR_SUCCESS, 0);
            lv_obj_set_style_bg_color(ctx->indicator[i],
                                      UI_COLOR_SUCCESS, 0);
        }
    }

    if (triggered_count > 0) {
        lv_label_set_text_fmt(ctx->lbl_global, "ALERT: %d rule(s) triggered!",
                              triggered_count);
        lv_obj_set_style_text_color(ctx->lbl_global,
                                    UI_COLOR_ERROR, 0);
    } else {
        lv_label_set_text(ctx->lbl_global, "All rules OK");
        lv_obj_set_style_text_color(ctx->lbl_global,
                                    UI_COLOR_SUCCESS, 0);
    }
}

static lv_obj_t *make_rule_card(lv_obj_t *parent, int idx, auto_ctx_t *ctx,
                                 const char *desc)
{
    lv_obj_t *card = example_card_create(parent, 370, 110,
                                          UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(card, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 4, 0);

    /* Rule header */
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_size(hdr, lv_pct(100), 28);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    char title[16];
    snprintf(title, sizeof(title), "Rule %d", idx + 1);
    example_label_create(hdr, title, &lv_font_montserrat_16,
                         UI_COLOR_PRIMARY);

    ctx->indicator[idx] = lv_obj_create(hdr);
    lv_obj_set_size(ctx->indicator[idx], 14, 14);
    lv_obj_set_style_radius(ctx->indicator[idx], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx->indicator[idx],
                              lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(ctx->indicator[idx], LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->indicator[idx], 0, 0);

    /* Rule description */
    example_label_create(card, desc, &lv_font_montserrat_14,
                         UI_COLOR_TEXT);

    /* Status row */
    lv_obj_t *srow = lv_obj_create(card);
    lv_obj_set_size(srow, lv_pct(100), 24);
    lv_obj_set_flex_flow(srow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(srow, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(srow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(srow, 0, 0);
    lv_obj_set_style_pad_all(srow, 0, 0);

    ctx->lbl_value[idx] = example_label_create(srow, "--",
                                               &lv_font_montserrat_14,
                                               UI_COLOR_TEXT_DIM);
    ctx->lbl_status[idx] = example_label_create(srow, "Disabled",
                                                &lv_font_montserrat_14,
                                                UI_COLOR_TEXT_DIM);
    return card;
}

void example_main(lv_obj_t *parent)
{
    static auto_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Pre-configure 4 rules */
    /* Rule 1: Accel magnitude > 1.5g (motion) */
    ctx.rules[0] = (rule_t){SOURCE_ACCEL_MAG, OP_GREATER, 1.5f, true, false};
    /* Rule 2: Temperature > 35 C */
    ctx.rules[1] = (rule_t){SOURCE_TEMPERATURE, OP_GREATER, 35.0f, true, false};
    /* Rule 3: Humidity > 70% */
    ctx.rules[2] = (rule_t){SOURCE_HUMIDITY, OP_GREATER, 70.0f, true, false};
    /* Rule 4: Pressure < 990 hPa (low pressure/storm) */
    ctx.rules[3] = (rule_t){SOURCE_PRESSURE, OP_LESS, 990.0f, true, false};

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    /* Title */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 770, 44);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    example_label_create(hdr, "Automation Rules",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);

    /* กฎอัตโนมัติ */
    example_label_create(hdr, "กฎอัตโนมัติ",
                         &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_global = example_label_create(hdr, "Starting...",
                                          &lv_font_montserrat_16,
                                          UI_COLOR_TEXT_DIM);

    /* Rule cards - 2x2 grid */
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 770, 260);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);

    char desc_buf[64];

    for (int i = 0; i < MAX_RULES; i++) {
        rule_t *r = &ctx.rules[i];
        snprintf(desc_buf, sizeof(desc_buf), "IF %s %s %.1f",
                 source_names[r->source], op_names[r->op], (double)r->threshold);
        make_rule_card(grid, i, &ctx, desc_buf);
    }

    /* Legend */
    lv_obj_t *legend = example_card_create(parent, 770, 80,
                                            UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(legend, 10, 0);
    lv_obj_set_style_pad_row(legend, 4, 0);

    example_label_create(legend, "Rule Engine: evaluates conditions at 5 Hz",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
    example_label_create(legend,
        "Green = Normal | Red = Triggered | Shake board to trigger Rule 1",
        &lv_font_montserrat_14,
        UI_COLOR_TEXT_DIM);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
