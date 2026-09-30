/**
 * @file    main_example.c
 * @brief   Temperature Gauge - DPS368 temperature via IPC sensorhub
 *
 * Reads DPS368 temperature every 500ms using ipc_sensorhub_snapshot().
 * Displays in an arc gauge with min/max tracking.
 * AI Kit only (BSP_HAS_DPS368).
 */

#include "pse84_common.h"

#if BSP_HAS_DPS368

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_min_max;
    float     min_temp;
    float     max_temp;
    bool      first_reading;
} temp_ctx_t;

static temp_ctx_t ctx;

static void temp_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_dps368) return;

    /* Convert temperature_x100 to float */
    float temperature = snap.dps368.temperature_x100 / 100.0f;

    lv_label_set_text_fmt(ctx.lbl_value, "%.1f \xC2\xB0""C", (double)temperature);

    int32_t arc_val = (int32_t)(temperature * 2.0f);  /* 0-100 for 0-50C */
    lv_arc_set_value(ctx.arc, LV_CLAMP(0, arc_val, 100));

    /* Track min/max */
    if (ctx.first_reading) {
        ctx.min_temp = temperature;
        ctx.max_temp = temperature;
        ctx.first_reading = false;
    } else {
        if (temperature < ctx.min_temp) ctx.min_temp = temperature;
        if (temperature > ctx.max_temp) ctx.max_temp = temperature;
    }
    lv_label_set_text_fmt(ctx.lbl_min_max, "Min: %.1f  Max: %.1f",
                          (double)ctx.min_temp, (double)ctx.max_temp);
}

void example_main(lv_obj_t *parent)
{
    ctx.first_reading = true;

    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "Temperature (DPS368)", &lv_font_montserrat_20, UI_COLOR_DPS368);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* อุณหภูมิ */
    lv_obj_t *th_sub = example_label_create(parent,
        "อุณหภูมิ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 220, 220);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ctx.arc, UI_COLOR_DPS368, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, -5);

    /* Center value */
    ctx.lbl_value = example_label_create(parent, "-- \xC2\xB0""C",
                                          &lv_font_montserrat_28, UI_COLOR_DPS368);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -5);

    /* Min/Max */
    ctx.lbl_min_max = example_label_create(parent, "Min: --  Max: --",
                                            &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_min_max, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Timer: 500ms */
    lv_timer_create(temp_timer_cb, 500, NULL);
}

#else /* !BSP_HAS_DPS368 */

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "DPS368 not available on this board.\nThis example requires AI Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
