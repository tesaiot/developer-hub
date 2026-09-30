/**
 * @file    page_stopwatch.c
 * @brief   Start/Stop/Reset stopwatch showing MM:SS.CC
 */

#include "pages.h"

static lv_obj_t   *s_lbl_time;
static uint32_t    s_start_tick;
static uint32_t    s_elapsed_ms;
static bool        s_running;

static void update_display(void)
{
    uint32_t mins = s_elapsed_ms / 60000;
    uint32_t secs = (s_elapsed_ms % 60000) / 1000;
    uint32_t cs   = (s_elapsed_ms % 1000) / 10;
    lv_label_set_text_fmt(s_lbl_time, "%02"PRIu32":%02"PRIu32".%02"PRIu32,
                          mins, secs, cs);
}

static void timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_running) return;
    uint32_t now = lv_tick_get();
    s_elapsed_ms += (now - s_start_tick);
    s_start_tick = now;
    update_display();
}

static void start_cb(lv_event_t *e)
{
    (void)e;
    if (!s_running) {
        s_running = true;
        s_start_tick = lv_tick_get();
    }
}

static void stop_cb(lv_event_t *e)
{
    (void)e;
    if (s_running) {
        uint32_t now = lv_tick_get();
        s_elapsed_ms += (now - s_start_tick);
        s_running = false;
        update_display();
    }
}

static void reset_cb(lv_event_t *e)
{
    (void)e;
    s_running = false;
    s_elapsed_ms = 0;
    update_display();
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *text,
                           lv_color_t color, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    return btn;
}

void page_stopwatch_create(lv_obj_t *parent)
{
    s_running = false;
    s_elapsed_ms = 0;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    /* Time display */
    s_lbl_time = example_label_create(parent, "00:00.00",
                                      &lv_font_montserrat_28, UI_COLOR_PRIMARY);

    /* Button row */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 15, 0);

    make_btn(row, "Start", UI_COLOR_SUCCESS, start_cb);
    make_btn(row, "Stop",  UI_COLOR_WARNING, stop_cb);
    make_btn(row, "Reset", UI_COLOR_ERROR,   reset_cb);

    /* 50 ms timer for ~20 Hz update */
    lv_timer_create(timer_cb, 50, NULL);
}
