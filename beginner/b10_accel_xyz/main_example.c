/**
 * @file    main_example.c
 * @brief   Accelerometer XYZ - BMI270 live accel via IPC sensorhub
 *
 * Reads BMI270 accelerometer data at 100ms intervals using
 * ipc_sensorhub_snapshot() and displays X, Y, Z with bar indicators.
 */

#include "pse84_common.h"

#if BSP_HAS_BMI270

typedef struct {
    lv_obj_t *lbl_x;
    lv_obj_t *lbl_y;
    lv_obj_t *lbl_z;
    lv_obj_t *bar_x;
    lv_obj_t *bar_y;
    lv_obj_t *bar_z;
} accel_ctx_t;

static accel_ctx_t ctx;

static void accel_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmi270) return;

    /* Convert raw int16 to g (divide by 16384) */
    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    lv_label_set_text_fmt(ctx.lbl_x, "X: %+.2f g", (double)ax);
    lv_label_set_text_fmt(ctx.lbl_y, "Y: %+.2f g", (double)ay);
    lv_label_set_text_fmt(ctx.lbl_z, "Z: %+.2f g", (double)az);

    /* Map [-2, +2]g to [0, 100] for bars */
    int32_t bx = (int32_t)((ax + 2.0f) * 25.0f);
    int32_t by = (int32_t)((ay + 2.0f) * 25.0f);
    int32_t bz = (int32_t)((az + 2.0f) * 25.0f);
    lv_bar_set_value(ctx.bar_x, LV_CLAMP(0, bx, 100), LV_ANIM_ON);
    lv_bar_set_value(ctx.bar_y, LV_CLAMP(0, by, 100), LV_ANIM_ON);
    lv_bar_set_value(ctx.bar_z, LV_CLAMP(0, bz, 100), LV_ANIM_ON);
}

static void create_axis_row(lv_obj_t *parent, const char *name, lv_palette_t color,
                             lv_obj_t **lbl_out, lv_obj_t **bar_out, int y_offset)
{
    *lbl_out = lv_label_create(parent);
    lv_label_set_text_fmt(*lbl_out, "%s: +0.00 g", name);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(*lbl_out, lv_palette_main(color), 0);
    lv_obj_align(*lbl_out, LV_ALIGN_LEFT_MID, 30, y_offset);

    *bar_out = lv_bar_create(parent);
    lv_obj_set_size(*bar_out, 300, 20);
    lv_bar_set_range(*bar_out, 0, 100);
    lv_bar_set_value(*bar_out, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(*bar_out, lv_palette_main(color), LV_PART_INDICATOR);
    lv_obj_align(*bar_out, LV_ALIGN_RIGHT_MID, -30, y_offset);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        LV_SYMBOL_REFRESH " Accelerometer (BMI270)",
        &lv_font_montserrat_20, UI_COLOR_BMI270);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* มาตรวัดความเร่ง XYZ */
    lv_obj_t *th_sub = example_label_create(parent,
        "มาตรวัดความเร่ง XYZ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Axis rows */
    create_axis_row(parent, "X", LV_PALETTE_RED,   &ctx.lbl_x, &ctx.bar_x, -40);
    create_axis_row(parent, "Y", LV_PALETTE_GREEN, &ctx.lbl_y, &ctx.bar_y,  10);
    create_axis_row(parent, "Z", LV_PALETTE_BLUE,  &ctx.lbl_z, &ctx.bar_z,  60);

    /* Timer: 100ms */
    lv_timer_create(accel_timer_cb, 100, NULL);
}

#else /* !BSP_HAS_BMI270 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "BMI270 not available on this board.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif /* BSP_HAS_BMI270 */
