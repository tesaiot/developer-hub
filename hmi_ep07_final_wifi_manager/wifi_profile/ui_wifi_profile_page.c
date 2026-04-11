#include "ui_wifi_profile_page.h"

#include "ui_menu_layout.h"
#include "wifi_profile_store.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define UI_PROFILE_LOG_ENABLE       (1)
#define UI_PROFILE_KEYBOARD_H       (165)
#define UI_PROFILE_INPUT_H          (34)

typedef struct {
    lv_obj_t *page;
    lv_obj_t *hint_label;
    lv_obj_t *status_label;
    lv_obj_t *ssid_ta;
    lv_obj_t *password_ta;
    lv_obj_t *security_dd;
    lv_obj_t *auto_switch;
    lv_obj_t *keyboard;
} ui_wifi_profile_ctx_t;

static ui_wifi_profile_ctx_t s_ctx;

static void ui_profile_log(const char *msg)
{
#if UI_PROFILE_LOG_ENABLE
    printf("[PROFILE_UI] %s\r\n", msg);
#else
    LV_UNUSED(msg);
#endif
}

static void ui_profile_set_status(const char *text, lv_color_t color)
{
    if(s_ctx.status_label == NULL) {
        return;
    }

    lv_label_set_text(s_ctx.status_label, text);
    lv_obj_set_style_text_color(s_ctx.status_label, color, LV_PART_MAIN);
}

/* Keyboard is hidden by default and shown only when a text input is focused. */
static void ui_profile_set_keyboard_visible(bool visible)
{
    if(s_ctx.keyboard == NULL) {
        return;
    }

    if(visible) {
        lv_obj_clear_flag(s_ctx.keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_height(s_ctx.keyboard, UI_PROFILE_KEYBOARD_H);
    } else {
        lv_keyboard_set_textarea(s_ctx.keyboard, NULL);
        lv_obj_set_height(s_ctx.keyboard, 0);
        lv_obj_add_flag(s_ctx.keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_profile_set_security_from_text(const char *security)
{
    uint16_t idx = 3U; /* AUTO */

    if(security == NULL) {
        lv_dropdown_set_selected(s_ctx.security_dd, idx);
        return;
    }

    if(strstr(security, "OPEN") != NULL) {
        idx = 0U;
    } else if(strstr(security, "WPA2") != NULL) {
        idx = 1U;
    } else if(strstr(security, "WPA3") != NULL) {
        idx = 2U;
    }

    lv_dropdown_set_selected(s_ctx.security_dd, idx);
}

static void ui_profile_get_security_text(char *buf, uint32_t buf_len)
{
    if((buf == NULL) || (buf_len == 0U) || (s_ctx.security_dd == NULL)) {
        return;
    }

    (void)memset(buf, 0, buf_len);
    lv_dropdown_get_selected_str(s_ctx.security_dd, buf, buf_len);
}

static void ui_profile_fill_from_data(const wifi_profile_data_t *profile)
{
    if((profile == NULL) || (s_ctx.ssid_ta == NULL) || (s_ctx.password_ta == NULL)) {
        return;
    }

    lv_textarea_set_text(s_ctx.ssid_ta, profile->ssid);
    lv_textarea_set_text(s_ctx.password_ta, profile->password);
    ui_profile_set_security_from_text(profile->security);

    if(profile->auto_connect) {
        lv_obj_add_state(s_ctx.auto_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_ctx.auto_switch, LV_STATE_CHECKED);
    }
}

static void ui_profile_read_to_data(wifi_profile_data_t *profile)
{
    char security_text[WIFI_PROFILE_SECURITY_MAX_LEN + 1U];

    if(profile == NULL) {
        return;
    }

    (void)memset(profile, 0, sizeof(*profile));

    (void)strncpy(profile->ssid,
                  lv_textarea_get_text(s_ctx.ssid_ta),
                  sizeof(profile->ssid) - 1U);
    (void)strncpy(profile->password,
                  lv_textarea_get_text(s_ctx.password_ta),
                  sizeof(profile->password) - 1U);

    ui_profile_get_security_text(security_text, sizeof(security_text));
    (void)strncpy(profile->security, security_text, sizeof(profile->security) - 1U);

    profile->auto_connect = lv_obj_has_state(s_ctx.auto_switch, LV_STATE_CHECKED);
}

static bool ui_profile_validate_before_save(const wifi_profile_data_t *profile)
{
    if((profile == NULL) || (profile->ssid[0] == '\0')) {
        ui_profile_set_status("Save failed: SSID is empty", lv_color_hex(0xFCA5A5));
        ui_profile_log("SAVE validate fail: empty SSID");
        return false;
    }

    if((strcmp(profile->security, "OPEN") != 0) && (profile->password[0] == '\0')) {
        ui_profile_set_status("Save failed: Password is required", lv_color_hex(0xFCA5A5));
        ui_profile_log("SAVE validate fail: empty password");
        return false;
    }

    return true;
}

static void ui_profile_keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if((code == LV_EVENT_READY) || (code == LV_EVENT_CANCEL)) {
        ui_profile_set_keyboard_visible(false);
    }
}

static void ui_profile_input_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if((code != LV_EVENT_CLICKED) && (code != LV_EVENT_FOCUSED)) {
        return;
    }

    if((s_ctx.keyboard == NULL) || (ta == NULL)) {
        return;
    }

    lv_keyboard_set_textarea(s_ctx.keyboard, ta);
    ui_profile_set_keyboard_visible(true);
}

static void ui_profile_save_btn_cb(lv_event_t *e)
{
    wifi_profile_data_t profile;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ui_profile_read_to_data(&profile);
    if(!ui_profile_validate_before_save(&profile)) {
        return;
    }

    if(wifi_profile_store_save(&profile)) {
        ui_profile_set_status("Profile saved to NVM", lv_color_hex(0x86EFAC));
        ui_profile_log("SAVE click -> OK");
    } else {
        ui_profile_set_status("Save failed (NVM write error)", lv_color_hex(0xFCA5A5));
        ui_profile_log("SAVE click -> FAIL");
    }
}

static void ui_profile_load_btn_cb(lv_event_t *e)
{
    wifi_profile_data_t profile;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if(wifi_profile_store_load(&profile)) {
        ui_profile_fill_from_data(&profile);
        ui_profile_set_status("Profile loaded from NVM", lv_color_hex(0x86EFAC));
        ui_profile_log("LOAD click -> OK");
    } else {
        ui_profile_set_status("No saved profile in NVM", lv_color_hex(0xFDE68A));
        ui_profile_log("LOAD click -> EMPTY");
    }
}

static void ui_profile_clear_btn_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if(wifi_profile_store_clear()) {
        lv_textarea_set_text(s_ctx.password_ta, "");
        lv_obj_clear_state(s_ctx.auto_switch, LV_STATE_CHECKED);
        ui_profile_set_status("Profile cleared from NVM", lv_color_hex(0x86EFAC));
        ui_profile_log("CLEAR click -> OK");
    } else {
        ui_profile_set_status("Clear failed (NVM write error)", lv_color_hex(0xFCA5A5));
        ui_profile_log("CLEAR click -> FAIL");
    }
}

static lv_obj_t *ui_profile_create_action_button(lv_obj_t *parent,
                                                 const char *text,
                                                 uint32_t bg_hex,
                                                 lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 120, 36);
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

lv_obj_t *ui_wifi_profile_page_create(lv_obj_t *menu)
{
    lv_obj_t *section;
    lv_obj_t *panel;
    lv_obj_t *form;
    lv_obj_t *row;
    lv_obj_t *action_row;
    wifi_profile_data_t saved_profile;

    if(menu == NULL) {
        return NULL;
    }

    (void)memset(&s_ctx, 0, sizeof(s_ctx));

    s_ctx.page = lv_menu_page_create(menu, "WiFi Profile");
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
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "WiFi Profile (NVM)");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_MENU_PANEL_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);

    s_ctx.hint_label = lv_label_create(panel);
    lv_label_set_text(s_ctx.hint_label, "Select AP from WiFi Scan page, type password, then Save.");
    lv_obj_set_width(s_ctx.hint_label, LV_PCT(100));
    lv_label_set_long_mode(s_ctx.hint_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_ctx.hint_label, lv_color_hex(UI_MENU_PANEL_TEXT_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.hint_label, &lv_font_montserrat_14, LV_PART_MAIN);

    form = lv_obj_create(panel);
    lv_obj_set_width(form, LV_PCT(100));
    lv_obj_set_flex_grow(form, 1);
    lv_obj_set_style_bg_opa(form, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(form, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(form, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(form, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(form, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(form, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_SCROLLABLE);

    row = lv_obj_create(form);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ssid_label = lv_label_create(row);
    lv_label_set_text(ssid_label, "SSID");
    lv_obj_set_width(ssid_label, 70);
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.ssid_ta = lv_textarea_create(row);
    lv_obj_set_size(s_ctx.ssid_ta, 300, UI_PROFILE_INPUT_H);
    lv_textarea_set_one_line(s_ctx.ssid_ta, true);
    lv_textarea_set_placeholder_text(s_ctx.ssid_ta, "SSID from scan");
    lv_obj_set_style_text_font(s_ctx.ssid_ta, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_add_event_cb(s_ctx.ssid_ta, ui_profile_input_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_ctx.ssid_ta, ui_profile_input_event_cb, LV_EVENT_FOCUSED, NULL);

    row = lv_obj_create(form);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sec_label = lv_label_create(row);
    lv_label_set_text(sec_label, "Security");
    lv_obj_set_width(sec_label, 70);
    lv_obj_set_style_text_color(sec_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(sec_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.security_dd = lv_dropdown_create(row);
    lv_dropdown_set_options(s_ctx.security_dd, "OPEN\nWPA2-AES-PSK\nWPA3-SAE\nAUTO");
    lv_obj_set_width(s_ctx.security_dd, 220);

    row = lv_obj_create(form);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *password_label = lv_label_create(row);
    lv_label_set_text(password_label, "Password");
    lv_obj_set_width(password_label, 70);
    lv_obj_set_style_text_color(password_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(password_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.password_ta = lv_textarea_create(row);
    lv_obj_set_size(s_ctx.password_ta, 300, UI_PROFILE_INPUT_H);
    lv_textarea_set_one_line(s_ctx.password_ta, true);
    lv_textarea_set_password_mode(s_ctx.password_ta, true);
    lv_textarea_set_placeholder_text(s_ctx.password_ta, "Type password");
    lv_obj_set_style_text_font(s_ctx.password_ta, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_add_event_cb(s_ctx.password_ta, ui_profile_input_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_ctx.password_ta, ui_profile_input_event_cb, LV_EVENT_FOCUSED, NULL);

    row = lv_obj_create(form);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *auto_label = lv_label_create(row);
    lv_label_set_text(auto_label, "Auto Connect");
    lv_obj_set_width(auto_label, 90);
    lv_obj_set_style_text_color(auto_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(auto_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.auto_switch = lv_switch_create(row);

    action_row = lv_obj_create(form);
    lv_obj_set_size(action_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(action_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(action_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(action_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(action_row, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(action_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(action_row, LV_OBJ_FLAG_SCROLLABLE);

    (void)ui_profile_create_action_button(action_row, "Save", 0x166534, ui_profile_save_btn_cb);
    (void)ui_profile_create_action_button(action_row, "Load", 0x1D4ED8, ui_profile_load_btn_cb);
    (void)ui_profile_create_action_button(action_row, "Clear", 0xB91C1C, ui_profile_clear_btn_cb);

    s_ctx.status_label = lv_label_create(form);
    lv_label_set_text(s_ctx.status_label, "NVM: not loaded");
    lv_obj_set_width(s_ctx.status_label, LV_PCT(100));
    lv_label_set_long_mode(s_ctx.status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_ctx.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.status_label, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.keyboard = lv_keyboard_create(panel);
    lv_obj_set_width(s_ctx.keyboard, LV_PCT(100));
    lv_obj_set_height(s_ctx.keyboard, 0);
    lv_obj_add_event_cb(s_ctx.keyboard, ui_profile_keyboard_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(s_ctx.keyboard, LV_OBJ_FLAG_HIDDEN);

    /* Load profile at startup to show latest saved values in the form. */
    if(wifi_profile_store_load(&saved_profile)) {
        ui_profile_fill_from_data(&saved_profile);
        ui_profile_set_status("Startup: profile loaded (Home is default page)", lv_color_hex(0x86EFAC));
        ui_profile_log("STARTUP load -> OK");
    } else {
        ui_profile_set_status("Startup: no saved profile", lv_color_hex(0xFDE68A));
        ui_profile_log("STARTUP load -> EMPTY");
    }

    return s_ctx.page;
}

void ui_wifi_profile_page_apply_ap(const wifi_scan_ap_t *ap)
{
    if((ap == NULL) || (s_ctx.ssid_ta == NULL) || (s_ctx.hint_label == NULL)) {
        return;
    }

    lv_textarea_set_text(s_ctx.ssid_ta, ap->ssid);
    ui_profile_set_security_from_text(ap->security);
    lv_label_set_text(s_ctx.hint_label,
                      "AP selected from scan. Enter password, then tap Save.");
    ui_profile_set_status("AP copied from WiFi Scan page", lv_color_hex(0x93C5FD));
    ui_profile_log("AP selected from scan");
}
