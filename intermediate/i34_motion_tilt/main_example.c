/**
 * Motion Tilt Angles
 *
 * Computes roll and pitch from BMI270 accelerometer and displays a
 * bubble-level tilt indicator with numeric readouts. Phi-split layout.
 *
 * Adapted from production page_motion.c tilt angle calculation.
 */
#include "pse84_common.h"

#if !BSP_HAS_BMI270
#error "This example requires BMI270"
#endif

#define UPDATE_MS    100
#define AREA_SZ      300
#define DOT_R        12
#define RAD_TO_DEG   57.2958f
#define SCALE_G      (9.81f / 16384.0f)

typedef struct {
    lv_obj_t *dot;
    lv_obj_t *area;
    lv_obj_t *lbl_roll, *lbl_pitch;
    lv_obj_t *lbl_ax, *lbl_ay, *lbl_az;
} tilt_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    tilt_ctx_t *ctx = (tilt_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.bmi270_changed) return;

    float ax = (float)snap.bmi270.ax * SCALE_G;
    float ay = (float)snap.bmi270.ay * SCALE_G;
    float az = (float)snap.bmi270.az * SCALE_G;

    float roll  = atan2f(ay, az) * RAD_TO_DEG;
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;

    /* Map tilt angles to pixel offset (1 deg = ~1.5 px) */
    int dx = (int)(roll  * 1.5f);
    int dy = (int)(pitch * 1.5f);
    int max_off = (AREA_SZ / 2) - DOT_R - 4;
    if (dx >  max_off) dx =  max_off;
    if (dx < -max_off) dx = -max_off;
    if (dy >  max_off) dy =  max_off;
    if (dy < -max_off) dy = -max_off;

    lv_obj_set_pos(ctx->dot,
                   (AREA_SZ / 2) - DOT_R + dx,
                   (AREA_SZ / 2) - DOT_R - dy);

    /* Dot color based on tilt magnitude */
    float mag = sqrtf(roll * roll + pitch * pitch);
    if (mag < 5.0f)
        lv_obj_set_style_bg_color(ctx->dot, UI_COLOR_SUCCESS, 0);
    else if (mag < 20.0f)
        lv_obj_set_style_bg_color(ctx->dot, UI_COLOR_WARNING, 0);
    else
        lv_obj_set_style_bg_color(ctx->dot, UI_COLOR_ERROR, 0);

    /* Numeric readouts */
    lv_label_set_text_fmt(ctx->lbl_roll,  "Roll:   %+6.1f deg", (double)roll);
    lv_label_set_text_fmt(ctx->lbl_pitch, "Pitch:  %+6.1f deg", (double)pitch);
    lv_label_set_text_fmt(ctx->lbl_ax, "aX: %+.2f m/s2", (double)ax);
    lv_label_set_text_fmt(ctx->lbl_ay, "aY: %+.2f m/s2", (double)ay);
    lv_label_set_text_fmt(ctx->lbl_az, "aZ: %+.2f m/s2", (double)az);
}

void example_main(lv_obj_t *parent)
{
    static tilt_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_column(parent, 16, 0);

    /* ── Left: tilt visualization area ────────────────────── */
    ctx.area = lv_obj_create(parent);
    lv_obj_set_size(ctx.area, AREA_SZ, AREA_SZ);
    lv_obj_set_style_bg_color(ctx.area, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx.area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx.area, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_color(ctx.area, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_border_width(ctx.area, 2, 0);
    lv_obj_set_style_pad_all(ctx.area, 0, 0);
    lv_obj_clear_flag(ctx.area, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshair - horizontal */
    lv_obj_t *ch = lv_obj_create(ctx.area);
    lv_obj_set_size(ch, AREA_SZ - 20, 1);
    lv_obj_set_pos(ch, 10, AREA_SZ / 2);
    lv_obj_set_style_bg_color(ch, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(ch, LV_OPA_40, 0);
    lv_obj_set_style_border_width(ch, 0, 0);

    /* Crosshair - vertical */
    lv_obj_t *cv = lv_obj_create(ctx.area);
    lv_obj_set_size(cv, 1, AREA_SZ - 20);
    lv_obj_set_pos(cv, AREA_SZ / 2, 10);
    lv_obj_set_style_bg_color(cv, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(cv, LV_OPA_40, 0);
    lv_obj_set_style_border_width(cv, 0, 0);

    /* Grid rings at 10 and 30 degrees */
    int r10 = (int)(10 * 1.5f) * 2;
    lv_obj_t *ring1 = lv_obj_create(ctx.area);
    lv_obj_set_size(ring1, r10, r10);
    lv_obj_set_pos(ring1, (AREA_SZ - r10) / 2, (AREA_SZ - r10) / 2);
    lv_obj_set_style_bg_opa(ring1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring1, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ring1, 1, 0);
    lv_obj_set_style_radius(ring1, LV_RADIUS_CIRCLE, 0);

    int r30 = (int)(30 * 1.5f) * 2;
    lv_obj_t *ring2 = lv_obj_create(ctx.area);
    lv_obj_set_size(ring2, r30, r30);
    lv_obj_set_pos(ring2, (AREA_SZ - r30) / 2, (AREA_SZ - r30) / 2);
    lv_obj_set_style_bg_opa(ring2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring2, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ring2, 1, 0);
    lv_obj_set_style_radius(ring2, LV_RADIUS_CIRCLE, 0);

    /* Tilt dot */
    ctx.dot = lv_obj_create(ctx.area);
    lv_obj_set_size(ctx.dot, DOT_R * 2, DOT_R * 2);
    lv_obj_set_style_radius(ctx.dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.dot, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(ctx.dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.dot, lv_color_white(), 0);
    lv_obj_set_style_border_width(ctx.dot, 2, 0);
    lv_obj_set_pos(ctx.dot, (AREA_SZ / 2) - DOT_R, (AREA_SZ / 2) - DOT_R);

    /* ── Right: numeric values panel ──────────────────────── */
    lv_obj_t *info = lv_obj_create(parent);
    lv_obj_set_size(info, 160, AREA_SZ);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(info, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(info, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(info, 0, 0);
    lv_obj_set_style_pad_all(info, 14, 0);
    lv_obj_set_style_pad_row(info, 10, 0);

    example_label_create(info, "Tilt Angles",
                         &lv_font_montserrat_20, lv_color_hex(0x4CAF50));

    /* มุมเอียง */
    example_label_create(info, "มุมเอียง",
                         &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_roll  = example_label_create(info, "Roll:   --",
                                         &lv_font_montserrat_14, UI_COLOR_TEXT);
    ctx.lbl_pitch = example_label_create(info, "Pitch:  --",
                                         &lv_font_montserrat_14, UI_COLOR_TEXT);

    /* Separator */
    lv_obj_t *sep = lv_obj_create(info);
    lv_obj_set_size(sep, 120, 1);
    lv_obj_set_style_bg_color(sep, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_40, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    example_label_create(info, "Raw Accel",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    ctx.lbl_ax = example_label_create(info, "aX: --",
                                      &lv_font_montserrat_14, UI_COLOR_TEXT);
    ctx.lbl_ay = example_label_create(info, "aY: --",
                                      &lv_font_montserrat_14, UI_COLOR_TEXT);
    ctx.lbl_az = example_label_create(info, "aZ: --",
                                      &lv_font_montserrat_14, UI_COLOR_TEXT);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
