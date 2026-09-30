/**
 * @file    page_temperature.c
 * @brief   DPS368 or SHT40 temperature with arc gauge (0-50 C)
 */

#include "pages.h"

#if BSP_HAS_DPS368 || BSP_HAS_SHT40

static lv_obj_t *s_arc;
static lv_obj_t *s_lbl_val;

static void temp_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    float temperature = 0.0f;
    bool valid = false;

#if BSP_HAS_DPS368
    if (snap.has_dps368) {
        temperature = snap.dps368.temperature_x100 / 100.0f;
        valid = true;
    }
#endif
#if BSP_HAS_SHT40
    if (!valid && snap.has_sht40) {
        temperature = snap.sht40.temperature_x100 / 100.0f;
        valid = true;
    }
#endif

    if (!valid) return;

    lv_label_set_text_fmt(s_lbl_val, "%.1f \xC2\xB0""C", (double)temperature);

    /* Map 0-50 C to 0-100 arc range */
    int32_t arc_val = (int32_t)(temperature * 2.0f);
    lv_arc_set_value(s_arc, LV_CLAMP(0, arc_val, 100));
}

void page_temperature_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Arc gauge */
    s_arc = lv_arc_create(parent);
    lv_obj_set_size(s_arc, 200, 200);
    lv_arc_set_rotation(s_arc, 135);
    lv_arc_set_bg_angles(s_arc, 0, 270);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_value(s_arc, 0);
    lv_obj_remove_style(s_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(s_arc, UI_COLOR_DPS368, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 14, LV_PART_MAIN);

    /* Value label */
    s_lbl_val = example_label_create(parent, "-- \xC2\xB0""C",
                                     &lv_font_montserrat_28, UI_COLOR_DPS368);

    /* Sensor hint */
#if BSP_HAS_DPS368
    example_label_create(parent, "Source: DPS368",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
#else
    example_label_create(parent, "Source: SHT40",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
#endif

    /* 500 ms update timer */
    lv_timer_create(temp_timer_cb, 500, NULL);
}

#else /* no temperature sensor */

void page_temperature_create(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "No temperature sensor on this board.\nRequires DPS368 or SHT40.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
