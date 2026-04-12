/**
 * A06 - Sensor Fusion (Practise)
 *
 * Combines accelerometer, gyroscope, and magnetometer data
 * for orientation display with complementary filter.
 * Shows roll/pitch/yaw, heading compass, and tilt visualization.
 *
 * Ported from Developer Hub — uses direct app_sensor drivers instead of IPC.
 * Sensors: BMI270 + BMM350 (9-DOF AHRS)
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"

#include "bmi270/bmi270_reader.h"
#include "bmm350/bmm350_reader.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#define REFRESH_MS      50
#define ALPHA           0.96f   /* complementary filter weight */
#define DEG2RAD         0.017453f
#define RAD2DEG         57.29578f
#define COMPASS_R       70
#define HORIZON_W       300
#define HORIZON_H       180

typedef struct {
    float roll;
    float pitch;
    float yaw;
    float heading;
} orientation_t;

typedef struct {
    lv_obj_t      *parent;
    lv_obj_t      *roll_label;
    lv_obj_t      *pitch_label;
    lv_obj_t      *yaw_label;
    lv_obj_t      *heading_label;
    lv_obj_t      *compass_needle;
    lv_obj_t      *compass_heading_lbl;
    lv_obj_t      *horizon_line;
    lv_obj_t      *ax_label;
    lv_obj_t      *ay_label;
    lv_obj_t      *az_label;
    lv_obj_t      *gx_label;
    lv_obj_t      *gy_label;
    lv_obj_t      *gz_label;
    lv_obj_t      *mx_label;
    lv_obj_t      *my_label;
    lv_obj_t      *mz_label;
    lv_obj_t      *tilt_indicator;
    lv_obj_t      *quality_bar;
    lv_obj_t      *quality_label;
    orientation_t  orient;
    bmi270_sample_t last_bmi;
    bmm350_sample_t last_bmm;
} app_ctx_t;

static app_ctx_t g_ctx;

static void update_orientation(app_ctx_t *ctx)
{
    float ax_g = ctx->last_bmi.acc_g_x;
    float ay_g = ctx->last_bmi.acc_g_y;
    float az_g = ctx->last_bmi.acc_g_z;
    float gx_dps = ctx->last_bmi.gyr_dps_x;
    float gy_dps = ctx->last_bmi.gyr_dps_y;

    /* Accel-based angles */
    float accel_roll  = atan2f(ay_g, az_g) * RAD2DEG;
    float accel_pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * RAD2DEG;

    /* Complementary filter */
    float dt = (float)REFRESH_MS / 1000.0f;
    ctx->orient.roll  = ALPHA * (ctx->orient.roll  + gx_dps * dt) + (1.0f - ALPHA) * accel_roll;
    ctx->orient.pitch = ALPHA * (ctx->orient.pitch + gy_dps * dt) + (1.0f - ALPHA) * accel_pitch;

    ctx->orient.heading = ctx->last_bmm.heading_deg;
    ctx->orient.yaw = ctx->orient.heading;
}

static void update_compass(app_ctx_t *ctx)
{
    float h_rad = ctx->orient.heading * DEG2RAD;
    lv_coord_t nx = (lv_coord_t)(sinf(h_rad) * (float)(COMPASS_R - 10));
    lv_coord_t ny = (lv_coord_t)(-cosf(h_rad) * (float)(COMPASS_R - 10));

    /* Move needle by positioning relative to center */
    lv_obj_set_pos(ctx->compass_needle,
                   COMPASS_R + nx - 3,
                   COMPASS_R + ny - 3);

    char hbuf[16];
    snprintf(hbuf, sizeof(hbuf), "%03.0f", (double)ctx->orient.heading);
    lv_label_set_text(ctx->compass_heading_lbl, hbuf);
}

static void update_horizon(app_ctx_t *ctx)
{
    /* Shift horizon line based on pitch, rotate via roll offset */
    lv_coord_t pitch_offset = (lv_coord_t)(ctx->orient.pitch * 1.5f);
    lv_coord_t roll_offset  = (lv_coord_t)(ctx->orient.roll * 0.5f);

    if (pitch_offset > HORIZON_H / 2 - 4) pitch_offset = HORIZON_H / 2 - 4;
    if (pitch_offset < -(HORIZON_H / 2 - 4)) pitch_offset = -(HORIZON_H / 2 - 4);

    lv_obj_set_y(ctx->horizon_line, HORIZON_H / 2 + pitch_offset - 2);
    lv_obj_set_x(ctx->horizon_line, roll_offset);

    /* Tilt indicator position */
    lv_coord_t tx = (lv_coord_t)(ctx->orient.roll * 0.8f);
    lv_coord_t ty = (lv_coord_t)(ctx->orient.pitch * 0.8f);
    if (tx > 40) tx = 40; if (tx < -40) tx = -40;
    if (ty > 40) ty = 40; if (ty < -40) ty = -40;
    lv_obj_align(ctx->tilt_indicator, LV_ALIGN_CENTER, tx, ty);
}

static void timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);

    /* Poll sensors directly */
    bmi270_reader_poll(&ctx->last_bmi);
    bmm350_reader_poll(&ctx->last_bmm);

    update_orientation(ctx);
    update_compass(ctx);
    update_horizon(ctx);

    char buf[32];
    snprintf(buf, sizeof(buf), "Roll:  %+6.1f", (double)ctx->orient.roll);
    lv_label_set_text(ctx->roll_label, buf);
    snprintf(buf, sizeof(buf), "Pitch: %+6.1f", (double)ctx->orient.pitch);
    lv_label_set_text(ctx->pitch_label, buf);
    snprintf(buf, sizeof(buf), "Yaw:   %+6.1f", (double)ctx->orient.yaw);
    lv_label_set_text(ctx->yaw_label, buf);
    snprintf(buf, sizeof(buf), "Heading: %05.1f", (double)ctx->orient.heading);
    lv_label_set_text(ctx->heading_label, buf);

    /* Raw BMI270 data */
    snprintf(buf, sizeof(buf), "aX: %+.2f g", (double)ctx->last_bmi.acc_g_x);
    lv_label_set_text(ctx->ax_label, buf);
    snprintf(buf, sizeof(buf), "aY: %+.2f g", (double)ctx->last_bmi.acc_g_y);
    lv_label_set_text(ctx->ay_label, buf);
    snprintf(buf, sizeof(buf), "aZ: %+.2f g", (double)ctx->last_bmi.acc_g_z);
    lv_label_set_text(ctx->az_label, buf);
    snprintf(buf, sizeof(buf), "gX: %+.1f /s", (double)ctx->last_bmi.gyr_dps_x);
    lv_label_set_text(ctx->gx_label, buf);
    snprintf(buf, sizeof(buf), "gY: %+.1f /s", (double)ctx->last_bmi.gyr_dps_y);
    lv_label_set_text(ctx->gy_label, buf);
    snprintf(buf, sizeof(buf), "gZ: %+.1f /s", (double)ctx->last_bmi.gyr_dps_z);
    lv_label_set_text(ctx->gz_label, buf);

    /* Raw BMM350 data */
    snprintf(buf, sizeof(buf), "mX: %+.1f uT", (double)ctx->last_bmm.x_ut);
    lv_label_set_text(ctx->mx_label, buf);
    snprintf(buf, sizeof(buf), "mY: %+.1f uT", (double)ctx->last_bmm.y_ut);
    lv_label_set_text(ctx->my_label, buf);
    snprintf(buf, sizeof(buf), "mZ: %+.1f uT", (double)ctx->last_bmm.z_ut);
    lv_label_set_text(ctx->mz_label, buf);

    /* Fusion quality estimate */
    float mag = sqrtf(ctx->orient.roll * ctx->orient.roll +
                      ctx->orient.pitch * ctx->orient.pitch);
    int quality = 100 - (int)(mag * 0.5f);
    if (quality < 10) quality = 10;
    if (quality > 100) quality = 100;
    lv_bar_set_value(ctx->quality_bar, quality, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d%%", quality);
    lv_label_set_text(ctx->quality_label, buf);
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    tesaiot_add_thai_support_badge();

    /* Initialize sensors */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);
    bmm350_reader_init(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_GPS " Sensor Fusion - 9-DOF AHRS");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 14, 8);

    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub,
        "ประมวลผลเซ็นเซอร์รวม");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(th_sub, 16, 30);

    /* Compass card (left) */
    lv_obj_t *comp_card = lv_obj_create(parent);
    lv_obj_set_size(comp_card, 180, 200);
    lv_obj_set_pos(comp_card, 10, 40);
    lv_obj_set_style_bg_color(comp_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(comp_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(comp_card, 12, 0);
    lv_obj_set_style_border_width(comp_card, 1, 0);
    lv_obj_set_style_border_color(comp_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_clear_flag(comp_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Compass circle */
    lv_obj_t *comp_bg = lv_obj_create(comp_card);
    lv_obj_set_size(comp_bg, COMPASS_R * 2, COMPASS_R * 2);
    lv_obj_align(comp_bg, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(comp_bg, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(comp_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(comp_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(comp_bg, 2, 0);
    lv_obj_set_style_border_color(comp_bg, UI_COLOR_PRIMARY, 0);
    lv_obj_clear_flag(comp_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* Cardinal labels */
    static const char *cardinals[] = {"N", "E", "S", "W"};
    static const lv_coord_t cx[] = {COMPASS_R - 6, COMPASS_R * 2 - 18, COMPASS_R - 6, 2};
    static const lv_coord_t cy[] = {4, COMPASS_R - 8, COMPASS_R * 2 - 22, COMPASS_R - 8};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *cl = lv_label_create(comp_bg);
        lv_label_set_text(cl, cardinals[i]);
        lv_obj_set_style_text_color(cl, (i == 0) ? UI_COLOR_ERROR : lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(cl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(cl, cx[i], cy[i]);
    }

    /* Needle (small dot that moves) */
    ctx->compass_needle = lv_obj_create(comp_bg);
    lv_obj_set_size(ctx->compass_needle, 8, 8);
    lv_obj_set_style_bg_color(ctx->compass_needle, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(ctx->compass_needle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->compass_needle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->compass_needle, 0, 0);
    lv_obj_clear_flag(ctx->compass_needle, LV_OBJ_FLAG_SCROLLABLE);

    ctx->compass_heading_lbl = lv_label_create(comp_card);
    lv_obj_set_style_text_color(ctx->compass_heading_lbl, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ctx->compass_heading_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->compass_heading_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_label_set_text(ctx->compass_heading_lbl, "000");

    /* Artificial horizon card (center) */
    lv_obj_t *hor_card = lv_obj_create(parent);
    lv_obj_set_size(hor_card, HORIZON_W + 20, HORIZON_H + 50);
    lv_obj_set_pos(hor_card, 200, 40);
    lv_obj_set_style_bg_color(hor_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(hor_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hor_card, 12, 0);
    lv_obj_set_style_border_width(hor_card, 1, 0);
    lv_obj_set_style_border_color(hor_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_clear_flag(hor_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hor_hdr = lv_label_create(hor_card);
    lv_label_set_text(hor_hdr, "ARTIFICIAL HORIZON");
    lv_obj_set_style_text_color(hor_hdr, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(hor_hdr, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(hor_hdr, 10, 4);

    lv_obj_t *hor_area = lv_obj_create(hor_card);
    lv_obj_set_size(hor_area, HORIZON_W, HORIZON_H);
    lv_obj_align(hor_area, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_color(hor_area, lv_color_hex(0x0a2463), 0);
    lv_obj_set_style_bg_opa(hor_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hor_area, 8, 0);
    lv_obj_set_style_border_width(hor_area, 0, 0);
    lv_obj_set_style_clip_corner(hor_area, true, 0);
    lv_obj_clear_flag(hor_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Ground half */
    ctx->horizon_line = lv_obj_create(hor_area);
    lv_obj_set_size(ctx->horizon_line, HORIZON_W + 40, HORIZON_H);
    lv_obj_set_pos(ctx->horizon_line, -20, HORIZON_H / 2);
    lv_obj_set_style_bg_color(ctx->horizon_line, lv_color_hex(0x4a3000), 0);
    lv_obj_set_style_bg_opa(ctx->horizon_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->horizon_line, 2, 0);
    lv_obj_set_style_border_color(ctx->horizon_line, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_border_side(ctx->horizon_line, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(ctx->horizon_line, 0, 0);
    lv_obj_clear_flag(ctx->horizon_line, LV_OBJ_FLAG_SCROLLABLE);

    /* Center crosshair */
    lv_obj_t *cross_h = lv_obj_create(hor_area);
    lv_obj_set_size(cross_h, 40, 2);
    lv_obj_align(cross_h, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(cross_h, UI_COLOR_WARNING, 0);
    lv_obj_set_style_bg_opa(cross_h, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cross_h, 0, 0);
    lv_obj_clear_flag(cross_h, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cross_v = lv_obj_create(hor_area);
    lv_obj_set_size(cross_v, 2, 40);
    lv_obj_align(cross_v, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(cross_v, UI_COLOR_WARNING, 0);
    lv_obj_set_style_bg_opa(cross_v, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cross_v, 0, 0);
    lv_obj_clear_flag(cross_v, LV_OBJ_FLAG_SCROLLABLE);

    /* Orientation values (right side) */
    lv_obj_t *orient_card = lv_obj_create(parent);
    lv_obj_set_size(orient_card, 260, 200);
    lv_obj_set_pos(orient_card, 530, 40);
    lv_obj_set_style_bg_color(orient_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(orient_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(orient_card, 12, 0);
    lv_obj_set_style_border_width(orient_card, 1, 0);
    lv_obj_set_style_border_color(orient_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(orient_card, 12, 0);
    lv_obj_clear_flag(orient_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *oh = lv_label_create(orient_card);
    lv_label_set_text(oh, "ORIENTATION");
    lv_obj_set_style_text_color(oh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(oh, &lv_font_montserrat_14, 0);

    ctx->roll_label = lv_label_create(orient_card);
    lv_obj_set_style_text_color(ctx->roll_label, UI_COLOR_BMI270, 0);
    lv_obj_set_style_text_font(ctx->roll_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->roll_label, 0, 26);

    ctx->pitch_label = lv_label_create(orient_card);
    lv_obj_set_style_text_color(ctx->pitch_label, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(ctx->pitch_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->pitch_label, 0, 50);

    ctx->yaw_label = lv_label_create(orient_card);
    lv_obj_set_style_text_color(ctx->yaw_label, UI_COLOR_SHT40, 0);
    lv_obj_set_style_text_font(ctx->yaw_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->yaw_label, 0, 74);

    ctx->heading_label = lv_label_create(orient_card);
    lv_obj_set_style_text_color(ctx->heading_label, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(ctx->heading_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->heading_label, 0, 98);

    /* Fusion quality */
    lv_obj_t *qh = lv_label_create(orient_card);
    lv_label_set_text(qh, "Fusion Quality:");
    lv_obj_set_style_text_color(qh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(qh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(qh, 0, 132);

    ctx->quality_bar = lv_bar_create(orient_card);
    lv_obj_set_size(ctx->quality_bar, 170, 12);
    lv_obj_set_pos(ctx->quality_bar, 0, 152);
    lv_bar_set_range(ctx->quality_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->quality_bar, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->quality_bar, UI_COLOR_SUCCESS, LV_PART_INDICATOR);

    ctx->quality_label = lv_label_create(orient_card);
    lv_obj_set_style_text_color(ctx->quality_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->quality_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->quality_label, 180, 150);

    /* Raw sensor data row (bottom) */
    lv_obj_t *raw_card = lv_obj_create(parent);
    lv_obj_set_size(raw_card, 780, 170);
    lv_obj_set_pos(raw_card, 10, 250);
    lv_obj_set_style_bg_color(raw_card, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(raw_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(raw_card, 12, 0);
    lv_obj_set_style_border_width(raw_card, 1, 0);
    lv_obj_set_style_border_color(raw_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(raw_card, 12, 0);
    lv_obj_clear_flag(raw_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Accel column */
    lv_obj_t *ah = lv_label_create(raw_card);
    lv_label_set_text(ah, "ACCELEROMETER");
    lv_obj_set_style_text_color(ah, UI_COLOR_BMI270, 0);
    lv_obj_set_style_text_font(ah, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ah, 0, 0);

    ctx->ax_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->ax_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->ax_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->ax_label, 0, 22);
    ctx->ay_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->ay_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->ay_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->ay_label, 0, 42);
    ctx->az_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->az_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->az_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->az_label, 0, 62);

    /* Gyro column */
    lv_obj_t *gh = lv_label_create(raw_card);
    lv_label_set_text(gh, "GYROSCOPE");
    lv_obj_set_style_text_color(gh, UI_COLOR_DPS368, 0);
    lv_obj_set_style_text_font(gh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(gh, 260, 0);

    ctx->gx_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->gx_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->gx_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->gx_label, 260, 22);
    ctx->gy_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->gy_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->gy_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->gy_label, 260, 42);
    ctx->gz_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->gz_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->gz_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->gz_label, 260, 62);

    /* Magnetometer column */
    lv_obj_t *mh = lv_label_create(raw_card);
    lv_label_set_text(mh, "MAGNETOMETER");
    lv_obj_set_style_text_color(mh, UI_COLOR_BMM350, 0);
    lv_obj_set_style_text_font(mh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(mh, 520, 0);

    ctx->mx_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->mx_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->mx_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->mx_label, 520, 22);
    ctx->my_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->my_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->my_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->my_label, 520, 42);
    ctx->mz_label = lv_label_create(raw_card);
    lv_obj_set_style_text_color(ctx->mz_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->mz_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->mz_label, 520, 62);

    /* Tilt indicator card */
    lv_obj_t *tilt_card = lv_obj_create(raw_card);
    lv_obj_set_size(tilt_card, 100, 100);
    lv_obj_set_pos(tilt_card, 640, 10);
    lv_obj_set_style_bg_color(tilt_card, lv_color_hex(0x1a2332), 0);
    lv_obj_set_style_bg_opa(tilt_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tilt_card, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(tilt_card, 1, 0);
    lv_obj_set_style_border_color(tilt_card, lv_color_hex(0x2a3a5c), 0);
    lv_obj_clear_flag(tilt_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->tilt_indicator = lv_obj_create(tilt_card);
    lv_obj_set_size(ctx->tilt_indicator, 12, 12);
    lv_obj_align(ctx->tilt_indicator, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->tilt_indicator, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(ctx->tilt_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->tilt_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->tilt_indicator, 0, 0);
    lv_obj_clear_flag(ctx->tilt_indicator, LV_OBJ_FLAG_SCROLLABLE);

    /* Set initial text */
    lv_label_set_text(ctx->roll_label, "Roll:  +0.0");
    lv_label_set_text(ctx->pitch_label, "Pitch: +0.0");
    lv_label_set_text(ctx->yaw_label, "Yaw:   +0.0");
    lv_label_set_text(ctx->heading_label, "Heading: 000.0");
    lv_label_set_text(ctx->ax_label, "aX: 0.00 g");
    lv_label_set_text(ctx->ay_label, "aY: 0.00 g");
    lv_label_set_text(ctx->az_label, "aZ: 0.00 g");
    lv_label_set_text(ctx->gx_label, "gX: 0.0 /s");
    lv_label_set_text(ctx->gy_label, "gY: 0.0 /s");
    lv_label_set_text(ctx->gz_label, "gZ: 0.0 /s");
    lv_label_set_text(ctx->mx_label, "mX: 0.0 uT");
    lv_label_set_text(ctx->my_label, "mY: 0.0 uT");
    lv_label_set_text(ctx->mz_label, "mZ: 0.0 uT");

    lv_timer_create(timer_cb, REFRESH_MS, ctx);
}
