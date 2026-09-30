/**
 * CapSense Visualization
 *
 * Visualizes CapSense button states with large circular indicators and
 * slider position with a progress bar. Eva Kit only (BSP_HAS_CAPSENSE).
 *
 * Adapted from production page_controls.c CapSense section.
 */
#include "pse84_common.h"

#if BSP_HAS_CAPSENSE

#define UPDATE_MS  100

typedef struct {
    lv_obj_t *ind0, *ind1;
    lv_obj_t *lbl0, *lbl1;
    lv_obj_t *slider_bar;
    lv_obj_t *lbl_slider;
} capsense_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    capsense_ctx_t *ctx = (capsense_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.capsense_changed) return;

    bool b0 = snap.capsense.btn0_pressed;
    bool b1 = snap.capsense.btn1_pressed;
    int  sl = snap.capsense.slider;

    /* Button 0 */
    lv_obj_set_style_bg_color(ctx->ind0,
        b0 ? UI_COLOR_SUCCESS : lv_color_hex(0x1A3050), 0);
    lv_label_set_text(ctx->lbl0, b0 ? "ACTIVE" : "IDLE");
    lv_obj_set_style_text_color(ctx->lbl0,
        b0 ? UI_COLOR_SUCCESS : UI_COLOR_TEXT_DIM, 0);

    /* Button 1 */
    lv_obj_set_style_bg_color(ctx->ind1,
        b1 ? UI_COLOR_SUCCESS : lv_color_hex(0x1A3050), 0);
    lv_label_set_text(ctx->lbl1, b1 ? "ACTIVE" : "IDLE");
    lv_obj_set_style_text_color(ctx->lbl1,
        b1 ? UI_COLOR_SUCCESS : UI_COLOR_TEXT_DIM, 0);

    /* Slider */
    lv_bar_set_value(ctx->slider_bar, sl, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx->lbl_slider, "%d %%", sl);
}

static lv_obj_t *make_btn_card(lv_obj_t *parent, const char *name,
                                capsense_ctx_t *ctx, lv_obj_t **ind,
                                lv_obj_t **lbl)
{
    lv_obj_t *card = example_card_create(parent, 200, 180, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 10, 0);

    example_label_create(card, name, &lv_font_montserrat_16, UI_COLOR_TEXT);

    *ind = lv_obj_create(card);
    lv_obj_set_size(*ind, 60, 60);
    lv_obj_set_style_radius(*ind, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(*ind, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_opa(*ind, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(*ind, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_border_width(*ind, 3, 0);

    *lbl = example_label_create(card, "IDLE", &lv_font_montserrat_16,
                                UI_COLOR_TEXT_DIM);
    return card;
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
    lv_obj_set_style_pad_row(parent, 14, 0);

    example_label_create(parent, "CapSense Controls",
                         &lv_font_montserrat_24, UI_COLOR_PRIMARY);
    /* ควบคุม CapSense */
    example_label_create(parent,
        "ควบคุม CapSense",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Button indicators row */
    lv_obj_t *btn_row = lv_obj_create(parent);
    lv_obj_set_size(btn_row, 450, 200);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);

    make_btn_card(btn_row, "BTN0", &ctx, &ctx.ind0, &ctx.lbl0);
    make_btn_card(btn_row, "BTN1", &ctx, &ctx.ind1, &ctx.lbl1);

    /* Slider section */
    lv_obj_t *scard = example_card_create(parent, 450, 120, UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(scard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scard, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scard, 10, 0);

    example_label_create(scard, "Slider Position",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    ctx.slider_bar = lv_bar_create(scard);
    lv_obj_set_size(ctx.slider_bar, 400, 20);
    lv_bar_set_range(ctx.slider_bar, 0, 100);
    lv_bar_set_value(ctx.slider_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.slider_bar, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_bg_color(ctx.slider_bar, lv_color_hex(0x00BCD4),
                              LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx.slider_bar, 10, 0);
    lv_obj_set_style_radius(ctx.slider_bar, 10, LV_PART_INDICATOR);

    ctx.lbl_slider = example_label_create(scard, "0 %",
                                          &lv_font_montserrat_28,
                                          UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}

#else /* !BSP_HAS_CAPSENSE */

void example_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    example_label_create(parent, "CapSense not available",
                         &lv_font_montserrat_20, UI_COLOR_ERROR);
    example_label_create(parent, "This example requires Eva Kit\n(BSP_HAS_CAPSENSE)",
                         &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
}

#endif
