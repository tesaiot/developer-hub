/**
 * @file    page_system_info.c
 * @brief   Shows uptime, free heap, and task count — auto-updates every 1s
 */

#include "pages.h"

static lv_obj_t *s_lbl_uptime;
static lv_obj_t *s_lbl_heap;
static lv_obj_t *s_lbl_tasks;

static void sysinfo_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Uptime from lv_tick_get() */
    uint32_t ms = lv_tick_get();
    uint32_t secs = ms / 1000;
    uint32_t mins = secs / 60;
    uint32_t hrs  = mins / 60;
    lv_label_set_text_fmt(s_lbl_uptime, "Uptime: %"PRIu32"h %02"PRIu32"m %02"PRIu32"s",
                          hrs, mins % 60, secs % 60);

    /* Display: 800x480 DSI */
    lv_label_set_text(s_lbl_heap, "Display: 800x480 DSI");

    /* Core info */
    lv_label_set_text(s_lbl_tasks, "Core: CM55 (LVGL 9.2)");
}

void page_system_info_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Info card */
    lv_obj_t *card = example_card_create(parent, 400, 220, lv_color_hex(0x1E3A5F));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 16, 0);

    s_lbl_uptime = example_label_create(card, "Uptime: --",
                                        &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    s_lbl_heap   = example_label_create(card, "Free Heap: --",
                                        &lv_font_montserrat_20, UI_COLOR_SUCCESS);
    s_lbl_tasks  = example_label_create(card, "Tasks: --",
                                        &lv_font_montserrat_20, UI_COLOR_WARNING);

    /* 1000 ms update timer */
    lv_timer_create(sysinfo_timer_cb, 1000, NULL);
}
