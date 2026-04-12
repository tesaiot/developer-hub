/**
 * I08 - Level Bubble (Practise)
 *
 * 2D bubble level using BMI270 accelerometer. A circle (bubble) moves
 * on a crosshair grid based on tilt angle.
 *
 * Ported from Developer Hub — uses direct app_sensor driver instead of IPC.
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#include "bmi270/bmi270_reader.h"
#include "sensor_bus.h"

#define UPDATE_MS    30
#define AREA_W       400
#define AREA_H       400
#define BUBBLE_R     20
#define SENSITIVITY  150.0f   /* pixels per g */

typedef struct {
    lv_obj_t *bubble;
    lv_obj_t *lbl_x, *lbl_y;
    lv_obj_t *lbl_level;
    lv_obj_t *area;
} level_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    level_ctx_t *ctx = (level_ctx_t *)lv_timer_get_user_data(t);
    bmi270_sample_t bmi_sample;
    if (!bmi270_reader_poll(&bmi_sample)) return;

    float ax = bmi_sample.acc_g_x;
    float ay = bmi_sample.acc_g_y;

    /* Map tilt to pixel offset from center */
    int dx = (int)(ax * SENSITIVITY);
    int dy = (int)(ay * SENSITIVITY);

    /* Clamp to area */
    int max_off = (AREA_W / 2) - BUBBLE_R - 4;
    if (dx >  max_off) dx =  max_off;
    if (dx < -max_off) dx = -max_off;
    if (dy >  max_off) dy =  max_off;
    if (dy < -max_off) dy = -max_off;

    /* Position bubble relative to area center */
    lv_obj_set_pos(ctx->bubble,
                   (AREA_W / 2) - BUBBLE_R + dx,
                   (AREA_H / 2) - BUBBLE_R + dy);

    lv_label_set_text_fmt(ctx->lbl_x, "X: %.3f g", (double)ax);
    lv_label_set_text_fmt(ctx->lbl_y, "Y: %.3f g", (double)ay);

    /* Level indicator */
    float tilt = (ax < 0 ? -ax : ax) + (ay < 0 ? -ay : ay);
    if (tilt < 0.02f) {
        lv_label_set_text(ctx->lbl_level, "LEVEL");
        lv_obj_set_style_text_color(ctx->lbl_level,
                                    UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_color(ctx->bubble,
                                  UI_COLOR_SUCCESS, 0);
    } else if (tilt < 0.1f) {
        lv_label_set_text(ctx->lbl_level, "Almost level");
        lv_obj_set_style_text_color(ctx->lbl_level,
                                    UI_COLOR_WARNING, 0);
        lv_obj_set_style_bg_color(ctx->bubble,
                                  UI_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(ctx->lbl_level, "Tilted");
        lv_obj_set_style_text_color(ctx->lbl_level,
                                    UI_COLOR_ERROR, 0);
        lv_obj_set_style_bg_color(ctx->bubble,
                                  UI_COLOR_ERROR, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    static level_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    tesaiot_add_thai_support_badge();

    /* Initialize BMI270 sensor via direct I2C driver */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_column(parent, 20, 0);

    /* Level area */
    ctx.area = lv_obj_create(parent);
    lv_obj_set_size(ctx.area, AREA_W, AREA_H);
    lv_obj_set_style_bg_color(ctx.area, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx.area, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(ctx.area, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.area, 2, 0);
    lv_obj_set_style_pad_all(ctx.area, 0, 0);
    lv_obj_clear_flag(ctx.area, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshair - horizontal */
    lv_obj_t *ch_h = lv_obj_create(ctx.area);
    lv_obj_set_size(ch_h, AREA_W - 20, 1);
    lv_obj_set_pos(ch_h, 10, AREA_H / 2);
    lv_obj_set_style_bg_color(ch_h, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(ch_h, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ch_h, 0, 0);

    /* Crosshair - vertical */
    lv_obj_t *ch_v = lv_obj_create(ctx.area);
    lv_obj_set_size(ch_v, 1, AREA_H - 20);
    lv_obj_set_pos(ch_v, AREA_W / 2, 10);
    lv_obj_set_style_bg_color(ch_v, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(ch_v, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ch_v, 0, 0);

    /* Inner ring */
    lv_obj_t *ring = lv_obj_create(ctx.area);
    lv_obj_set_size(ring, 100, 100);
    lv_obj_set_pos(ring, (AREA_W - 100) / 2, (AREA_H - 100) / 2);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ring, 1, 0);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);

    /* Bubble */
    ctx.bubble = lv_obj_create(ctx.area);
    lv_obj_set_size(ctx.bubble, BUBBLE_R * 2, BUBBLE_R * 2);
    lv_obj_set_style_radius(ctx.bubble, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.bubble, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(ctx.bubble, LV_OPA_80, 0);
    lv_obj_set_style_border_color(ctx.bubble, lv_color_white(), 0);
    lv_obj_set_style_border_width(ctx.bubble, 2, 0);
    lv_obj_set_pos(ctx.bubble,
                   (AREA_W / 2) - BUBBLE_R,
                   (AREA_H / 2) - BUBBLE_R);

    /* Info panel */
    lv_obj_t *info = lv_obj_create(parent);
    lv_obj_set_size(info, 300, 400);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(info, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(info, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info, 12, 0);
    lv_obj_set_style_border_width(info, 0, 0);
    lv_obj_set_style_pad_all(info, 20, 0);
    lv_obj_set_style_pad_row(info, 16, 0);

    example_label_create(info, "Bubble Level",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);

    example_label_create(info,
        "ระดับน้ำฟองอากาศ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_level = example_label_create(info, "Initializing...",
                                         &lv_font_montserrat_28,
                                         UI_COLOR_TEXT);

    ctx.lbl_x = example_label_create(info, "X: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_TEXT_DIM);
    ctx.lbl_y = example_label_create(info, "Y: --",
                                     &lv_font_montserrat_16,
                                     UI_COLOR_TEXT_DIM);

    example_label_create(info, "Place board flat\nto center bubble",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
