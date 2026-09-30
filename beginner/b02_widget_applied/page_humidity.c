/**
 * @file    page_humidity.c
 * @brief   SHT40 humidity displayed as a bar widget (0-100%)
 */

#include "pages.h"

#if BSP_HAS_SHT40

static lv_obj_t *s_bar;
static lv_obj_t *s_lbl_val;

static void humidity_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_sht40) return;

    float humidity = snap.sht40.humidity_x100 / 100.0f;

    lv_bar_set_value(s_bar, (int32_t)humidity, LV_ANIM_ON);
    lv_label_set_text_fmt(s_lbl_val, "%.1f%%", (double)humidity);

    /* Color based on comfort zone */
    lv_color_t color;
    if (humidity < 30.0f) {
        color = UI_COLOR_WARNING;
    } else if (humidity < 60.0f) {
        color = UI_COLOR_SUCCESS;
    } else {
        color = UI_COLOR_ERROR;
    }
    lv_obj_set_style_bg_color(s_bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(s_lbl_val, color, 0);
}

void page_humidity_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* Value label */
    s_lbl_val = example_label_create(parent, "--%",
                                     &lv_font_montserrat_28, UI_COLOR_SHT40);

    /* Bar widget 0-100% */
    s_bar = lv_bar_create(parent);
    lv_obj_set_size(s_bar, 350, 30);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2A3A5C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, UI_COLOR_SHT40, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 8, 0);
    lv_obj_set_style_radius(s_bar, 8, LV_PART_INDICATOR);

    /* Sensor hint */
    example_label_create(parent, "SHT40 Humidity Sensor",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    /* 1000 ms update timer */
    lv_timer_create(humidity_timer_cb, 1000, NULL);
}

#else /* !BSP_HAS_SHT40 */

void page_humidity_create(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "SHT40 not available on this board.\nThis example requires AI Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
