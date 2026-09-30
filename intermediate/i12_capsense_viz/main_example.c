/**
 * I12 - CapSense Visualization
 *
 * Visualizes CapSense button presses and slider position with animated
 * feedback. Eva Kit only (requires BSP_HAS_CAPSENSE).
 */
#include "pse84_common.h"

#if !BSP_HAS_CAPSENSE
#error "This example requires CapSense (Eva Kit only)"
#endif

#define UPDATE_MS  50

typedef struct {
    lv_obj_t *btn0_indicator, *btn1_indicator;
    lv_obj_t *lbl_btn0, *lbl_btn1;
    lv_obj_t *slider_bar;
    lv_obj_t *lbl_slider;
    lv_obj_t *lbl_events;
    uint32_t  btn0_count, btn1_count;
    bool      prev_btn0, prev_btn1;
} capsense_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    capsense_ctx_t *ctx = (capsense_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    bool b0 = snap.capsense.btn0_pressed;
    bool b1 = snap.capsense.btn1_pressed;
    int  slider = snap.capsense.slider;

    /* Button 0 */
    lv_obj_set_style_bg_color(ctx->btn0_indicator,
        b0 ? UI_COLOR_PRIMARY : lv_color_hex(0x1A3050), 0);
    lv_label_set_text(ctx->lbl_btn0, b0 ? "BTN0: PRESSED" : "BTN0: Released");
    lv_obj_set_style_text_color(ctx->lbl_btn0,
        b0 ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_DIM, 0);

    if (b0 && !ctx->prev_btn0) ctx->btn0_count++;
    ctx->prev_btn0 = b0;

    /* Button 1 */
    lv_obj_set_style_bg_color(ctx->btn1_indicator,
        b1 ? UI_COLOR_WARNING : lv_color_hex(0x1A3050), 0);
    lv_label_set_text(ctx->lbl_btn1, b1 ? "BTN1: PRESSED" : "BTN1: Released");
    lv_obj_set_style_text_color(ctx->lbl_btn1,
        b1 ? UI_COLOR_WARNING : UI_COLOR_TEXT_DIM, 0);

    if (b1 && !ctx->prev_btn1) ctx->btn1_count++;
    ctx->prev_btn1 = b1;

    /* Slider */
    lv_bar_set_value(ctx->slider_bar, slider, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx->lbl_slider, "Slider: %d%%", slider);

    /* Color slider bar based on position */
    lv_color_t sc;
    if (slider < 33) sc = UI_COLOR_INFO;
    else if (slider < 66) sc = UI_COLOR_SUCCESS;
    else sc = UI_COLOR_WARNING;
    lv_obj_set_style_bg_color(ctx->slider_bar, sc, LV_PART_INDICATOR);

    /* Event counters */
    lv_label_set_text_fmt(ctx->lbl_events,
        "BTN0 presses: %lu  |  BTN1 presses: %lu",
        (unsigned long)ctx->btn0_count,
        (unsigned long)ctx->btn1_count);
}

void example_main(lv_obj_t *parent)
{
    static capsense_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 16, 0);
    lv_obj_set_style_pad_row(parent, 12, 0);

    example_label_create(parent, "CapSense Visualization",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    /* แสดงผล CapSense */
    example_label_create(parent,
        "แสดงผล CapSense",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Buttons section */
    lv_obj_t *btn_row = lv_obj_create(parent);
    lv_obj_set_size(btn_row, 750, 150);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);

    /* BTN0 card */
    lv_obj_t *c0 = example_card_create(btn_row, 340, 130,
                                        UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(c0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c0, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(c0, 12, 0);

    ctx.btn0_indicator = lv_obj_create(c0);
    lv_obj_set_size(ctx.btn0_indicator, 80, 80);
    lv_obj_set_style_radius(ctx.btn0_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.btn0_indicator, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(ctx.btn0_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.btn0_indicator,
                                  UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(ctx.btn0_indicator, 3, 0);

    ctx.lbl_btn0 = example_label_create(c0, "BTN0: Released",
                                        &lv_font_montserrat_16,
                                        UI_COLOR_TEXT_DIM);

    /* BTN1 card */
    lv_obj_t *c1 = example_card_create(btn_row, 340, 130,
                                        UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(c1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c1, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(c1, 12, 0);

    ctx.btn1_indicator = lv_obj_create(c1);
    lv_obj_set_size(ctx.btn1_indicator, 80, 80);
    lv_obj_set_style_radius(ctx.btn1_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.btn1_indicator, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(ctx.btn1_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.btn1_indicator,
                                  UI_COLOR_WARNING, 0);
    lv_obj_set_style_border_width(ctx.btn1_indicator, 3, 0);

    ctx.lbl_btn1 = example_label_create(c1, "BTN1: Released",
                                        &lv_font_montserrat_16,
                                        UI_COLOR_TEXT_DIM);

    /* Slider section */
    lv_obj_t *sc = example_card_create(parent, 750, 100,
                                        UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(sc, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sc, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(sc, 12, 0);
    lv_obj_set_style_pad_row(sc, 8, 0);

    ctx.lbl_slider = example_label_create(sc, "Slider: 0%",
                                          &lv_font_montserrat_20,
                                          UI_COLOR_TEXT);

    ctx.slider_bar = lv_bar_create(sc);
    lv_obj_set_size(ctx.slider_bar, 700, 20);
    lv_bar_set_range(ctx.slider_bar, 0, 100);
    lv_bar_set_value(ctx.slider_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.slider_bar, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_color(ctx.slider_bar, UI_COLOR_INFO,
                              LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx.slider_bar, 10, 0);
    lv_obj_set_style_radius(ctx.slider_bar, 10, LV_PART_INDICATOR);

    /* Event counter */
    ctx.lbl_events = example_label_create(parent, "BTN0 presses: 0  |  BTN1 presses: 0",
                                          &lv_font_montserrat_14,
                                          UI_COLOR_TEXT_DIM);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
