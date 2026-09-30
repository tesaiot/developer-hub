/**
 * @file    main_example.c
 * @brief   Humidity Gauge - SHT40 humidity via IPC sensorhub
 *
 * Reads SHT40 humidity every 1s using ipc_sensorhub_snapshot().
 * Displays in an arc gauge with comfort level indicator.
 * AI Kit only (BSP_HAS_SHT40).
 */

#include "pse84_common.h"

#if BSP_HAS_SHT40

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_comfort;
} humidity_ctx_t;

static humidity_ctx_t ctx;

static const char *comfort_level(float rh)
{
    if (rh < 20.0f) return "Very Dry";
    if (rh < 30.0f) return "Dry";
    if (rh < 60.0f) return "Comfortable";
    if (rh < 80.0f) return "Humid";
    return "Very Humid";
}

static lv_color_t comfort_color(float rh)
{
    if (rh < 30.0f) return lv_palette_main(LV_PALETTE_BLUE);
    if (rh < 60.0f) return UI_COLOR_SUCCESS;
    return UI_COLOR_ERROR;
}

static void humidity_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_sht40) return;

    /* Convert humidity_x100 to percent */
    float humidity = snap.sht40.humidity_x100 / 100.0f;

    lv_label_set_text_fmt(ctx.lbl_value, "%.1f%%", (double)humidity);
    lv_arc_set_value(ctx.arc, (int32_t)humidity);

    lv_color_t color = comfort_color(humidity);
    lv_obj_set_style_arc_color(ctx.arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(ctx.lbl_value, color, 0);

    lv_label_set_text_fmt(ctx.lbl_comfort, "Comfort: %s", comfort_level(humidity));
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "Humidity (SHT40)", &lv_font_montserrat_20, UI_COLOR_SHT40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* ความชื้นสัมพัทธ์ */
    lv_obj_t *th_sub = example_label_create(parent,
        "ความชื้นสัมพัทธ์",
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
    lv_obj_set_style_arc_color(ctx.arc, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, -5);

    /* Center value */
    ctx.lbl_value = example_label_create(parent, "--%",
                                          &lv_font_montserrat_28, UI_COLOR_SHT40);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 0, -5);

    /* Comfort level */
    ctx.lbl_comfort = example_label_create(parent, "Comfort: --",
                                            &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_comfort, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Timer: 1000ms */
    lv_timer_create(humidity_timer_cb, 1000, NULL);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "SHT40 not available on this board.\nThis example requires AI Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
