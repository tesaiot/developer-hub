/**
 * I13 - Potentiometer Control
 *
 * Potentiometer simultaneously controlling an arc gauge, bar indicator,
 * and numeric label. Eva Kit only (requires BSP_HAS_POTENTIOMETER).
 */
#include "pse84_common.h"

#if !BSP_HAS_POTENTIOMETER
#error "This example requires Potentiometer (Eva Kit only)"
#endif

#define UPDATE_MS  50

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *bar;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_raw;
    lv_obj_t *color_box;
} pot_ctx_t;

static lv_color_t percent_to_color(float pct)
{
    /* Gradient from blue (0%) through green (50%) to red (100%) */
    uint8_t r, g, b;
    if (pct < 50.0f) {
        float t = pct / 50.0f;
        r = 0;
        g = (uint8_t)(255 * t);
        b = (uint8_t)(255 * (1.0f - t));
    } else {
        float t = (pct - 50.0f) / 50.0f;
        r = (uint8_t)(255 * t);
        g = (uint8_t)(255 * (1.0f - t));
        b = 0;
    }
    return lv_color_make(r, g, b);
}

static void timer_cb(lv_timer_t *t)
{
    pot_ctx_t *ctx = (pot_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float pct = snap.pot.percent_x10 / 10.0f;
    int ipct = (int)pct;
    if (ipct < 0) ipct = 0;
    if (ipct > 100) ipct = 100;

    lv_arc_set_value(ctx->arc, ipct);
    lv_bar_set_value(ctx->bar, ipct, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx->lbl_value, "%.1f%%", (double)pct);
    lv_label_set_text_fmt(ctx->lbl_raw, "Raw: %u", snap.pot.percent_x10);

    /* Update arc color */
    lv_color_t c = percent_to_color(pct);
    lv_obj_set_style_arc_color(ctx->arc, c, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ctx->bar, c, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ctx->color_box, c, 0);
    lv_obj_set_style_text_color(ctx->lbl_value, c, 0);
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
    lv_obj_set_style_pad_row(parent, 12, 0);

    example_label_create(parent, "Potentiometer Control",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    /* ควบคุมด้วยโพเทนชิโอมิเตอร์ */
    example_label_create(parent,
        "ควบคุมด้วยโพเทนชิโอมิเตอร์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Main content row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 760, 300);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    /* Arc gauge */
    lv_obj_t *arc_card = example_card_create(row, 280, 280,
                                              UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(arc_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(arc_card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ctx.arc = lv_arc_create(arc_card);
    lv_obj_set_size(ctx.arc, 220, 220);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_arc_set_bg_angles(ctx.arc, 135, 405);
    lv_obj_set_style_arc_width(ctx.arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ctx.arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ctx.arc, lv_color_hex(0x1A3050), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx.arc, UI_COLOR_INFO,
                               LV_PART_INDICATOR);
    lv_obj_clear_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(ctx.arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(ctx.arc, 0, LV_PART_KNOB);

    ctx.lbl_value = lv_label_create(ctx.arc);
    lv_label_set_text(ctx.lbl_value, "0%");
    lv_obj_set_style_text_font(ctx.lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ctx.lbl_value,
                                UI_COLOR_TEXT, 0);
    lv_obj_center(ctx.lbl_value);

    /* Right panel */
    lv_obj_t *rpanel = example_card_create(row, 400, 280,
                                            UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(rpanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rpanel, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(rpanel, 16, 0);
    lv_obj_set_style_pad_row(rpanel, 16, 0);

    /* Bar */
    example_label_create(rpanel, "Linear", &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
    ctx.bar = lv_bar_create(rpanel);
    lv_obj_set_size(ctx.bar, 350, 24);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_color(ctx.bar, UI_COLOR_INFO,
                              LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx.bar, 12, 0);
    lv_obj_set_style_radius(ctx.bar, 12, LV_PART_INDICATOR);

    /* Color preview */
    example_label_create(rpanel, "Color Output", &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
    ctx.color_box = lv_obj_create(rpanel);
    lv_obj_set_size(ctx.color_box, 350, 60);
    lv_obj_set_style_radius(ctx.color_box, 8, 0);
    lv_obj_set_style_bg_color(ctx.color_box, UI_COLOR_INFO, 0);
    lv_obj_set_style_bg_opa(ctx.color_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx.color_box, 0, 0);

    /* Raw value */
    ctx.lbl_raw = example_label_create(rpanel, "Raw: 0",
                                       &lv_font_montserrat_14,
                                       UI_COLOR_TEXT_DIM);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
