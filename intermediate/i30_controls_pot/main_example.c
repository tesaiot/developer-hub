/**
 * Potentiometer Arc Gauge
 *
 * Large arc gauge showing potentiometer rotation with detail card
 * displaying raw ADC, voltage, and percentage. Eva Kit only
 * (BSP_HAS_POTENTIOMETER).
 *
 * Adapted from production page_controls.c potentiometer section.
 */
#include "pse84_common.h"

#if BSP_HAS_POTENTIOMETER

#define UPDATE_MS  100

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_pct;
    lv_obj_t *lbl_raw;
    lv_obj_t *lbl_voltage;
    lv_obj_t *lbl_pct_detail;
} pot_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    pot_ctx_t *ctx = (pot_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.pot_changed) return;

    uint16_t raw   = snap.pot.raw;
    int      px10  = snap.pot.percent_x10;
    float    pct   = px10 / 10.0f;
    float    volts = (float)raw * 3.3f / 65535.0f;

    /* Clamp arc value */
    int arc_val = px10;
    if (arc_val < 0) arc_val = 0;
    if (arc_val > 1000) arc_val = 1000;

    lv_arc_set_value(ctx->arc, arc_val);
    lv_label_set_text_fmt(ctx->lbl_pct, "%.1f%%", (double)pct);

    /* Detail card */
    lv_label_set_text_fmt(ctx->lbl_raw, "Raw ADC:  %u", (unsigned)raw);
    lv_label_set_text_fmt(ctx->lbl_voltage, "Voltage:  %.3f V", (double)volts);
    lv_label_set_text_fmt(ctx->lbl_pct_detail, "Position: %.1f %%", (double)pct);

    /* Arc color gradient: blue → cyan → green */
    lv_color_t c;
    if (pct < 50.0f)
        c = lv_color_mix(UI_COLOR_INFO, UI_COLOR_PRIMARY,
                          (uint8_t)(pct * 255 / 50));
    else
        c = lv_color_mix(UI_COLOR_PRIMARY, UI_COLOR_SUCCESS,
                          (uint8_t)((pct - 50.0f) * 255 / 50));
    lv_obj_set_style_arc_color(ctx->arc, c, LV_PART_INDICATOR);
}

void example_main(lv_obj_t *parent)
{
    static pot_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 16, 0);
    lv_obj_set_style_pad_row(parent, 14, 0);

    example_label_create(parent, "Potentiometer Gauge",
                         &lv_font_montserrat_24, UI_COLOR_PRIMARY);
    /* ควบคุมโพเทนชิโอมิเตอร์ */
    example_label_create(parent,
        "ควบคุมโพเทนชิโอมิเตอร์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Arc gauge card */
    lv_obj_t *arc_card = example_card_create(parent, 260, 260,
                                              UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(arc_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(arc_card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ctx.arc = lv_arc_create(arc_card);
    lv_obj_set_size(ctx.arc, 200, 200);
    lv_arc_set_range(ctx.arc, 0, 1000);
    lv_arc_set_value(ctx.arc, 0);
    lv_arc_set_bg_angles(ctx.arc, 135, 405);
    lv_obj_set_style_arc_width(ctx.arc, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ctx.arc, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ctx.arc, lv_color_hex(0x1A3050), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx.arc, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_clear_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(ctx.arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(ctx.arc, 0, LV_PART_KNOB);

    ctx.lbl_pct = lv_label_create(ctx.arc);
    lv_label_set_text(ctx.lbl_pct, "0.0%");
    lv_obj_set_style_text_font(ctx.lbl_pct, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_pct, UI_COLOR_TEXT, 0);
    lv_obj_center(ctx.lbl_pct);

    /* Detail card */
    lv_obj_t *detail = example_card_create(parent, 400, 140, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(detail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(detail, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(detail, 16, 0);
    lv_obj_set_style_pad_row(detail, 8, 0);

    ctx.lbl_raw = example_label_create(detail, "Raw ADC:  --",
                                       &lv_font_montserrat_16, UI_COLOR_TEXT);
    ctx.lbl_voltage = example_label_create(detail, "Voltage:  -- V",
                                           &lv_font_montserrat_16, UI_COLOR_TEXT);
    ctx.lbl_pct_detail = example_label_create(detail, "Position: -- %",
                                              &lv_font_montserrat_16, UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}

#else /* !BSP_HAS_POTENTIOMETER */

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    example_label_create(parent, "Potentiometer not available",
                         &lv_font_montserrat_20, UI_COLOR_ERROR);
    example_label_create(parent, "This example requires Eva Kit\n(BSP_HAS_POTENTIOMETER)",
                         &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
}

#endif
