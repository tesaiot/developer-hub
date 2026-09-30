/**
 * @file    page_imu_display.c
 * @brief   BMI270 accelerometer X/Y/Z live display with colored labels
 */

#include "pages.h"

#if BSP_HAS_BMI270

static lv_obj_t *s_lbl_x;
static lv_obj_t *s_lbl_y;
static lv_obj_t *s_lbl_z;

static void imu_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmi270) return;

    float ax = snap.bmi270.ax / 16384.0f;
    float ay = snap.bmi270.ay / 16384.0f;
    float az = snap.bmi270.az / 16384.0f;

    lv_label_set_text_fmt(s_lbl_x, "X: %+.2f g", (double)ax);
    lv_label_set_text_fmt(s_lbl_y, "Y: %+.2f g", (double)ay);
    lv_label_set_text_fmt(s_lbl_z, "Z: %+.2f g", (double)az);
}

void page_imu_display_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Sensor card */
    lv_obj_t *card = example_card_create(parent, 380, 200, lv_color_hex(0x1E3A5F));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 12, 0);

    s_lbl_x = example_label_create(card, "X: --",
                                   &lv_font_montserrat_24, UI_COLOR_ERROR);
    s_lbl_y = example_label_create(card, "Y: --",
                                   &lv_font_montserrat_24, UI_COLOR_SUCCESS);
    s_lbl_z = example_label_create(card, "Z: --",
                                   &lv_font_montserrat_24, UI_COLOR_INFO);

    /* Hint */
    example_label_create(parent, "BMI270 Accelerometer",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    /* 100 ms update timer */
    lv_timer_create(imu_timer_cb, 100, NULL);
}

#else /* !BSP_HAS_BMI270 */

void page_imu_display_create(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "BMI270 not available on this board.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
