/**
 * @file    main_example.c
 * @brief   WiFi Connect via IPC - CM55 sends commands to CM33_NS
 *
 * @description
 *   Demonstrates the cross-core WiFi connection flow: CM55 builds IPC messages
 *   to request scan, connect, and disconnect operations that execute on CM33_NS
 *   (which owns the CYW55513 SDHC0 bus). WiFi state updates arrive asynchronously
 *   via IPC_CMD_WIFI_STATE_PUSH and are applied to LVGL using the deferred-flag
 *   pattern (ISR sets flag, LVGL timer reads it).
 *
 *   IPC Command Reference:
 *     0xD0  IPC_CMD_WIFI_SCAN       - trigger AP scan on CM33_NS
 *     0xD1  IPC_CMD_WIFI_CONNECT    - connect (SSID + password in payload)
 *     0xD2  IPC_CMD_WIFI_DISCONNECT - disconnect current AP
 *     0xD9  IPC_CMD_WIFI_STATE_PUSH - CM33 pushes state to CM55
 *
 * @board   AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/* ── IPC Commands ──────────────────────────────────────────────────── */
#define IPC_CMD_WIFI_SCAN         (0xD0)
#define IPC_CMD_WIFI_CONNECT      (0xD1)
#define IPC_CMD_WIFI_DISCONNECT   (0xD2)
#define IPC_CMD_WIFI_STATE_PUSH   (0xD9)

/* ── WiFi States ───────────────────────────────────────────────────── */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_SCAN_BUSY,
    WIFI_STATE_ERROR,
} wifi_state_t;

/* ── Colors ────────────────────────────────────────────────────────── */
#define COLOR_BG          lv_color_hex(0x0D1B2A)
#define COLOR_CARD        lv_color_hex(0x142240)
#define COLOR_TEXT        lv_color_hex(0xE0E0E0)
#define COLOR_CONNECTED   lv_color_hex(0x4CAF50)
#define COLOR_CONNECTING  lv_color_hex(0xFF9800)
#define COLOR_DISCONNECT  lv_color_hex(0xF44336)
#define COLOR_BTN_BG      lv_color_hex(0x1A3050)

/* ── IPC TX buffer: MUST be in shared SRAM ─────────────────────────── */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;

/* ── WiFi connect payload layout ───────────────────────────────────── */
/* data[0]     = SSID length (max 32)
 * data[1..32] = SSID bytes
 * data[33]    = password length (max 63)
 * data[34..96]= password bytes
 */
#define SSID_OFFSET       1
#define SSID_MAX_LEN      32
#define PASS_LEN_OFFSET   33
#define PASS_OFFSET        34
#define PASS_MAX_LEN      63

/* ── Deferred flag state (written in ISR, read in task) ────────────── */
static volatile bool         s_state_updated;
static volatile wifi_state_t s_wifi_state;
static volatile int8_t       s_rssi;
static volatile uint32_t     s_ip_addr;

/* ── UI handles ────────────────────────────────────────────────────── */
static lv_obj_t *s_status_indicator;
static lv_obj_t *s_status_label;
static lv_obj_t *s_ssid_ta;
static lv_obj_t *s_pass_ta;
static lv_obj_t *s_btn_connect;
static lv_obj_t *s_btn_disconnect;
static lv_obj_t *s_rssi_label;
static lv_obj_t *s_ip_label;
static lv_obj_t *s_kb;

static wifi_state_t s_displayed_state;

/* ── IPC callback - ISR context, deferred flag only ────────────────── */
static void wifi_state_ipc_cb(uint32_t *msg_ptr)
{
    ipc_msg_t *msg = (ipc_msg_t *)msg_ptr;
    if (msg->cmd != IPC_CMD_WIFI_STATE_PUSH) return;

    /* data[0] = state, data[1] = rssi (signed), data[2..5] = IP (LE) */
    s_wifi_state = (wifi_state_t)msg->data[0];
    s_rssi       = (int8_t)msg->data[1];
    s_ip_addr    = (uint32_t)msg->data[2]
                 | ((uint32_t)msg->data[3] << 8)
                 | ((uint32_t)msg->data[4] << 16)
                 | ((uint32_t)msg->data[5] << 24);

    /* DEFERRED: do NOT touch LVGL here */
    s_state_updated = true;
}

/* ── Send IPC command helper ───────────────────────────────────────── */
static bool send_ipc_cmd(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    memset(&s_ipc_tx, 0, sizeof(s_ipc_tx));
    s_ipc_tx.cmd = cmd;
    if (payload && len > 0) {
        memcpy(s_ipc_tx.data, payload, len);
    }

    cy_en_ipc_pipe_status_t st =
        Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                                CM55_IPC_PIPE_EP_ADDR,
                                (uint32_t *)&s_ipc_tx, NULL);
    return (st == CY_IPC_PIPE_SUCCESS);
}

/* ── Build and send WiFi connect command ───────────────────────────── */
static void send_wifi_connect(void)
{
    const char *ssid = lv_textarea_get_text(s_ssid_ta);
    const char *pass = lv_textarea_get_text(s_pass_ta);
    uint8_t ssid_len = (uint8_t)strlen(ssid);
    uint8_t pass_len = (uint8_t)strlen(pass);

    if (ssid_len == 0 || ssid_len > SSID_MAX_LEN) return;
    if (pass_len > PASS_MAX_LEN) return;

    uint8_t payload[97];
    memset(payload, 0, sizeof(payload));
    payload[0] = ssid_len;
    memcpy(&payload[SSID_OFFSET], ssid, ssid_len);
    payload[PASS_LEN_OFFSET] = pass_len;
    if (pass_len > 0) {
        memcpy(&payload[PASS_OFFSET], pass, pass_len);
    }

    send_ipc_cmd(IPC_CMD_WIFI_CONNECT, payload, sizeof(payload));

    /* Optimistic UI update */
    s_wifi_state = WIFI_STATE_CONNECTING;
    s_state_updated = true;
}

/* ── UI state update (runs in LVGL timer - task context) ───────────── */
static void update_ui_state(void)
{
    lv_color_t ind_color;
    const char *state_text;

    switch (s_displayed_state) {
    case WIFI_STATE_CONNECTED:
        ind_color  = COLOR_CONNECTED;
        state_text = LV_SYMBOL_WIFI " Connected";
        break;
    case WIFI_STATE_CONNECTING:
        ind_color  = COLOR_CONNECTING;
        state_text = LV_SYMBOL_REFRESH " Connecting...";
        break;
    case WIFI_STATE_SCAN_BUSY:
        ind_color  = COLOR_CONNECTING;
        state_text = LV_SYMBOL_REFRESH " Scanning...";
        break;
    case WIFI_STATE_ERROR:
        ind_color  = COLOR_DISCONNECT;
        state_text = LV_SYMBOL_WARNING " Error";
        break;
    default:
        ind_color  = COLOR_DISCONNECT;
        state_text = LV_SYMBOL_CLOSE " Disconnected";
        break;
    }

    lv_obj_set_style_bg_color(s_status_indicator, ind_color, 0);
    lv_label_set_text(s_status_label, state_text);

    if (s_displayed_state == WIFI_STATE_CONNECTED) {
        lv_label_set_text_fmt(s_rssi_label, "RSSI: %d dBm", (int)s_rssi);
        uint8_t *ip = (uint8_t *)&s_ip_addr;
        lv_label_set_text_fmt(s_ip_label, "IP: %d.%d.%d.%d",
                              ip[0], ip[1], ip[2], ip[3]);
    } else {
        lv_label_set_text(s_rssi_label, "RSSI: --");
        lv_label_set_text(s_ip_label, "IP: --");
    }
}

/* ── LVGL timer: deferred flag polling ─────────────────────────────── */
static void wifi_poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_state_updated) {
        s_state_updated = false;
        s_displayed_state = s_wifi_state;
        update_ui_state();
    }
}

/* ── Button callbacks ──────────────────────────────────────────────── */
static void connect_btn_cb(lv_event_t *e)
{
    (void)e;
    send_wifi_connect();
}

static void disconnect_btn_cb(lv_event_t *e)
{
    (void)e;
    send_ipc_cmd(IPC_CMD_WIFI_DISCONNECT, NULL, 0);
}

/* ── Textarea focus → show keyboard ────────────────────────────────── */
static void ta_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
}

static void ta_defocus_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
}

/* ── Helper: create styled button ──────────────────────────────────── */
static lv_obj_t *create_btn(lv_obj_t *parent, const char *text,
                             lv_color_t color, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 200, 44);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);

    return btn;
}

/* ── Entry point ───────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Reset state */
    s_state_updated  = false;
    s_wifi_state     = WIFI_STATE_DISCONNECTED;
    s_displayed_state = WIFI_STATE_DISCONNECTED;
    s_rssi           = 0;
    s_ip_addr        = 0;

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);

    /* ── Title ─────────────────────────────────────────────────────── */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Connect via IPC");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* เชื่อมต่อ WiFi ผ่าน IPC */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เชื่อมต่อ WiFi ผ่าน IPC");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* ── Status card ───────────────────────────────────────────────── */
    lv_obj_t *status_card = example_card_create(parent, 440, 80, COLOR_CARD);
    lv_obj_align(status_card, LV_ALIGN_TOP_MID, 0, 36);

    /* Status indicator circle */
    s_status_indicator = lv_obj_create(status_card);
    lv_obj_set_size(s_status_indicator, 16, 16);
    lv_obj_set_style_radius(s_status_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_status_indicator, COLOR_DISCONNECT, 0);
    lv_obj_set_style_bg_opa(s_status_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_status_indicator, 0, 0);
    lv_obj_align(s_status_indicator, LV_ALIGN_LEFT_MID, 0, -12);

    s_status_label = lv_label_create(status_card);
    lv_label_set_text(s_status_label, LV_SYMBOL_CLOSE " Disconnected");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status_label, COLOR_TEXT, 0);
    lv_obj_align(s_status_label, LV_ALIGN_LEFT_MID, 24, -12);

    s_rssi_label = lv_label_create(status_card);
    lv_label_set_text(s_rssi_label, "RSSI: --");
    lv_obj_set_style_text_font(s_rssi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_rssi_label, lv_color_hex(0x808080), 0);
    lv_obj_align(s_rssi_label, LV_ALIGN_LEFT_MID, 0, 12);

    s_ip_label = lv_label_create(status_card);
    lv_label_set_text(s_ip_label, "IP: --");
    lv_obj_set_style_text_font(s_ip_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ip_label, lv_color_hex(0x808080), 0);
    lv_obj_align(s_ip_label, LV_ALIGN_RIGHT_MID, 0, 12);

    /* ── Input card ────────────────────────────────────────────────── */
    lv_obj_t *input_card = example_card_create(parent, 440, 160, COLOR_CARD);
    lv_obj_align(input_card, LV_ALIGN_TOP_MID, 0, 126);

    /* SSID label + textarea */
    lv_obj_t *ssid_lbl = lv_label_create(input_card);
    lv_label_set_text(ssid_lbl, "SSID:");
    lv_obj_set_style_text_color(ssid_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ssid_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(ssid_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ssid_ta = lv_textarea_create(input_card);
    lv_textarea_set_one_line(s_ssid_ta, true);
    lv_textarea_set_max_length(s_ssid_ta, SSID_MAX_LEN);
    lv_textarea_set_placeholder_text(s_ssid_ta, "Enter SSID");
    lv_obj_set_size(s_ssid_ta, 360, 40);
    lv_obj_align(s_ssid_ta, LV_ALIGN_TOP_RIGHT, 0, -6);
    lv_obj_set_style_bg_color(s_ssid_ta, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_text_color(s_ssid_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(s_ssid_ta, lv_color_hex(0x2A3A5C), 0);
    lv_obj_add_event_cb(s_ssid_ta, ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_ssid_ta, ta_defocus_cb, LV_EVENT_DEFOCUSED, NULL);

    /* Password label + textarea */
    lv_obj_t *pass_lbl = lv_label_create(input_card);
    lv_label_set_text(pass_lbl, "Password:");
    lv_obj_set_style_text_color(pass_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(pass_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(pass_lbl, LV_ALIGN_TOP_LEFT, 0, 52);

    s_pass_ta = lv_textarea_create(input_card);
    lv_textarea_set_one_line(s_pass_ta, true);
    lv_textarea_set_max_length(s_pass_ta, PASS_MAX_LEN);
    lv_textarea_set_placeholder_text(s_pass_ta, "Enter password");
    lv_textarea_set_password_mode(s_pass_ta, true);
    lv_obj_set_size(s_pass_ta, 360, 40);
    lv_obj_align(s_pass_ta, LV_ALIGN_TOP_RIGHT, 0, 46);
    lv_obj_set_style_bg_color(s_pass_ta, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_text_color(s_pass_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(s_pass_ta, lv_color_hex(0x2A3A5C), 0);
    lv_obj_add_event_cb(s_pass_ta, ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_pass_ta, ta_defocus_cb, LV_EVENT_DEFOCUSED, NULL);

    /* Buttons row */
    s_btn_connect = create_btn(input_card, LV_SYMBOL_OK " Connect",
                               lv_color_hex(0x00796B), connect_btn_cb);
    lv_obj_align(s_btn_connect, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    s_btn_disconnect = create_btn(input_card, LV_SYMBOL_CLOSE " Disconnect",
                                   COLOR_DISCONNECT, disconnect_btn_cb);
    lv_obj_align(s_btn_disconnect, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    /* ── Info card: IPC protocol reference ─────────────────────────── */
    lv_obj_t *info_card = example_card_create(parent, 440, 120, COLOR_CARD);
    lv_obj_align(info_card, LV_ALIGN_TOP_MID, 0, 296);

    lv_obj_t *info_title = lv_label_create(info_card);
    lv_label_set_text(info_title, "IPC Protocol:");
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *info_body = lv_label_create(info_card);
    lv_label_set_text(info_body,
        "0xD0  WIFI_SCAN        Trigger AP scan\n"
        "0xD1  WIFI_CONNECT     SSID+pass in payload\n"
        "0xD2  WIFI_DISCONNECT  Drop current AP\n"
        "0xD9  STATE_PUSH       CM33 -> CM55 async");
    lv_obj_set_style_text_font(info_body, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info_body, lv_color_hex(0x808080), 0);
    lv_obj_align(info_body, LV_ALIGN_TOP_LEFT, 0, 20);

    /* ── On-screen keyboard (hidden until TA focused) ──────────────── */
    s_kb = lv_keyboard_create(parent);
    lv_obj_set_size(s_kb, 480, 240);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);

    /* ── Register IPC callback + start deferred poll timer ─────────── */
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                  wifi_state_ipc_cb,
                                  IPC_CMD_WIFI_STATE_PUSH);
    lv_timer_create(wifi_poll_timer_cb, 50, NULL);
}
