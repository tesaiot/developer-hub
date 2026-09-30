/**
 * @file    main_example.c
 * @brief   Potentiometer Gauge - Analog value via IPC sensorhub
 *
 * Reads potentiometer every 100ms using ipc_sensorhub_snapshot().
 * Displays as an arc gauge with percentage and raw value.
 * Eva Kit only (BSP_HAS_POTENTIOMETER).
 */

#include "pse84_common.h"

#if BSP_HAS_POTENTIOMETER

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_pct;
    lv_obj_t *bar;
} pot_ctx_t;

static pot_ctx_t ctx;

static void pot_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_pot) return;

    /* percent_x10: 0-1000 (0.0-100.0%) */
    int32_t pct = snap.pot.percent_x10 / 10;
    uint16_t raw = snap.pot.raw;

    lv_arc_set_value(ctx.arc, pct);
    lv_bar_set_value(ctx.bar, pct, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx.lbl_value, "%u", raw);
    lv_label_set_text_fmt(ctx.lbl_pct, "%"PRId32"%%", pct);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "Potentiometer (Eva Kit)", &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* ตัวต้านทานปรับค่าได้ */
    lv_obj_t *th_sub = example_label_create(parent,
        "ตัวต้านทานปรับค่าได้",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Arc gauge */
    ctx.arc = lv_arc_create(parent);
    lv_obj_set_size(ctx.arc, 200, 200);
    lv_arc_set_rotation(ctx.arc, 135);
    lv_arc_set_bg_angles(ctx.arc, 0, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ctx.arc, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 15, LV_PART_MAIN);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, -130, 0);

    /* Percentage inside arc */
    ctx.lbl_pct = example_label_create(parent, "0%",
                                        &lv_font_montserrat_28,
                                        lv_palette_main(LV_PALETTE_PURPLE));
    lv_obj_align(ctx.lbl_pct, LV_ALIGN_CENTER, -130, 0);

    /* Raw value */
    lv_obj_t *lbl_raw_title = example_label_create(parent, "Raw ADC Value:",
                                                    &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(lbl_raw_title, LV_ALIGN_CENTER, 130, -40);

    ctx.lbl_value = example_label_create(parent, "0",
                                          &lv_font_montserrat_24, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_value, LV_ALIGN_CENTER, 130, -10);

    /* Bar indicator */
    ctx.bar = lv_bar_create(parent);
    lv_obj_set_size(ctx.bar, 250, 20);
    lv_bar_set_range(ctx.bar, 0, 100);
    lv_bar_set_value(ctx.bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx.bar, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_INDICATOR);
    lv_obj_align(ctx.bar, LV_ALIGN_CENTER, 130, 30);

    /* Timer: 100ms */
    lv_timer_create(pot_timer_cb, 100, NULL);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "Potentiometer not available.\nThis example requires Eva Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
