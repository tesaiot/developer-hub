#include "ui_wifi_status_page.h"

#include "ui_menu_layout.h"
#include "wifi_connection_service.h"
#include "wifi_profile_store.h"

#include <stdio.h>

#define UI_WIFI_STATUS_LOG_ENABLE          (1)
#define UI_WIFI_STATUS_TIMER_MS            (400U)
#define UI_WIFI_PING_FAIL_WARN_THRESHOLD   (3U)

typedef struct {
    lv_obj_t *page;
    lv_obj_t *state_label;
    lv_obj_t *ssid_label;
    lv_obj_t *ip_label;
    lv_obj_t *internet_label;
    lv_obj_t *rssi_label;
    lv_obj_t *retry_label;
    lv_obj_t *result_label;
    lv_obj_t *hint_label;
    lv_obj_t *btn_connect;
    lv_obj_t *btn_disconnect;
    lv_obj_t *btn_retry;
    lv_timer_t *refresh_timer;
    bool startup_done;
} ui_wifi_status_ctx_t;

static ui_wifi_status_ctx_t s_ctx;

static void ui_wifi_status_log(const char *msg)
{
#if UI_WIFI_STATUS_LOG_ENABLE
    printf("[WIFI_UI] %s\r\n", msg);
#else
    LV_UNUSED(msg);
#endif
}

static void ui_wifi_status_set_hint(const char *text, uint32_t color_hex)
{
    if(s_ctx.hint_label == NULL) {
        return;
    }

    lv_label_set_text(s_ctx.hint_label, text);
    lv_obj_set_style_text_color(s_ctx.hint_label, lv_color_hex(color_hex), LV_PART_MAIN);
}

static void ui_wifi_status_set_btn_enabled(lv_obj_t *btn, bool enabled)
{
    if(btn == NULL) {
        return;
    }

    if(enabled) {
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
    }
}

static lv_obj_t *ui_wifi_status_create_button(lv_obj_t *parent,
                                              const char *text,
                                              uint32_t bg_hex,
                                              lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 150, 38);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_hex), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x60A5FA), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label);

    return btn;
}

static void ui_wifi_status_connect_btn_cb(lv_event_t *e)
{
    wifi_profile_data_t profile;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if(!wifi_profile_store_load(&profile)) {
        ui_wifi_status_set_hint("No saved profile in NVM. Go to Profile page and Save first.", 0xFDE68A);
        ui_wifi_status_log("CONNECT_SKIP no_profile");
        return;
    }

    if(!wifi_connection_service_connect_profile(&profile, true)) {
        ui_wifi_status_set_hint("Connect request failed. Check profile and retry.", 0xFCA5A5);
        ui_wifi_status_log("CONNECT_QUEUE_FAIL");
        return;
    }

    ui_wifi_status_set_hint("Connect request queued.", 0x93C5FD);
    ui_wifi_status_log("CONNECT request queued");
}

static void ui_wifi_status_disconnect_btn_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    wifi_connection_service_disconnect();
    ui_wifi_status_set_hint("Disconnect request queued.", 0x93C5FD);
    ui_wifi_status_log("DISCONNECT request queued");
}

static void ui_wifi_status_retry_btn_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    wifi_connection_service_retry_now();
    ui_wifi_status_set_hint("Retry request queued.", 0x93C5FD);
    ui_wifi_status_log("RETRY request queued");
}

/* Poll snapshot from service and refresh labels/buttons without blocking UI. */
static void ui_wifi_status_refresh_timer_cb(lv_timer_t *timer)
{
    wifi_connection_snapshot_t snapshot;

    LV_UNUSED(timer);

    if((s_ctx.state_label == NULL) || !wifi_connection_service_get_snapshot(&snapshot)) {
        return;
    }

    lv_label_set_text_fmt(s_ctx.state_label,
                          "State: %s",
                          wifi_connection_service_state_text(snapshot.state));

    lv_label_set_text_fmt(s_ctx.ssid_label,
                          "SSID: %s | Security: %s",
                          (snapshot.ssid[0] != '\0') ? snapshot.ssid : "-",
                          (snapshot.security[0] != '\0') ? snapshot.security : "-");

    lv_label_set_text_fmt(s_ctx.ip_label,
                          "IP: %s",
                          (snapshot.ip_addr[0] != '\0') ? snapshot.ip_addr : "-");

    lv_label_set_text_fmt(s_ctx.internet_label,
                          "Internet: %s | Ping Fail: %u",
                          snapshot.internet_ok ? "Online" : "No Internet",
                          (unsigned int)snapshot.ping_fail_streak);

    if(snapshot.internet_ok) {
        lv_obj_set_style_text_color(s_ctx.internet_label, lv_color_hex(0x86EFAC), LV_PART_MAIN);
    } else if(snapshot.ping_fail_streak >= UI_WIFI_PING_FAIL_WARN_THRESHOLD) {
        lv_obj_set_style_text_color(s_ctx.internet_label, lv_color_hex(0xFCA5A5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(s_ctx.internet_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    }

    lv_label_set_text_fmt(s_ctx.rssi_label,
                          "RSSI: %d dBm",
                          (int)snapshot.rssi);

    if(snapshot.retry_pending) {
        lv_label_set_text_fmt(s_ctx.retry_label,
                              "Reconnect: wait %lu ms (stage %u)",
                              (unsigned long)snapshot.retry_wait_ms,
                              (unsigned int)snapshot.retry_stage);
    } else {
        lv_label_set_text_fmt(s_ctx.retry_label,
                              "Reconnect: %s",
                              snapshot.reconnect_enabled ? "Enabled" : "Disabled");
    }

    lv_label_set_text_fmt(s_ctx.result_label,
                          "Last conn: 0x%08lx | Last ping: 0x%08lx | reason: %lu",
                          (unsigned long)snapshot.last_rslt,
                          (unsigned long)snapshot.last_ping_rslt,
                          (unsigned long)snapshot.disconnect_reason);

    ui_wifi_status_set_btn_enabled(s_ctx.btn_connect,
                                   snapshot.state != WIFI_CONN_STATE_CONNECTING);
    ui_wifi_status_set_btn_enabled(s_ctx.btn_disconnect,
                                   snapshot.state != WIFI_CONN_STATE_DISCONNECTING);
    ui_wifi_status_set_btn_enabled(s_ctx.btn_retry,
                                   snapshot.have_profile &&
                                   (snapshot.state != WIFI_CONN_STATE_CONNECTING) &&
                                   (snapshot.state != WIFI_CONN_STATE_DISCONNECTING));
}

lv_obj_t *ui_wifi_status_page_create(lv_obj_t *menu)
{
    lv_obj_t *section;
    lv_obj_t *panel;
    lv_obj_t *button_row;
    lv_obj_t *title;

    if(menu == NULL) {
        return NULL;
    }

    (void)memset(&s_ctx, 0, sizeof(s_ctx));

    s_ctx.page = lv_menu_page_create(menu, "Home");
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
    lv_obj_set_style_pad_all(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(panel);
    lv_label_set_text(title, "WiFi Manager - Runtime Status");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_MENU_PANEL_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);

    s_ctx.state_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.state_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.state_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_ctx.ssid_label = lv_label_create(panel);
    lv_obj_set_width(s_ctx.ssid_label, LV_PCT(100));
    lv_label_set_long_mode(s_ctx.ssid_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(s_ctx.ssid_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.ssid_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.ip_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.ip_label, lv_color_hex(0x86EFAC), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.ip_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.internet_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.internet_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.internet_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.rssi_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.rssi_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.rssi_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.retry_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.retry_label, lv_color_hex(0xA5B4FC), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.retry_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.result_label = lv_label_create(panel);
    lv_obj_set_style_text_color(s_ctx.result_label, lv_color_hex(UI_MENU_PANEL_TEXT_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.result_label, &lv_font_montserrat_14, LV_PART_MAIN);

    button_row = lv_obj_create(panel);
    lv_obj_set_size(button_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(button_row, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.btn_connect = ui_wifi_status_create_button(button_row,
                                                     LV_SYMBOL_PLAY " Connect Saved",
                                                     0x166534,
                                                     ui_wifi_status_connect_btn_cb);

    s_ctx.btn_disconnect = ui_wifi_status_create_button(button_row,
                                                        LV_SYMBOL_CLOSE " Disconnect",
                                                        0xB91C1C,
                                                        ui_wifi_status_disconnect_btn_cb);

    s_ctx.btn_retry = ui_wifi_status_create_button(button_row,
                                                   LV_SYMBOL_REFRESH " Retry",
                                                   0x1D4ED8,
                                                   ui_wifi_status_retry_btn_cb);

    s_ctx.hint_label = lv_label_create(panel);
    lv_obj_set_width(s_ctx.hint_label, LV_PCT(100));
    lv_label_set_long_mode(s_ctx.hint_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_ctx.hint_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.hint_label, &lv_font_montserrat_14, LV_PART_MAIN);

    ui_wifi_status_set_hint("Use Connect Saved to start using profile from NVM.", 0x93C5FD);

    s_ctx.refresh_timer = lv_timer_create(ui_wifi_status_refresh_timer_cb,
                                          UI_WIFI_STATUS_TIMER_MS,
                                          NULL);

    ui_wifi_status_refresh_timer_cb(NULL);

    return s_ctx.page;
}

/* Run startup auto-connect once after pages are created. */
void ui_wifi_status_page_startup_auto_connect(void)
{
    wifi_profile_data_t profile;

    if(s_ctx.startup_done) {
        return;
    }

    s_ctx.startup_done = true;

    if(!wifi_profile_store_load(&profile)) {
        ui_wifi_status_log("AUTO_CONNECT skip no profile");
        return;
    }

    if(!profile.auto_connect) {
        ui_wifi_status_log("AUTO_CONNECT disabled in profile");
        return;
    }

    if(!wifi_connection_service_connect_profile(&profile, false)) {
        ui_wifi_status_set_hint("Auto connect failed to queue. Press Connect Saved.", 0xFCA5A5);
        ui_wifi_status_log("AUTO_CONNECT queue fail");
        return;
    }

    ui_wifi_status_set_hint("Auto connect requested from saved profile.", 0x86EFAC);
    ui_wifi_status_log("AUTO_CONNECT requested");
}



