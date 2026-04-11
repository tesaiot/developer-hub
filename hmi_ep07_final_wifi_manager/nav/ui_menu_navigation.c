#include "lvgl.h"

#include "ui_menu_navigation.h"
#include "ui_menu_layout.h"
#include "menu_nav_logic.h"
#include "app_logo.h"
#include "ui_wifi_list_page.h"
#include "ui_wifi_profile_page.h"
#include "ui_wifi_status_page.h"

#include <stdio.h>

#define UI_NAV_LOG_ENABLE    (1)

static menu_nav_state_t s_menu_nav_state;

static void ui_nav_log(const char *msg)
{
#if UI_NAV_LOG_ENABLE
    printf("[UI_NAV] %s\r\n", msg);
#else
    LV_UNUSED(msg);
#endif
}

static lv_obj_t *create_top_nav_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_MENU_NAV_BTN_W, UI_MENU_NAV_BTN_H);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_MENU_NAV_BTN_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(UI_MENU_NAV_BTN_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_MENU_NAV_BTN_TEXT_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label);

    return btn;
}

/* Called by WiFi Scan page when user confirms an AP row. */
static void ui_menu_nav_use_ap_cb(const wifi_scan_ap_t *ap, void *user_data)
{
    menu_nav_state_t *state = (menu_nav_state_t *)user_data;

    if((ap == NULL) || (state == NULL)) {
        return;
    }

    ui_wifi_profile_page_apply_ap(ap);
    menu_nav_logic_go_profile(state);
    ui_nav_log("AP forwarded to profile page");
}

void ui_wifi_manager_final_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    ui_nav_log("CREATE shell");

    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_MENU_BG_COLOR_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    /* Header */
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), UI_MENU_HEADER_H);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_MENU_HEADER_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(UI_MENU_HEADER_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(header, UI_MENU_HEADER_PAD_HOR, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(header, UI_MENU_HEADER_PAD_VER, LV_PART_MAIN);
    lv_obj_set_style_pad_column(header, UI_MENU_HEADER_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header_title = lv_label_create(header);
    lv_label_set_text(header_title, "WiFi Manager (Final App)");
    lv_obj_set_style_text_color(header_title, lv_color_hex(UI_MENU_HEADER_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(header_title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_align(header_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_flex_grow(header_title, 1);

    lv_obj_t *header_logo = lv_image_create(header);
    lv_image_set_src(header_logo, &APP_LOGO);
    lv_image_set_scale(header_logo, 36);
    lv_obj_clear_flag(header_logo, LV_OBJ_FLAG_CLICKABLE);

    /* Top tabs */
    lv_obj_t *top_nav = lv_obj_create(screen);
    lv_obj_set_size(top_nav, LV_PCT(100), UI_MENU_NAVBAR_H);
    lv_obj_set_style_bg_color(top_nav, lv_color_hex(UI_MENU_NAVBAR_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top_nav, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_nav, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(top_nav, lv_color_hex(UI_MENU_NAVBAR_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(top_nav, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(top_nav, UI_MENU_NAVBAR_PAD_HOR, LV_PART_MAIN);
    lv_obj_set_style_pad_column(top_nav, UI_MENU_NAVBAR_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(top_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_nav, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(top_nav, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_home = create_top_nav_button(top_nav, LV_SYMBOL_HOME " Home");
    lv_obj_t *btn_wifi_scan = create_top_nav_button(top_nav, LV_SYMBOL_WIFI " WiFi Scan");
    lv_obj_t *btn_wifi_profile = create_top_nav_button(top_nav, LV_SYMBOL_EDIT " Profile");
    lv_obj_t *btn_back = create_top_nav_button(top_nav, LV_SYMBOL_LEFT " Back");

    /* Content */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(UI_MENU_CONTENT_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(content, lv_color_hex(UI_MENU_CONTENT_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, UI_MENU_CONTENT_PAD, LV_PART_MAIN);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *menu = lv_menu_create(content);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(menu, lv_color_hex(UI_MENU_MENU_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(menu, lv_color_hex(UI_MENU_MENU_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(menu, 0, LV_PART_MAIN);
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_mode_header(menu, LV_MENU_HEADER_TOP_FIXED);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_DISABLED);

    lv_obj_t *home_page = ui_wifi_status_page_create(menu);
    lv_obj_t *page_wifi_scan = ui_wifi_list_page_create(menu);
    lv_obj_t *page_wifi_profile = ui_wifi_profile_page_create(menu);

    lv_menu_set_page(menu, home_page);

    /* Footer */
    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_set_size(footer, LV_PCT(100), UI_MENU_FOOTER_H);
    lv_obj_set_style_bg_color(footer, lv_color_hex(UI_MENU_FOOTER_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(footer, lv_color_hex(UI_MENU_FOOTER_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(footer, UI_MENU_FOOTER_PAD_HOR, LV_PART_MAIN);
    lv_obj_set_style_pad_right(footer, UI_MENU_FOOTER_PAD_HOR, LV_PART_MAIN);
    lv_obj_set_style_pad_column(footer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *footer_status = lv_label_create(footer);
    lv_label_set_text(footer_status, "Page: Home");
    lv_obj_set_style_text_color(footer_status, lv_color_hex(UI_MENU_FOOTER_TEXT_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(footer_status, &lv_font_montserrat_14, LV_PART_MAIN);

    /* Bind navigation state and callbacks. */
    menu_nav_logic_init(&s_menu_nav_state,
                        menu,
                        home_page,
                        page_wifi_scan,
                        page_wifi_profile,
                        footer_status);

    menu_nav_logic_bind_nav_widgets(&s_menu_nav_state,
                                    btn_home,
                                    btn_wifi_scan,
                                    btn_wifi_profile);

    ui_wifi_list_page_set_use_ap_callback(ui_menu_nav_use_ap_cb, &s_menu_nav_state);

    lv_obj_add_event_cb(btn_home, menu_nav_logic_home_btn_event_cb, LV_EVENT_CLICKED, &s_menu_nav_state);
    lv_obj_add_event_cb(btn_wifi_scan, menu_nav_logic_wifi_btn_event_cb, LV_EVENT_CLICKED, &s_menu_nav_state);
    lv_obj_add_event_cb(btn_wifi_profile, menu_nav_logic_profile_btn_event_cb, LV_EVENT_CLICKED, &s_menu_nav_state);
    lv_obj_add_event_cb(btn_back, menu_nav_logic_back_btn_event_cb, LV_EVENT_CLICKED, &s_menu_nav_state);

    ui_wifi_status_page_startup_auto_connect();
}

void ui_wifi_profile_nvm_create(void)
{
    ui_wifi_manager_final_create();
}
