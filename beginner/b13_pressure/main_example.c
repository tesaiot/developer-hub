/**
 * @file    main_example.c
 * @brief   Pressure Bar - DPS368 atmospheric pressure via IPC sensorhub
 *
 * Reads DPS368 pressure every 500ms using ipc_sensorhub_snapshot().
 * Displayed as a bar widget with numeric value and status.
 * AI Kit only (BSP_HAS_DPS368).
 */

#include "pse84_common.h"
#include <math.h>

#if BSP_HAS_DPS368

typedef struct {
    lv_obj_t *bar;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_altitude;
    lv_obj_t *lbl_status;
} pressure_ctx_t;

static pressure_ctx_t ctx;

static const char *pressure_status(float hpa)
{
    if (hpa < 980.0f)  return "Very Low";
    if (hpa < 1000.0f) return "Low";
    if (hpa < 1020.0f) return "Normal";
    if (hpa < 1040.0f) return "High";
    return "Very High";
}

static void pressure_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_dps368) return;

    /* Convert pressure_x100 to hPa */
    float pressure = snap.dps368.pressure_x100 / 100.0f;

    lv_label_set_text_fmt(ctx.lbl_value, "%.2f hPa", (double)pressure);

    /* Barometric altitude (ISA standard atmosphere) */
    float altitude = 44330.0f * (1.0f - powf(pressure / 1013.25f, 0.190295f));
    lv_label_set_text_fmt(ctx.lbl_altitude, "Altitude: %.0f m", (double)altitude);

    /* Map 950-1050 to 0-100 */
    int32_t bar_val = (int32_t)(pressure - 950.0f);
    lv_bar_set_value(ctx.bar, LV_CLAMP(0, bar_val, 100), LV_ANIM_ON);

    lv_label_set_text_fmt(ctx.lbl_status, "Status: %s", pressure_status(pressure));
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "Barometric Pressure (DPS368)", &lv_font_montserrat_20, UI_COLOR_DPS368);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* ความดันบรรยากาศ */
    lv_obj_t *th_sub = example_label_create(parent,
        "ความดันบรรยากาศ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Pressure value */
    ctx.lbl_value = example_label_create(parent, "---- hPa",
                                          &lv_font_montserrat_28, UI_COLOR_SUCCESS);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -60);

    /* Altitude estimate */
    ctx.lbl_altitude = example_label_create(parent, "Altitude: -- m",
                                             &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_altitude, LV_ALIGN_CENTER, 0, -30);

    /* Pressure bar */
    ctx.bar = lv_bar_create(parent);
    lv_obj_set_size(ctx.bar, 500, 30);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_align(ctx.bar, LV_ALIGN_CENTER, 0, 0);

    /* Range labels */
    lv_obj_t *lbl_lo = example_label_create(parent, "950",
                                             &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(lbl_lo, LV_ALIGN_CENTER, -260, 25);

    lv_obj_t *lbl_hi = example_label_create(parent, "1050",
                                             &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(lbl_hi, LV_ALIGN_CENTER, 255, 25);

    /* Status label */
    ctx.lbl_status = example_label_create(parent, "Status: --",
                                           &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_status, LV_ALIGN_CENTER, 0, 60);

    /* Timer: 500ms */
    lv_timer_create(pressure_timer_cb, 500, NULL);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "DPS368 not available on this board.\nThis example requires AI Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
