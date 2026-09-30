/**
 * @file    main_example.c
 * @brief   WiFi Connect - STA connection flow with wifi_manager API
 *
 * @description
 *   Non-blocking WiFi STA flow: scan -> select AP from dropdown -> enter
 *   password -> connect. Uses wifi_manager non-blocking scan/status APIs
 *   to avoid blocking the GFX task. State machine drives UI transitions.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"
#include "wifi_manager.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_SCAN_RESULTS    WIFI_MGR_SCAN_MAX_ENTRIES
#define PASSWORD_MAX_LEN    64
#define POLL_INTERVAL_MS    200

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
    wifi_mgr_scan_entry_t aps[MAX_SCAN_RESULTS];
    int                ap_count;
    connect_state_t    state;
} wifi_connect_ctx_t;

static wifi_connect_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * State transition: update UI visibility
 * --------------------------------------------------------------------------- */
static void set_state(wifi_connect_ctx_t *ctx, connect_state_t new_state)
{
    ctx->state = new_state;

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
static const char *security_str(uint8_t sec)
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
 * Poll timer: handles scan-ready and connect-status checks
 * --------------------------------------------------------------------------- */
static void poll_timer_cb(lv_timer_t *timer)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)lv_timer_get_user_data(timer);

    if (ctx->state == STATE_SCANNING) {
        if (!wifi_manager_scan_ready()) return;

        ctx->ap_count = wifi_manager_scan_result(ctx->aps, MAX_SCAN_RESULTS);

        if (ctx->ap_count <= 0) {
            lv_label_set_text(ctx->status_label, "No networks found");
            lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_WARNING, 0);
            set_state(ctx, STATE_ERROR);
            return;
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
        /* Check if connected */
        if (wifi_manager_is_connected()) {
            const char *ip = wifi_manager_get_ip();
            char ip_buf[48];
            snprintf(ip_buf, sizeof(ip_buf), "IP: %s", ip ? ip : "unknown");
            lv_label_set_text(ctx->ip_label, ip_buf);
            lv_obj_set_style_text_color(ctx->ip_label, UI_COLOR_SUCCESS, 0);

            lv_label_set_text(ctx->status_label, "Connected!");
            lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
            set_state(ctx, STATE_CONNECTED);
        }
        /* Note: timeout handled by wifi_manager internally */
    }
}

/* ---------------------------------------------------------------------------
 * Button callbacks
 * --------------------------------------------------------------------------- */
static void btn_scan_clicked(lv_event_t *e)
{
    wifi_connect_ctx_t *ctx = (wifi_connect_ctx_t *)lv_event_get_user_data(e);

    if (!wifi_manager_scan_start()) {
        lv_label_set_text(ctx->status_label, "Scan start failed");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
        return;
    }

    lv_label_set_text(ctx->status_label, "Scanning...");
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

    lv_label_set_text(ctx->status_label, "Connecting...");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    set_state(ctx, STATE_CONNECTING);

    /* wifi_manager_connect is blocking on CM33; on CM55 it sends IPC */
    bool ok = wifi_manager_connect(ssid, password);
    if (!ok) {
        lv_label_set_text(ctx->status_label, "Connect failed");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
        set_state(ctx, STATE_ERROR);
    }
    /* If ok, poll_timer_cb will detect connection via wifi_manager_is_connected() */
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Connect");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* เชื่อมต่อ WiFi */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เชื่อมต่อ WiFi");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* --- Connection indicator (circle) --- */
    s_ctx.indicator = lv_obj_create(parent);
    lv_obj_set_size(s_ctx.indicator, 14, 14);
    lv_obj_set_style_radius(s_ctx.indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ctx.indicator, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_bg_opa(s_ctx.indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.indicator, 0, 0);
    lv_obj_align(s_ctx.indicator, LV_ALIGN_TOP_LEFT, 16, 42);
    lv_obj_clear_flag(s_ctx.indicator, LV_OBJ_FLAG_SCROLLABLE);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Scan to find networks");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 38, 38);

    /* --- Spinner --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 36, 36);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 32);
    lv_obj_add_flag(s_ctx.spinner, LV_OBJ_FLAG_HIDDEN);

    /* --- Dropdown (network list) --- */
    s_ctx.dropdown = lv_dropdown_create(parent);
    lv_obj_set_width(s_ctx.dropdown, 400);
    lv_obj_align(s_ctx.dropdown, LV_ALIGN_TOP_MID, 0, 64);
    lv_dropdown_set_text(s_ctx.dropdown, "Select Network");
    lv_obj_add_flag(s_ctx.dropdown, LV_OBJ_FLAG_HIDDEN);

    /* --- Password textarea --- */
    s_ctx.textarea = lv_textarea_create(parent);
    lv_obj_set_size(s_ctx.textarea, 400, 44);
    lv_obj_align(s_ctx.textarea, LV_ALIGN_TOP_MID, 0, 110);
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
