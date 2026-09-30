/**
 * @file    page_timer.c
 * @brief   Timer Update — Label counting up every second with Start/Stop
 *
 * Demonstrates lv_timer_create, lv_timer_pause, lv_timer_resume.
 */

#include "pages.h"

static lv_obj_t *s_timer_label;
static lv_obj_t *s_timer_btn_label;
static lv_timer_t *s_timer;
static int s_elapsed;
static bool s_running;

static void timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    s_elapsed++;

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "Elapsed: %ds", s_elapsed);
    lv_label_set_text(s_timer_label, buf);
}

static void toggle_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_running) {
        lv_timer_pause(s_timer);
        s_running = false;
        lv_label_set_text(s_timer_btn_label, LV_SYMBOL_PLAY " Start");
    } else {
        lv_timer_resume(s_timer);
        s_running = true;
        lv_label_set_text(s_timer_btn_label, LV_SYMBOL_PAUSE " Stop");
    }
}

static void reset_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_elapsed = 0;
    lv_label_set_text(s_timer_label, "Elapsed: 0s");

    if (s_running) {
        lv_timer_pause(s_timer);
        s_running = false;
        lv_label_set_text(s_timer_btn_label, LV_SYMBOL_PLAY " Start");
    }
}

void page_timer_create(lv_obj_t *parent)
{
    s_elapsed = 0;
    s_running = true;

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "A timer counts up every second.\n"
                                "Use Start/Stop and Reset buttons.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Timer display label */
    s_timer_label = lv_label_create(parent);
    lv_label_set_text(s_timer_label, "Elapsed: 0s");
    lv_obj_set_style_text_font(s_timer_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_timer_label, UI_COLOR_PRIMARY, 0);
    lv_obj_align(s_timer_label, LV_ALIGN_TOP_MID, 0, 60);

    /* Start/Stop button */
    lv_obj_t *btn_toggle = lv_btn_create(parent);
    lv_obj_set_size(btn_toggle, 160, 50);
    lv_obj_align(btn_toggle, LV_ALIGN_TOP_MID, -90, 130);
    lv_obj_set_style_bg_color(btn_toggle, UI_COLOR_WARNING, 0);
    lv_obj_set_style_radius(btn_toggle, 10, 0);

    s_timer_btn_label = lv_label_create(btn_toggle);
    lv_label_set_text(s_timer_btn_label, LV_SYMBOL_PAUSE " Stop");
    lv_obj_set_style_text_font(s_timer_btn_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_timer_btn_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_timer_btn_label);
    lv_obj_add_event_cb(btn_toggle, toggle_cb, LV_EVENT_CLICKED, NULL);

    /* Reset button */
    lv_obj_t *btn_reset = lv_btn_create(parent);
    lv_obj_set_size(btn_reset, 160, 50);
    lv_obj_align(btn_reset, LV_ALIGN_TOP_MID, 90, 130);
    lv_obj_set_style_bg_color(btn_reset, UI_COLOR_ERROR, 0);
    lv_obj_set_style_radius(btn_reset, 10, 0);

    lv_obj_t *lbl_reset = lv_label_create(btn_reset);
    lv_label_set_text(lbl_reset, LV_SYMBOL_TRASH " Reset");
    lv_obj_set_style_text_font(lbl_reset, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_reset, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_reset);
    lv_obj_add_event_cb(btn_reset, reset_cb, LV_EVENT_CLICKED, NULL);

    /* Create LVGL timer: 1000ms period, auto-start */
    s_timer = lv_timer_create(timer_cb, 1000, NULL);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "s_timer = lv_timer_create(cb, 1000, NULL);\n"
                                "lv_timer_pause(s_timer);\n"
                                "lv_timer_resume(s_timer);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 220);
}
