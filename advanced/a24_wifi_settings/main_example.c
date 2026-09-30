/*******************************************************************************
 * A24 - WiFi Settings (iOS-style Scan + Connect)
 *
 * Scan:    wifi_manager non-blocking (scan_start / scan_ready / scan_result).
 * Connect: wifi_manager_connect() — BLOCKING, deferred 50ms one-shot timer.
 *          Exact same pattern as production WiFi Settings page
 *          (in the firmware). Display freezes 10-35s during connect.
 *          Auto-retry once on failure (cy_wcm_connect_ap is flaky).
 *
 * Board:  PSoC Edge E84 (AI Kit / Eva Kit)
 * Core:   CM55 (display + UI), CM33_NS (WiFi)
 *******************************************************************************/

#include "pse84_common.h"
#include "wifi_manager.h"

/* ── Colors ───────────────────────────────────────────────────────── */
#define COLOR_BG          lv_color_hex(0xFFFFFF)
#define COLOR_CARD        lv_color_hex(0xF2F2F7)
#define COLOR_CARD_DARK   lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_DARK   lv_color_hex(0x1C1C1E)
#define COLOR_TEXT_GRAY   lv_color_hex(0x8E8E93)
#define COLOR_BLUE        lv_color_hex(0x007AFF)
#define COLOR_GREEN       lv_color_hex(0x34C759)
#define COLOR_SEP         lv_color_hex(0xD1D1D6)
#define COLOR_ROW_PRESS   lv_color_hex(0xE5E5EA)

/* ── Layout ───────────────────────────────────────────────────────── */
#define CARD_W            520
#define CARD_H            380
#define ROW_H             48
#define MAX_SCAN          WIFI_MGR_SCAN_MAX_ENTRIES
#define SSID_MAX_LEN      32
#define PASS_MAX_LEN      63

/* ── UI State Machine ─────────────────────────────────────────────── */
typedef enum {
    WS_SCANNING, WS_LIST, WS_PASSWORD,
    WS_CONNECTING, WS_CONNECTED, WS_ERROR,
} ws_state_t;

/* ── Module State ─────────────────────────────────────────────────── */
static struct {
    lv_obj_t *parent, *card, *title_label, *subtitle_label;
    lv_obj_t *spinner, *network_label, *list_cont;
    lv_obj_t *row_objs[MAX_SCAN];
    lv_obj_t *pass_cont, *pass_ta, *pass_ssid_label, *pass_btn;
    lv_obj_t *kb, *status_label, *time_label;
    lv_timer_t *poll_timer;

    wifi_mgr_scan_entry_t scan_results[MAX_SCAN];
    int scan_count, selected_idx;
    ws_state_t state;
    char connected_ssid[33];

    /* Deferred connect parameters (stashed before lv_timer fires) */
    char pending_ssid[33];
    char pending_pass[64];
} s_ws;

/* ── Forward Decl ─────────────────────────────────────────────────── */
static void ws_set_state(ws_state_t st);
static void ws_start_scan(void);
static void ws_show_list(void);
static void ws_show_password(int idx);
static void ws_send_connect(void);

/* ── Signal bars ──────────────────────────────────────────────────── */
static int rssi_to_bars(int8_t rssi)
{
    if (rssi > -50) return 3;
    if (rssi > -65) return 2;
    if (rssi > -80) return 1;
    return 0;
}

static lv_obj_t *create_signal_bars(lv_obj_t *parent, int bars)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 24, 20);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(cont, 2, 0);
    int heights[] = {6, 11, 16};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *bar = lv_obj_create(cont);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 5, heights[i]);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_bg_color(bar,
            (i < bars) ? COLOR_TEXT_DARK : COLOR_SEP, 0);
    }
    return cont;
}

/* ── Network Row ──────────────────────────────────────────────────── */
static void row_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < s_ws.scan_count)
        ws_show_password(idx);
}

static lv_obj_t *create_network_row(lv_obj_t *parent, int idx,
    const char *ssid, int8_t rssi, uint8_t security, bool connected)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), ROW_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(row, COLOR_CARD_DARK, 0);
    lv_obj_set_style_bg_color(row, COLOR_ROW_PRESS, LV_STATE_PRESSED);
    lv_obj_set_style_pad_left(row, 12, 0);
    lv_obj_set_style_pad_right(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED,
                         (void *)(intptr_t)idx);
    if (connected) {
        lv_obj_t *check = lv_label_create(row);
        lv_label_set_text(check, LV_SYMBOL_OK);
        lv_obj_set_style_text_color(check, COLOR_BLUE, 0);
        lv_obj_set_style_text_font(check, &lv_font_montserrat_16, 0);
        lv_obj_align(check, LV_ALIGN_LEFT_MID, 0, 0);
    }
    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, ssid);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
    if (connected) {
        lv_obj_set_style_text_color(name, COLOR_BLUE, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 28, 0);
    } else {
        lv_obj_set_style_text_color(name, COLOR_TEXT_DARK, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 8, 0);
    }
    if (security > 0) {
        lv_obj_t *lock = lv_label_create(row);
        lv_label_set_text(lock, LV_SYMBOL_EYE_CLOSE);
        lv_obj_set_style_text_color(lock, COLOR_TEXT_GRAY, 0);
        lv_obj_set_style_text_font(lock, &lv_font_montserrat_14, 0);
        lv_obj_align(lock, LV_ALIGN_RIGHT_MID, -30, 0);
    }
    lv_obj_t *sig = create_signal_bars(row, rssi_to_bars(rssi));
    lv_obj_align(sig, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *sep = lv_obj_create(row);
    lv_obj_remove_style_all(sep);
    lv_obj_set_size(sep, LV_PCT(95), 1);
    lv_obj_set_style_bg_color(sep, COLOR_SEP, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_align(sep, LV_ALIGN_BOTTOM_MID, 0, 0);
    return row;
}

/* ── Scan ─────────────────────────────────────────────────────────── */
static void ws_start_scan(void)
{
    ws_set_state(WS_SCANNING);
    s_ws.scan_count = 0;
    wifi_manager_scan_start();
}

static void ws_show_list(void)
{
    lv_obj_clean(s_ws.list_cont);
    if (s_ws.scan_count == 0) {
        lv_obj_t *e = lv_label_create(s_ws.list_cont);
        lv_label_set_text(e, "No networks found. Tap spinner to retry.");
        lv_obj_set_style_text_color(e, COLOR_TEXT_GRAY, 0);
        lv_obj_set_style_text_font(e, &lv_font_montserrat_14, 0);
        lv_obj_center(e);
    } else {
        for (int i = 0; i < s_ws.scan_count; i++) {
            bool is_conn = (s_ws.connected_ssid[0] != '\0' &&
                strcmp(s_ws.scan_results[i].ssid, s_ws.connected_ssid) == 0);
            s_ws.row_objs[i] = create_network_row(s_ws.list_cont, i,
                s_ws.scan_results[i].ssid, s_ws.scan_results[i].rssi,
                s_ws.scan_results[i].security, is_conn);
        }
    }
    ws_set_state(WS_LIST);
}

/* ── Password ─────────────────────────────────────────────────────── */
static void pass_connect_cb(lv_event_t *e) { (void)e; ws_send_connect(); }
static void pass_ta_ready_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_READY) ws_send_connect();
}

static void ws_show_password(int idx)
{
    s_ws.selected_idx = idx;
    ws_set_state(WS_PASSWORD);
    lv_label_set_text_fmt(s_ws.pass_ssid_label,
        "Enter password for \"%s\"", s_ws.scan_results[idx].ssid);
    lv_textarea_set_text(s_ws.pass_ta, "");
    lv_keyboard_set_textarea(s_ws.kb, s_ws.pass_ta);
    lv_obj_remove_flag(s_ws.kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ws.pass_cont, LV_OBJ_FLAG_HIDDEN);
}

/*******************************************************************************
 * BLOCKING connect — exact same pattern as the firmware's Wi-Fi page
 *
 * 1. Stash SSID/password
 * 2. Show "Connecting..." + force refresh
 * 3. Deferred 50ms one-shot timer
 * 4. wifi_manager_connect() — blocking, auto-retry once
 * 5. Update UI with result
 *
 * Display freezes 10-35s during connect. This is normal and matches
 * the built-in WiFi Settings page behavior.
 *******************************************************************************/
static void deferred_connect_cb(lv_timer_t *timer)
{
    lv_timer_delete(timer);

    /* Auto-retry: cy_wcm_connect_ap() often fails on the first attempt
     * after a scan due to internal WCM state transition. Retry once. */
    bool ok = wifi_manager_connect(s_ws.pending_ssid, s_ws.pending_pass);
    if (!ok) {
        ok = wifi_manager_connect(s_ws.pending_ssid, s_ws.pending_pass);
    }

    if (!ok) {
        s_ws.connected_ssid[0] = '\0';
        ws_set_state(WS_ERROR);
        return;
    }

    /* Success — store connected SSID and re-scan to show checkmark */
    strncpy(s_ws.connected_ssid, s_ws.pending_ssid, 32);
    s_ws.connected_ssid[32] = '\0';
    ws_set_state(WS_CONNECTED);
    ws_start_scan();
}

static void ws_send_connect(void)
{
    const char *ssid = s_ws.scan_results[s_ws.selected_idx].ssid;
    const char *pass = lv_textarea_get_text(s_ws.pass_ta);
    if (strlen(ssid) == 0 || strlen(ssid) > SSID_MAX_LEN) return;
    if (strlen(pass) > PASS_MAX_LEN) return;

    /* Stash parameters for deferred callback */
    strncpy(s_ws.pending_ssid, ssid, 32);
    s_ws.pending_ssid[32] = '\0';
    strncpy(s_ws.pending_pass, pass, 63);
    s_ws.pending_pass[63] = '\0';

    /* Hide keyboard + password */
    lv_obj_add_flag(s_ws.kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_ws.pass_cont, LV_OBJ_FLAG_HIDDEN);

    /* Show connecting state — LVGL renders this before blocking IPC */
    ws_set_state(WS_CONNECTING);

    /* Defer connect 50ms so LVGL can render "Connecting..." first.
     * This is the exact same pattern as the firmware's Wi-Fi page. */
    lv_timer_create(deferred_connect_cb, 50, NULL);
}

/* ── Set UI State ─────────────────────────────────────────────────── */
static void ws_set_state(ws_state_t st)
{
    s_ws.state = st;
    switch (st) {
    case WS_SCANNING:
        lv_obj_remove_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_ws.subtitle_label, "Scanning for networks...");
        lv_label_set_text(s_ws.status_label, "");
        lv_obj_set_style_text_color(s_ws.subtitle_label, COLOR_TEXT_GRAY, 0);
        break;
    case WS_LIST:
        lv_obj_add_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(s_ws.subtitle_label,
            "Select your Wi-Fi Network.  (%d found)", s_ws.scan_count);
        lv_obj_set_style_text_color(s_ws.subtitle_label, COLOR_TEXT_GRAY, 0);
        break;
    case WS_PASSWORD:
        lv_obj_add_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_ws.status_label, "");
        break;
    case WS_CONNECTING:
        lv_obj_remove_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(s_ws.subtitle_label,
            "Connecting to \"%s\"...\nDisplay will freeze. Please wait.",
            s_ws.pending_ssid);
        lv_obj_set_style_text_color(s_ws.subtitle_label, COLOR_TEXT_GRAY, 0);
        break;
    case WS_CONNECTED:
        lv_obj_add_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(s_ws.subtitle_label,
            LV_SYMBOL_OK " Connected to \"%s\"", s_ws.connected_ssid);
        lv_obj_set_style_text_color(s_ws.subtitle_label, COLOR_GREEN, 0);
        lv_label_set_text(s_ws.status_label, "");
        break;
    case WS_ERROR:
        lv_obj_add_flag(s_ws.spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_ws.subtitle_label,
            "Connection failed. Tap spinner to retry.");
        lv_obj_set_style_text_color(s_ws.subtitle_label,
            lv_color_hex(0xFF3B30), 0);
        break;
    }
}

/* ── Rescan ───────────────────────────────────────────────────────── */
static void rescan_cb(lv_event_t *e)
{
    (void)e;
    ws_start_scan();
}

/* ── Poll Timer (scan + clock) ────────────────────────────────────── */
static void poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_ws.state == WS_SCANNING && wifi_manager_scan_ready()) {
        s_ws.scan_count = wifi_manager_scan_result(s_ws.scan_results, MAX_SCAN);
        if (s_ws.scan_count < 0) s_ws.scan_count = 0;
        ws_show_list();
    }
    uint32_t ticks = xTaskGetTickCount();
    uint32_t sec = ticks / configTICK_RATE_HZ;
    lv_label_set_text_fmt(s_ws.time_label, "%02lu:%02lu:%02lu",
        (unsigned long)((sec/3600)%24), (unsigned long)((sec/60)%60),
        (unsigned long)(sec%60));
}

/* ── Entry Point ──────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    memset(&s_ws, 0, sizeof(s_ws));
    s_ws.parent = parent;
    s_ws.selected_idx = -1;

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Time label */
    s_ws.time_label = lv_label_create(parent);
    lv_label_set_text(s_ws.time_label, "");
    lv_obj_set_style_text_font(s_ws.time_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ws.time_label, COLOR_TEXT_GRAY, 0);
    lv_obj_align(s_ws.time_label, LV_ALIGN_TOP_RIGHT, -12, 6);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " Wi-Fi Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT_DARK, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* Thai */
    lv_obj_t *th = thai_label(parent, "ตั้งค่า WiFi", 14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th, LV_ALIGN_TOP_LEFT, 12, 8);

    /* Card */
    s_ws.card = lv_obj_create(parent);
    lv_obj_set_size(s_ws.card, CARD_W, CARD_H);
    lv_obj_align(s_ws.card, LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_style_bg_color(s_ws.card, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_ws.card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ws.card, 16, 0);
    lv_obj_set_style_border_width(s_ws.card, 0, 0);
    lv_obj_set_style_shadow_width(s_ws.card, 20, 0);
    lv_obj_set_style_shadow_color(s_ws.card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(s_ws.card, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(s_ws.card, 16, 0);
    lv_obj_clear_flag(s_ws.card, LV_OBJ_FLAG_SCROLLABLE);

    s_ws.title_label = lv_label_create(s_ws.card);
    lv_label_set_text(s_ws.title_label, "Wi-Fi Connection");
    lv_obj_set_style_text_font(s_ws.title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_ws.title_label, COLOR_TEXT_DARK, 0);
    lv_obj_align(s_ws.title_label, LV_ALIGN_TOP_MID, 0, 0);

    s_ws.subtitle_label = lv_label_create(s_ws.card);
    lv_label_set_text(s_ws.subtitle_label, "Select your Wi-Fi Network.");
    lv_obj_set_style_text_font(s_ws.subtitle_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ws.subtitle_label, COLOR_TEXT_GRAY, 0);
    lv_obj_align(s_ws.subtitle_label, LV_ALIGN_TOP_MID, 0, 30);

    s_ws.network_label = lv_label_create(s_ws.card);
    lv_label_set_text(s_ws.network_label, "Network");
    lv_obj_set_style_text_font(s_ws.network_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ws.network_label, COLOR_TEXT_DARK, 0);
    lv_obj_set_pos(s_ws.network_label, 16, 54);

    /* Spinner */
    s_ws.spinner = lv_spinner_create(s_ws.card);
    lv_obj_set_size(s_ws.spinner, 24, 24);
    lv_obj_set_pos(s_ws.spinner, CARD_W - 58, 52);
    lv_spinner_set_anim_params(s_ws.spinner, 1000, 300);
    lv_obj_set_style_arc_width(s_ws.spinner, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ws.spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ws.spinner, COLOR_SEP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ws.spinner, lv_color_hex(0x3C3C43), LV_PART_INDICATOR);
    lv_obj_add_flag(s_ws.spinner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_ws.spinner, rescan_cb, LV_EVENT_CLICKED, NULL);

    /* List */
    s_ws.list_cont = lv_obj_create(s_ws.card);
    lv_obj_set_size(s_ws.list_cont, CARD_W - 32, CARD_H - 140);
    lv_obj_set_pos(s_ws.list_cont, 0, 80);
    lv_obj_set_style_bg_color(s_ws.list_cont, COLOR_CARD_DARK, 0);
    lv_obj_set_style_bg_opa(s_ws.list_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ws.list_cont, 12, 0);
    lv_obj_set_style_border_width(s_ws.list_cont, 0, 0);
    lv_obj_set_style_pad_all(s_ws.list_cont, 0, 0);
    lv_obj_set_style_pad_row(s_ws.list_cont, 0, 0);
    lv_obj_set_flex_flow(s_ws.list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(s_ws.list_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_ws.list_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_ws.list_cont, LV_SCROLLBAR_MODE_AUTO);

    s_ws.status_label = lv_label_create(s_ws.card);
    lv_label_set_text(s_ws.status_label, "");
    lv_obj_set_style_text_font(s_ws.status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ws.status_label, COLOR_TEXT_GRAY, 0);
    lv_obj_align(s_ws.status_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* Password container */
    s_ws.pass_cont = lv_obj_create(parent);
    lv_obj_set_size(s_ws.pass_cont, CARD_W, 80);
    lv_obj_align(s_ws.pass_cont, LV_ALIGN_BOTTOM_MID, 0, -205);
    lv_obj_set_style_bg_color(s_ws.pass_cont, COLOR_CARD_DARK, 0);
    lv_obj_set_style_bg_opa(s_ws.pass_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ws.pass_cont, 12, 0);
    lv_obj_set_style_border_width(s_ws.pass_cont, 1, 0);
    lv_obj_set_style_border_color(s_ws.pass_cont, COLOR_SEP, 0);
    lv_obj_set_style_shadow_width(s_ws.pass_cont, 10, 0);
    lv_obj_set_style_shadow_color(s_ws.pass_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(s_ws.pass_cont, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(s_ws.pass_cont, 8, 0);
    lv_obj_clear_flag(s_ws.pass_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_ws.pass_cont, LV_OBJ_FLAG_HIDDEN);

    s_ws.pass_ssid_label = lv_label_create(s_ws.pass_cont);
    lv_label_set_text(s_ws.pass_ssid_label, "");
    lv_obj_set_style_text_font(s_ws.pass_ssid_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ws.pass_ssid_label, COLOR_TEXT_DARK, 0);
    lv_obj_align(s_ws.pass_ssid_label, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ws.pass_ta = lv_textarea_create(s_ws.pass_cont);
    lv_textarea_set_one_line(s_ws.pass_ta, true);
    lv_textarea_set_max_length(s_ws.pass_ta, PASS_MAX_LEN);
    lv_textarea_set_placeholder_text(s_ws.pass_ta, "Password");
    lv_textarea_set_password_mode(s_ws.pass_ta, true);
    lv_obj_set_size(s_ws.pass_ta, CARD_W - 160, 36);
    lv_obj_align(s_ws.pass_ta, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(s_ws.pass_ta, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_text_color(s_ws.pass_ta, COLOR_TEXT_DARK, 0);
    lv_obj_set_style_border_color(s_ws.pass_ta, COLOR_SEP, 0);
    lv_obj_add_event_cb(s_ws.pass_ta, pass_ta_ready_cb, LV_EVENT_READY, NULL);

    s_ws.pass_btn = lv_btn_create(s_ws.pass_cont);
    lv_obj_set_size(s_ws.pass_btn, 90, 36);
    lv_obj_align(s_ws.pass_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(s_ws.pass_btn, COLOR_BLUE, 0);
    lv_obj_set_style_radius(s_ws.pass_btn, 8, 0);
    lv_obj_add_event_cb(s_ws.pass_btn, pass_connect_cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *lbl = lv_label_create(s_ws.pass_btn);
        lv_label_set_text(lbl, "Connect");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
    }

    /* Keyboard */
    s_ws.kb = lv_keyboard_create(parent);
    lv_obj_set_size(s_ws.kb, 520, 200);
    lv_obj_align(s_ws.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_ws.kb, LV_OBJ_FLAG_HIDDEN);

    /* Start */
    s_ws.poll_timer = lv_timer_create(poll_timer_cb, 200, NULL);
    ws_start_scan();
}
