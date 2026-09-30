/**
 * @file    page_bar_chart.c
 * @brief   Bar chart with 8 random bars and a Refresh button
 */

#include "pages.h"

#define BAR_COUNT 8

static lv_obj_t     *s_chart;
static lv_chart_series_t *s_series;

static void randomize_data(void)
{
    for (int i = 0; i < BAR_COUNT; i++) {
        lv_coord_t val = 10 + (lv_tick_get() * (i + 7)) % 91;  /* 10-100 */
        lv_chart_set_value_by_id(s_chart, s_series, i, val);
    }
    lv_chart_refresh(s_chart);
}

static void refresh_cb(lv_event_t *e)
{
    (void)e;
    randomize_data();
}

void page_bar_chart_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 380, 250);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(s_chart, BAR_COUNT);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_bg_color(s_chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_chart, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_radius(s_chart, UI_CARD_RADIUS, 0);

    /* Series */
    s_series = lv_chart_add_series(s_chart, UI_COLOR_INFO,
                                   LV_CHART_AXIS_PRIMARY_Y);

    /* Initial random data */
    randomize_data();

    /* Refresh button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 50);
    lv_obj_set_style_bg_color(btn, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_add_event_cb(btn, refresh_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
}
