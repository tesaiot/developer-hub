#include "ui_wifi_list_page.h"

#include "ui_menu_layout.h"
#include "wifi_scan_service.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define UI_WIFI_LOG_ENABLE         (1)
#define UI_WIFI_POLL_MS            (150U)
#define UI_WIFI_ROW_H              (35)

typedef struct {
    lv_obj_t *page;
    lv_obj_t *status_label;
    lv_obj_t *count_label;
    lv_obj_t *scan_btn;
    lv_obj_t *scan_btn_label;
    lv_obj_t *list_box;
    lv_obj_t *hint_label;
    lv_timer_t *poll_timer;
    wifi_scan_service_t service;
    uint16_t selected_idx;
} ui_wifi_list_ctx_t;

static ui_wifi_list_ctx_t s_ctx;

static void ui_wifi_log(const char *msg)
{
#if UI_WIFI_LOG_ENABLE
    printf("[WIFI_LIST] %s\r\n", msg);
#else
    LV_UNUSED(msg);
#endif
}

static void ui_wifi_update_status_labels(void)
{
    uint16_t count = 0;

    if((s_ctx.status_label == NULL) || (s_ctx.count_label == NULL)) {
        return;
    }

    (void)wifi_scan_service_get_list(&s_ctx.service, &count);

    if(s_ctx.service.scanning) {
        lv_label_set_text(s_ctx.status_label, "Status: Scanning...");
    } else if(s_ctx.service.scan_sequence == 0U) {
        lv_label_set_text(s_ctx.status_label, "Status: Idle");
    } else if(count == 0U) {
        lv_label_set_text(s_ctx.status_label, "Status: Scan complete (0 AP)");
    } else {
        lv_label_set_text(s_ctx.status_label, "Status: Scan complete");
    }

    lv_label_set_text_fmt(s_ctx.count_label,
                          "AP: %u | Seq: %u",
                          (unsigned int)count,
                          (unsigned int)s_ctx.service.scan_sequence);
}
static void ui_wifi_ap_item_click_cb(lv_event_t *e)
{
    uint32_t user_val;
    uint16_t count;
    const wifi_scan_ap_t *aps;
    uint16_t index;
    char log_buf[96];

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    user_val = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(user_val == 0U) {
        return;
    }

    index = (uint16_t)(user_val - 1U);
    aps = wifi_scan_service_get_list(&s_ctx.service, &count);

    if((aps == NULL) || (index >= count)) {
        return;
    }

    s_ctx.selected_idx = index;

    lv_label_set_text_fmt(s_ctx.hint_label,
                          "Selected: %s | RSSI %d dBm | %s",
                          aps[index].ssid,
                          (int)aps[index].rssi,
                          aps[index].security);

    (void)snprintf(log_buf,
                   sizeof(log_buf),
                   "SELECT ssid=%s rssi=%d sec=%s",
                   aps[index].ssid,
                   (int)aps[index].rssi,
                   aps[index].security);
    ui_wifi_log(log_buf);
}

static void ui_wifi_render_ap_list(void)
{
    uint16_t count;
    const wifi_scan_ap_t *aps;

    if(s_ctx.list_box == NULL) {
        return;
    }

    lv_obj_clean(s_ctx.list_box);

    aps = wifi_scan_service_get_list(&s_ctx.service, &count);

    if(s_ctx.service.scanning) {
        lv_obj_t *msg = lv_label_create(s_ctx.list_box);
        lv_label_set_text(msg, "Scanning nearby WiFi networks...");
        lv_obj_set_width(msg, LV_PCT(100));
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_color(msg, lv_color_hex(0x93C5FD), LV_PART_MAIN);
        return;
    }

    if((aps == NULL) || (count == 0U)) {
        lv_obj_t *msg = lv_label_create(s_ctx.list_box);
        lv_label_set_text(msg, "No AP list yet. Tap Scan to refresh.");
        lv_obj_set_width(msg, LV_PCT(100));
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_color(msg, lv_color_hex(0x94A3B8), LV_PART_MAIN);
        return;
    }

    for(uint16_t i = 0; i < count; i++) {
        lv_obj_t *row_btn = lv_button_create(s_ctx.list_box);
        lv_obj_set_size(row_btn, LV_PCT(100), UI_WIFI_ROW_H);
        lv_obj_set_style_radius(row_btn, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(row_btn,
                                  (i == s_ctx.selected_idx) ? lv_color_hex(0x1E3A8A) : lv_color_hex(0x1E293B),
                                  LV_PART_MAIN);
        lv_obj_set_style_border_color(row_btn,
                                      (i == s_ctx.selected_idx) ? lv_color_hex(0x60A5FA) : lv_color_hex(0x334155),
                                      LV_PART_MAIN);
        lv_obj_set_style_border_width(row_btn, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_left(row_btn, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_right(row_btn, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_top(row_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(row_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_clip_corner(row_btn, true, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(row_btn, 0, LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_set_style_outline_width(row_btn, 0, LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_set_style_translate_x(row_btn, 0, LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_set_style_translate_y(row_btn, 0, LV_PART_MAIN | LV_STATE_ANY);
        lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row_btn,
                            ui_wifi_ap_item_click_cb,
                            LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(i + 1U));

        lv_obj_t *row_label = lv_label_create(row_btn);
        lv_label_set_text_fmt(row_label,
                              "%s | %d dBm | %s",
                              aps[i].ssid,
                              (int)aps[i].rssi,
                              aps[i].security);
        lv_obj_set_width(row_label, LV_PCT(100));
        lv_label_set_long_mode(row_label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(row_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
        lv_obj_set_style_text_font(row_label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_align(row_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    }
}

static void ui_wifi_scan_click_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    LV_UNUSED(e);

    if(!wifi_scan_service_start(&s_ctx.service)) {
        if(s_ctx.service.scanning) {
            ui_wifi_log("SCAN_IGNORED busy");
            return;
        }

        ui_wifi_log("SCAN_START_FAIL");
        lv_label_set_text(s_ctx.hint_label, "Scan start failed. Check WiFi stack and retry.");
        ui_wifi_update_status_labels();
        return;
    }

    s_ctx.selected_idx = UINT16_MAX;
    lv_obj_add_state(s_ctx.scan_btn, LV_STATE_DISABLED);
    lv_label_set_text(s_ctx.hint_label, "Scanning... please wait");

    ui_wifi_update_status_labels();
    ui_wifi_render_ap_list();
}
static void ui_wifi_poll_timer_cb(lv_timer_t *timer)
{
    uint16_t count = 0U;

    LV_UNUSED(timer);

    if(wifi_scan_service_process(&s_ctx.service)) {
        lv_obj_clear_state(s_ctx.scan_btn, LV_STATE_DISABLED);
        (void)wifi_scan_service_get_list(&s_ctx.service, &count);

        if((s_ctx.service.scan_sequence == 0U) && (count == 0U)) {
            lv_label_set_text(s_ctx.hint_label, "Scan start failed. Tap Scan again.");
        } else if(count == 0U) {
            lv_label_set_text(s_ctx.hint_label, "No AP found. Tap Scan again.");
        } else {
            lv_label_set_text(s_ctx.hint_label, "Tap any AP row to inspect details.");
        }

        ui_wifi_update_status_labels();
        ui_wifi_render_ap_list();
    }
}
lv_obj_t *ui_wifi_list_page_create(lv_obj_t *menu)
{
    lv_obj_t *section;
    lv_obj_t *panel;
    lv_obj_t *header_row;
    lv_obj_t *scan_row;

    if(menu == NULL) {
        return NULL;
    }

    (void)memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.selected_idx = UINT16_MAX;

    s_ctx.page = lv_menu_page_create(menu, "WiFi Manager");
    lv_obj_set_style_pad_all(s_ctx.page, 0, LV_PART_MAIN);

    section = lv_menu_section_create(s_ctx.page);
    lv_obj_set_size(section, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(section, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 0, LV_PART_MAIN);
    lv_obj_set_flex_grow(section, 1);

    panel = lv_obj_create(section);
    lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_MENU_PANEL_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(UI_MENU_PANEL_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, UI_MENU_PANEL_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, UI_MENU_PANEL_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(panel);
    lv_label_set_text(title_label, "WiFi Scan List (EP05)");
    lv_obj_set_style_text_color(title_label, lv_color_hex(UI_MENU_PANEL_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, LV_PART_MAIN);

    header_row = lv_obj_create(panel);
    lv_obj_set_size(header_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(header_row, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.status_label = lv_label_create(header_row);
    lv_obj_set_flex_grow(s_ctx.status_label, 1);
    lv_obj_set_style_text_color(s_ctx.status_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_ctx.status_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    s_ctx.count_label = lv_label_create(header_row);
    lv_obj_set_style_text_color(s_ctx.count_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.count_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_ctx.count_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    scan_row = lv_obj_create(panel);
    lv_obj_set_size(scan_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(scan_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scan_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(scan_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scan_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(scan_row, 8, LV_PART_MAIN);
    lv_obj_clear_flag(scan_row, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.scan_btn = lv_button_create(scan_row);
    lv_obj_set_size(s_ctx.scan_btn, 150, 38);
    lv_obj_set_style_bg_color(s_ctx.scan_btn, lv_color_hex(0x1D4ED8), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ctx.scan_btn, lv_color_hex(0x60A5FA), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ctx.scan_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ctx.scan_btn, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(s_ctx.scan_btn, ui_wifi_scan_click_cb, LV_EVENT_CLICKED, NULL);

    s_ctx.scan_btn_label = lv_label_create(s_ctx.scan_btn);
    lv_label_set_text(s_ctx.scan_btn_label, LV_SYMBOL_REFRESH " Scan WiFi");
    lv_obj_set_style_text_color(s_ctx.scan_btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.scan_btn_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(s_ctx.scan_btn_label);

    s_ctx.list_box = lv_obj_create(panel);
    lv_obj_set_width(s_ctx.list_box, LV_PCT(100));
    lv_obj_set_flex_grow(s_ctx.list_box, 1);
    lv_obj_set_style_bg_color(s_ctx.list_box, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_ctx.list_box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ctx.list_box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ctx.list_box, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(s_ctx.list_box, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_ctx.list_box, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_ctx.list_box, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_ctx.list_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ctx.list_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_clip_corner(s_ctx.list_box, true, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(s_ctx.list_box, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(s_ctx.list_box, LV_DIR_VER);
    lv_obj_clear_flag(s_ctx.list_box, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(s_ctx.list_box, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(s_ctx.list_box, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_clear_flag(s_ctx.list_box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    s_ctx.hint_label = lv_label_create(panel);
    lv_label_set_text(s_ctx.hint_label, "Tap Scan to load AP list.");
    lv_obj_set_width(s_ctx.hint_label, LV_PCT(100));
    lv_label_set_long_mode(s_ctx.hint_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_ctx.hint_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ctx.hint_label, lv_color_hex(UI_MENU_PANEL_TEXT_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.hint_label, &lv_font_montserrat_14, LV_PART_MAIN);

    wifi_scan_service_init(&s_ctx.service);
    ui_wifi_update_status_labels();
    ui_wifi_render_ap_list();

    s_ctx.poll_timer = lv_timer_create(ui_wifi_poll_timer_cb, UI_WIFI_POLL_MS, NULL);

    return s_ctx.page;
}




