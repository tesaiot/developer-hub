/**
 * I07 - Motion Detector (Practise)
 *
 * Detects motion from BMI270 accelerometer magnitude changes.
 * Shows alert status, motion count, and a scrollable event history log.
 *
 * Ported from Developer Hub — uses direct app_sensor driver instead of IPC.
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#include "bmi270/bmi270_reader.h"
#include "sensor_bus.h"

#include <math.h>

#define UPDATE_MS       50
#define MOTION_THRESH   0.3f   /* g difference threshold */
#define COOLDOWN_MS     1000
#define LOG_MAX_LINES   50

typedef struct {
    lv_obj_t *lbl_status;
    lv_obj_t *lbl_count;
    lv_obj_t *lbl_magnitude;
    lv_obj_t *log_area;
    lv_obj_t *indicator;
    float     prev_mag;
    uint32_t  motion_count;
    uint32_t  last_motion_tick;
    bool      first_sample;
} motion_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    motion_ctx_t *ctx = (motion_ctx_t *)lv_timer_get_user_data(t);
    bmi270_sample_t bmi_sample;
    if (!bmi270_reader_poll(&bmi_sample)) return;

    float ax = bmi_sample.acc_g_x;
    float ay = bmi_sample.acc_g_y;
    float az = bmi_sample.acc_g_z;
    float mag = sqrtf(ax * ax + ay * ay + az * az);

    lv_label_set_text_fmt(ctx->lbl_magnitude, "Mag: %.3f g", (double)mag);

    if (ctx->first_sample) {
        ctx->prev_mag = mag;
        ctx->first_sample = false;
        return;
    }

    float diff = mag - ctx->prev_mag;
    if (diff < 0) diff = -diff;
    ctx->prev_mag = mag;

    uint32_t now = lv_tick_get();

    if (diff > MOTION_THRESH &&
        (now - ctx->last_motion_tick) > COOLDOWN_MS) {
        ctx->motion_count++;
        ctx->last_motion_tick = now;

        /* Flash indicator */
        lv_obj_set_style_bg_color(ctx->indicator,
                                  UI_COLOR_ERROR, 0);
        lv_label_set_text(ctx->lbl_status, "MOTION DETECTED!");
        lv_obj_set_style_text_color(ctx->lbl_status,
                                    UI_COLOR_ERROR, 0);

        lv_label_set_text_fmt(ctx->lbl_count, "Events: %lu",
                              (unsigned long)ctx->motion_count);

        /* Append to log */
        char entry[64];
        uint32_t sec = now / 1000;
        snprintf(entry, sizeof(entry), "[%lu.%03lus] Motion #%lu (delta=%.3f g)\n",
                 (unsigned long)(sec), (unsigned long)(now % 1000),
                 (unsigned long)ctx->motion_count, (double)diff);

        const char *old = lv_textarea_get_text(ctx->log_area);
        /* Limit log length - keep last portion */
        size_t old_len = strlen(old);
        if (old_len > 2000) {
            const char *trim = old + old_len - 1500;
            lv_textarea_set_text(ctx->log_area, trim);
        }
        lv_textarea_add_text(ctx->log_area, entry);
    } else {
        /* Calm state */
        lv_obj_set_style_bg_color(ctx->indicator,
                                  UI_COLOR_SUCCESS, 0);
        lv_label_set_text(ctx->lbl_status, "No motion");
        lv_obj_set_style_text_color(ctx->lbl_status,
                                    UI_COLOR_SUCCESS, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    static motion_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.first_sample = true;

    tesaiot_add_thai_support_badge();

    /* Initialize BMI270 sensor via direct I2C driver */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    /* Title */
    example_label_create(parent, "Motion Detector",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);
    example_label_create(parent,
        "ตรวจจับการเคลื่อนไหว",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Status row */
    lv_obj_t *srow = lv_obj_create(parent);
    lv_obj_set_size(srow, 770, 60);
    lv_obj_set_flex_flow(srow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(srow, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(srow, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(srow, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(srow, 10, 0);
    lv_obj_set_style_border_width(srow, 0, 0);
    lv_obj_set_style_pad_hor(srow, 16, 0);
    lv_obj_set_style_pad_column(srow, 16, 0);

    /* Indicator dot */
    ctx.indicator = lv_obj_create(srow);
    lv_obj_set_size(ctx.indicator, 24, 24);
    lv_obj_set_style_radius(ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.indicator, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx.indicator, 0, 0);

    ctx.lbl_status = example_label_create(srow, "Initializing...",
                                          &lv_font_montserrat_20,
                                          UI_COLOR_TEXT);
    ctx.lbl_count = example_label_create(srow, "Events: 0",
                                         &lv_font_montserrat_16,
                                         UI_COLOR_TEXT_DIM);
    ctx.lbl_magnitude = example_label_create(srow, "Mag: --",
                                             &lv_font_montserrat_14,
                                             UI_COLOR_TEXT_DIM);

    /* Event log */
    example_label_create(parent, "Event Log:",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);

    ctx.log_area = lv_textarea_create(parent);
    lv_obj_set_size(ctx.log_area, 770, 280);
    lv_textarea_set_text(ctx.log_area, "");
    lv_textarea_set_cursor_click_pos(ctx.log_area, false);
    lv_obj_set_style_bg_color(ctx.log_area, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(ctx.log_area, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(ctx.log_area, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(ctx.log_area, &lv_font_montserrat_14, 0);
    lv_obj_set_style_border_color(ctx.log_area,
                                  UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.log_area, 1, 0);
    lv_obj_set_style_radius(ctx.log_area, 8, 0);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
