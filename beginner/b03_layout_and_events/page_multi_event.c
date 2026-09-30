/**
 * @file    page_multi_event.c
 * @brief   Multi Event — 4 event types logged from a single button
 *
 * Demonstrates registering PRESSED, RELEASED, CLICKED, and LONG_PRESSED
 * callbacks on the same object. Events are shown in a scrollable log.
 */

#include "pages.h"

static lv_obj_t *s_log_label;
static char s_log_buf[512];
static int s_log_len;
static int s_event_num;

static void append_log(const char *event_name)
{
    s_event_num++;
    int written = lv_snprintf(s_log_buf + s_log_len,
                              sizeof(s_log_buf) - s_log_len,
                              "%d. %s\n", s_event_num, event_name);
    if (written > 0 && (s_log_len + written) < (int)sizeof(s_log_buf)) {
        s_log_len += written;
    }
    lv_label_set_text(s_log_label, s_log_buf);
}

static void pressed_cb(lv_event_t *e)   { LV_UNUSED(e); append_log("PRESSED");      }
static void released_cb(lv_event_t *e)  { LV_UNUSED(e); append_log("RELEASED");     }
static void clicked_cb(lv_event_t *e)   { LV_UNUSED(e); append_log("CLICKED");      }
static void long_cb(lv_event_t *e)      { LV_UNUSED(e); append_log("LONG_PRESSED"); }

void page_multi_event_create(lv_obj_t *parent)
{
    s_log_buf[0] = '\0';
    s_log_len = 0;
    s_event_num = 0;

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Tap or hold the button.\n"
                                "Watch the event log below.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Event button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 220, 60);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x9C27B0), 0);
    lv_obj_set_style_radius(btn, 12, 0);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, LV_SYMBOL_BELL " Event Button");
    lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_btn);

    /* Register 4 event callbacks on the same button */
    lv_obj_add_event_cb(btn, pressed_cb,  LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, released_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn, clicked_cb,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, long_cb,     LV_EVENT_LONG_PRESSED, NULL);

    /* Log header */
    lv_obj_t *lbl_hdr = lv_label_create(parent);
    lv_label_set_text(lbl_hdr, "Event Log:");
    lv_obj_set_style_text_font(lbl_hdr, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_hdr, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_hdr, LV_ALIGN_TOP_LEFT, 5, 125);

    /* Scrollable log container */
    lv_obj_t *log_cont = lv_obj_create(parent);
    lv_obj_set_size(log_cont, 420, 400);
    lv_obj_align(log_cont, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_color(log_cont, lv_color_hex(0x0A1525), 0);
    lv_obj_set_style_bg_opa(log_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(log_cont, 8, 0);
    lv_obj_set_style_border_width(log_cont, 1, 0);
    lv_obj_set_style_border_color(log_cont, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_pad_all(log_cont, 8, 0);
    lv_obj_set_scrollbar_mode(log_cont, LV_SCROLLBAR_MODE_AUTO);

    /* Log label */
    s_log_label = lv_label_create(log_cont);
    lv_label_set_text(s_log_label, "(no events yet)");
    lv_obj_set_style_text_font(s_log_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_log_label, UI_COLOR_TEXT, 0);
    lv_obj_set_width(s_log_label, 400);
    lv_label_set_long_mode(s_log_label, LV_LABEL_LONG_WRAP);
}
