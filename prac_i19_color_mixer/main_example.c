/**
 * I19 - Color Mixer
 *
 * Three sliders (R, G, B) that mix a color displayed in a large preview box.
 * Shows hex code and individual channel values.
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

typedef struct {
    lv_obj_t *slider_r, *slider_g, *slider_b;
    lv_obj_t *lbl_r, *lbl_g, *lbl_b;
    lv_obj_t *preview;
    lv_obj_t *lbl_hex;
} mixer_ctx_t;

static void update_preview(mixer_ctx_t *ctx)
{
    int r = (int)lv_slider_get_value(ctx->slider_r);
    int g = (int)lv_slider_get_value(ctx->slider_g);
    int b = (int)lv_slider_get_value(ctx->slider_b);

    lv_color_t c = lv_color_make(r, g, b);
    lv_obj_set_style_bg_color(ctx->preview, c, 0);

    lv_label_set_text_fmt(ctx->lbl_r, "R: %d", r);
    lv_label_set_text_fmt(ctx->lbl_g, "G: %d", g);
    lv_label_set_text_fmt(ctx->lbl_b, "B: %d", b);
    lv_label_set_text_fmt(ctx->lbl_hex, "#%02X%02X%02X", r, g, b);

    /* Determine text color for contrast */
    int lum = (r * 299 + g * 587 + b * 114) / 1000;
    lv_color_t txt_c = (lum > 128) ? lv_color_black() : lv_color_white();
    lv_obj_set_style_text_color(ctx->lbl_hex, txt_c, 0);
}

static void slider_cb(lv_event_t *e)
{
    mixer_ctx_t *ctx = (mixer_ctx_t *)lv_event_get_user_data(e);
    update_preview(ctx);
}

static lv_obj_t *make_color_slider(lv_obj_t *parent, const char *label,
                                     lv_color_t color, mixer_ctx_t *ctx,
                                     lv_obj_t **out_slider, lv_obj_t **out_lbl)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 340, 60);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 8, 0);

    *out_lbl = example_label_create(row, label, &lv_font_montserrat_14, color);
    lv_obj_set_width(*out_lbl, 50);

    *out_slider = lv_slider_create(row);
    lv_obj_set_size(*out_slider, 200, 16);
    lv_slider_set_range(*out_slider, 0, 255);
    lv_slider_set_value(*out_slider, 128, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(*out_slider, lv_color_hex(0x1A3050), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_slider, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(*out_slider, color, LV_PART_KNOB);
    lv_obj_set_style_pad_all(*out_slider, -2, LV_PART_KNOB);
    lv_obj_add_event_cb(*out_slider, slider_cb, LV_EVENT_VALUE_CHANGED, ctx);

    return row;
}

void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    static mixer_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 16, 0);
    lv_obj_set_style_pad_column(parent, 20, 0);

    /* Left: sliders */
    lv_obj_t *left = example_card_create(parent, 370, 380,
                                          UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_set_style_pad_row(left, 8, 0);

    example_label_create(left, "RGB Color Mixer",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);

    example_label_create(left, "ผสมสี RGB",
                         &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    make_color_slider(left, "R: 128", lv_color_hex(0xF44336),
                      &ctx, &ctx.slider_r, &ctx.lbl_r);
    make_color_slider(left, "G: 128", UI_COLOR_SUCCESS,
                      &ctx, &ctx.slider_g, &ctx.lbl_g);
    make_color_slider(left, "B: 128", UI_COLOR_INFO,
                      &ctx, &ctx.slider_b, &ctx.lbl_b);

    example_label_create(left, "Drag sliders to mix colors",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    /* Right: preview */
    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_size(right, 360, 380);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_row(right, 12, 0);

    ctx.preview = lv_obj_create(right);
    lv_obj_set_size(ctx.preview, 300, 300);
    lv_obj_set_style_radius(ctx.preview, 16, 0);
    lv_obj_set_style_bg_opa(ctx.preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.preview,
                                  UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.preview, 2, 0);
    lv_obj_clear_flag(ctx.preview, LV_OBJ_FLAG_SCROLLABLE);

    ctx.lbl_hex = lv_label_create(ctx.preview);
    lv_obj_set_style_text_font(ctx.lbl_hex, &lv_font_montserrat_28, 0);
    lv_obj_center(ctx.lbl_hex);

    /* Initial display */
    update_preview(&ctx);
}
