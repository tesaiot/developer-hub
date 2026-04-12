/**
 * @file    main_example.c
 * @brief   WiFi Connect - STA connection flow UI demonstration
 *
 * @description
 *   Non-blocking WiFi STA flow UI: scan -> select AP from dropdown -> enter
 *   password -> connect. This practise version demonstrates the full UI state
 *   machine with demo data. The actual WiFi stack (cy_wcm) runs on CM33_NS
 *   and requires either IPC bridge or direct CM33 deployment for real WiFi.
 *
 *   TODO: To enable real WiFi connectivity, either:
 *   1. Deploy this example on CM33_NS where cy_wcm is available, or
 *   2. Implement IPC calls to the CM33 WiFi manager from CM55.
 *
 * Ported from Developer Hub a02_wifi_connect.
 *
 * @board    KIT_PSE84_AI, TESAIoT_PSE84_AI
 * @author   TESAIoT
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_SCAN_RESULTS    16
#define PASSWORD_MAX_LEN    64
#define POLL_INTERVAL_MS    200

/* Demo AP data for UI demonstration */
#define DEMO_AP_COUNT       5

/* ---------------------------------------------------------------------------
 * State Machine
 * --------------------------------------------------------------------------- */
typedef enum {
    STATE_IDLE,
    STATE_SCANNING,
    STATE_SELECTING,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_ERROR,
} connect_state_t;

typedef struct {
    char ssid[33];
    int  rssi;
    int  security;  /* 0=Open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3 */
} demo_ap_entry_t;

typedef struct {
    lv_obj_t          *parent;
    lv_obj_t          *status_label;
    lv_obj_t          *ip_label;
    lv_obj_t          *spinner;
    lv_obj_t          *dropdown;
    lv_obj_t          *textarea;
    lv_obj_t          *keyboard;
    lv_obj_t          *btn_scan;
    lv_obj_t          *btn_connect;
    lv_obj_t          *indicator;
    lv_timer_t        *poll_timer;
    demo_ap_entry_t    aps[MAX_SCAN_RESULTS];
    int                ap_count;
    connect_state_t    state;
    uint32_t           state_enter_tick;
} wifi_connect_ctx_t;

static wifi_connect_ctx_t s_ctx;

/* Demo AP list */
static const demo_ap_entry_t s_demo_aps[DEMO_AP_COUNT] = {
    { "TESAIoT-Lab",      -42, 3 },
    { "TESA-Guest",        -58, 3 },
    { "IoT-Workshop",      -65, 2 },
    { "SmartFactory-5G",   -71, 4 },
    { "OpenNet",           -78, 0 },
};

/* ---------------------------------------------------------------------------
 * State transition: update UI visibility
 * --------------------------------------------------------------------------- */
static void set_state(wifi_connect_ctx_t *ctx, connect_state_t new_state)
{
    ctx->state = new_state;
    ctx->state_enter_tick = lv_tick_get();

    bool show_spinner  = (new_state == STATE_SCANNING || new_state == STATE_CONNECTING);
    bool show_dropdown = (new_state == STATE_SELECTING || new_state == STATE_CONNECTING);
    bool show_password = (new_state == STATE_SELECTING);
    bool show_keyboard = (new_state == STATE_SELECTING);
    bool show_connect  = (new_state == STATE_SELECTING);
    bool show_scan     = (new_state == STATE_IDLE || new_state == STATE_CONNECTED ||
                          new_state == STATE_ERROR);
    bool show_ip       = (new_state == STATE_CONNECTED);

    if (show_spinner)  lv_obj_remove_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);

    if (show_dropdown) lv_obj_remove_flag(ctx->dropdown, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->dropdown, LV_OBJ_FLAG_HIDDEN);

    if (show_password) lv_obj_remove_flag(ctx->textarea, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->textarea, LV_OBJ_FLAG_HIDDEN);

    if (show_keyboard) lv_obj_remove_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);

    if (show_connect)  lv_obj_remove_flag(ctx->btn_connect, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->btn_connect, LV_OBJ_FLAG_HIDDEN);

    if (show_scan)     lv_obj_remove_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);

    if (show_ip)       lv_obj_remove_flag(ctx->ip_label, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ctx->ip_label, LV_OBJ_FLAG_HIDDEN);

    /* Update indicator color */
    lv_color_t ind_color;
    switch (new_state) {
        case STATE_CONNECTED: ind_color = UI_COLOR_SUCCESS; break;
        case STATE_ERROR:     ind_color = UI_COLOR_ERROR;   break;
        case STATE_SCANNING:
        case STATE_CONNECTING: ind_color = UI_COLOR_WARNING; break;
        default:               ind_color = UI_COLOR_TEXT_DIM; break;
    }
    lv_obj_set_style_bg_color(ctx->indicator, ind_color, 0);
}

/* ---------------------------------------------------------------------------
 * Security string
 * --------------------------------------------------------------------------- */
static const char *security_str(int sec)
{
    switch (sec) {
        case 0:  return "Open";
        case 1:  return "WEP";
        case 2:  return "WPA";
        case 3:  return "WPA2";
        case 4:  return "WPA3";
        default: return "?";
    }
}

/* ---------------------------------------------------------------------------
 * Poll timer: handles scan-ready and connect-status checks (DEMO)
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)lv_timer_get_user_data(timer);
    uint32_t elapsed = lv_tick_get() - ctx->state_enter_tick;

    if (ctx->state == STATE_SCANNING) {
        /* TODO: Replace with real wifi_manager_scan_ready() check */
        /* Simulate scan completing after 1.5 seconds */
        if (elapsed < 1500) return;

        /* Populate with demo AP data */
        ctx->ap_count = DEMO_AP_COUNT;
        for (int i = 0; i < DEMO_AP_COUNT; i++) {
            ctx->aps[i] = s_demo_aps[i];
        }

        /* Populate dropdown */
        lv_dropdown_clear_options(ctx->dropdown);
        for (int i = 0; i < ctx->ap_count; i++) {
            char opt[64];
            snprintf(opt, sizeof(opt), "%s (%d dBm, %s)",
                     ctx->aps[i].ssid, ctx->aps[i].rssi,
                     security_str(ctx->aps[i].security));
            lv_dropdown_add_option(ctx->dropdown, opt, (uint32_t)i);
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "Found %d networks", ctx->ap_count);
        lv_label_set_text(ctx->status_label, buf);
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
        set_state(ctx, STATE_SELECTING);

    } else if (ctx->state == STATE_CONNECTING) {
        /* TODO: Replace with real wifi_manager_is_connected() check */
        /* Simulate connection completing after 2 seconds */
        if (elapsed < 2000) return;

        /* Show demo IP address */
        lv_label_set_text(ctx->ip_label, "IP: 192.168.1.42 (demo)");
        lv_obj_set_style_text_color(ctx->ip_label, UI_COLOR_SUCCESS, 0);

        lv_label_set_text(ctx->status_label, "Connected! (demo)");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
        set_state(ctx, STATE_CONNECTED);
    }
}

/* ---------------------------------------------------------------------------
 * Button callbacks
 * --------------------------------------------------------------------------- */
static void btn_scan_clicked(lv_event_t *e)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)lv_event_get_user_data(e);

    /* TODO: Replace with real wifi_manager_scan_start() */
    lv_label_set_text(ctx->status_label, "Scanning... (demo)");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    set_state(ctx, STATE_SCANNING);
}

static void btn_connect_clicked(lv_event_t *e)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)lv_event_get_user_data(e);

    uint32_t sel = lv_dropdown_get_selected(ctx->dropdown);
    if ((int)sel >= ctx->ap_count) return;

    const char *password = lv_textarea_get_text(ctx->textarea);
    const char *ssid = ctx->aps[sel].ssid;

    /* TODO: Replace with real wifi_manager_connect(ssid, password) */
    (void)password;

    char buf[64];
    snprintf(buf, sizeof(buf), "Connecting to %s... (demo)", ssid);
    lv_label_set_text(ctx->status_label, buf);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    set_state(ctx, STATE_CONNECTING);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Connect");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub,
        "เชื่อมต่อ WiFi");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* --- Demo notice --- */
    lv_obj_t *demo_note = lv_label_create(parent);
    lv_label_set_text(demo_note, "(Demo mode - WiFi runs on CM33_NS)");
    lv_obj_set_style_text_font(demo_note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(demo_note, UI_COLOR_WARNING, 0);
    lv_obj_align(demo_note, LV_ALIGN_TOP_MID, 0, 44);

    /* --- Connection indicator (circle) --- */
    s_ctx.indicator = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.indicator, 14, 14);
    lv_obj_set_style_radius(s_ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ctx.indicator, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(s_ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.indicator, 0, 0);
    lv_obj_align(s_ctx.indicator, LV_ALIGN_TOP_LEFT, 16, 62);
    lv_obj_clear_flag(s_ctx.indicator, LV_OBJ_FLAG_SCROLLABLE);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Scan to find networks");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 38, 58);

    /* --- Spinner --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 36, 36);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 52);
    lv_obj_add_flag(s_ctx.spinner, LV_OBJ_FLAG_HIDDEN);

    /* --- Dropdown (network list) --- */
    s_ctx.dropdown = lv_dropdown_create(parent);
    lv_obj_set_width(s_ctx.dropdown, 400);
    lv_obj_align(s_ctx.dropdown, LV_ALIGN_TOP_MID, 0, 84);
    lv_dropdown_set_text(s_ctx.dropdown, "Select Network");
    lv_obj_add_flag(s_ctx.dropdown, LV_OBJ_FLAG_HIDDEN);

    /* --- Password textarea --- */
    s_ctx.textarea = lv_textarea_create(parent);
    lv_obj_set_size(s_ctx.textarea, 400, 44);
    lv_obj_align(s_ctx.textarea, LV_ALIGN_TOP_MID, 0, 130);
    lv_textarea_set_placeholder_text(s_ctx.textarea, "Enter password");
    lv_textarea_set_password_mode(s_ctx.textarea, true);
    lv_textarea_set_max_length(s_ctx.textarea, PASSWORD_MAX_LEN);
    lv_textarea_set_one_line(s_ctx.textarea, true);
    lv_obj_add_flag(s_ctx.textarea, LV_OBJ_FLAG_HIDDEN);

    /* --- Keyboard --- */
    s_ctx.keyboard = lv_keyboard_create(parent);
    lv_obj_set_size(s_ctx.keyboard, 600, 160);
    lv_obj_align(s_ctx.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(s_ctx.keyboard, s_ctx.textarea);
    lv_obj_add_flag(s_ctx.keyboard, LV_OBJ_FLAG_HIDDEN);

    /* --- IP address label --- */
    s_ctx.ip_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.ip_label, "");
    lv_obj_set_style_text_font(s_ctx.ip_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.ip_label, UI_COLOR_SUCCESS, 0);
    lv_obj_align(s_ctx.ip_label, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_flag(s_ctx.ip_label, LV_OBJ_FLAG_HIDDEN);

    /* --- Scan button --- */
    s_ctx.btn_scan = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_scan, 140, 44);
    lv_obj_align(s_ctx.btn_scan, LV_ALIGN_BOTTOM_MID, -80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_scan, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(s_ctx.btn_scan, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_scan, btn_scan_clicked, LV_EVENT_CLICKED, &s_ctx);
    lv_obj_t *lbl1 = lv_label_create(s_ctx.btn_scan);
    lv_label_set_text(lbl1, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(lbl1);

    /* --- Connect button --- */
    s_ctx.btn_connect = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_connect, 140, 44);
    lv_obj_align(s_ctx.btn_connect, LV_ALIGN_BOTTOM_MID, 80, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_connect, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(s_ctx.btn_connect, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_connect, btn_connect_clicked, LV_EVENT_CLICKED, &s_ctx);
    lv_obj_t *lbl2 = lv_label_create(s_ctx.btn_connect);
    lv_label_set_text(lbl2, LV_SYMBOL_OK " Connect");
    lv_obj_center(lbl2);
    lv_obj_add_flag(s_ctx.btn_connect, LV_OBJ_FLAG_HIDDEN);

    /* --- Poll timer for non-blocking operations --- */
    s_ctx.poll_timer = lv_timer_create(poll_timer_cb, POLL_INTERVAL_MS, &s_ctx);
}
