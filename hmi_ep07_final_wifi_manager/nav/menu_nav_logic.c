#include "menu_nav_logic.h"

#include "ui_menu_layout.h"

#include <stdio.h>

#define MENU_NAV_LOG_ENABLE   (1)

static void menu_nav_log(const char *msg)
{
#if MENU_NAV_LOG_ENABLE
    printf("[MENU_NAV] %s\r\n", msg);
#else
    LV_UNUSED(msg);
#endif
}

static const char *menu_nav_page_name(menu_nav_page_id_t page_id)
{
    switch(page_id) {
    case MENU_NAV_PAGE_HOME:
        return "Home";
    case MENU_NAV_PAGE_WIFI_SCAN:
        return "WiFi Scan";
    case MENU_NAV_PAGE_WIFI_PROFILE:
        return "WiFi Profile";
    default:
        return "Unknown";
    }
}

static lv_obj_t *menu_nav_get_page_obj(menu_nav_state_t *state, menu_nav_page_id_t page_id)
{
    if(state == NULL) {
        return NULL;
    }

    switch(page_id) {
    case MENU_NAV_PAGE_HOME:
        return state->home_page;
    case MENU_NAV_PAGE_WIFI_SCAN:
        return state->page_wifi_scan;
    case MENU_NAV_PAGE_WIFI_PROFILE:
        return state->page_wifi_profile;
    default:
        return state->home_page;
    }
}

static void menu_nav_set_child_label_color(lv_obj_t *obj, lv_color_t text_color)
{
    uint32_t child_cnt;

    if(obj == NULL) {
        return;
    }

    child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            lv_obj_set_style_text_color(child, text_color, LV_PART_MAIN);
        }
    }
}

static void menu_nav_set_top_tab_style(lv_obj_t *btn, bool active)
{
    lv_color_t bg;
    lv_color_t border;
    lv_color_t text;

    if(btn == NULL) {
        return;
    }

    bg = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_BG_HEX)
                : lv_color_hex(UI_MENU_NAV_BTN_BG_HEX);
    border = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_BORDER_HEX)
                    : lv_color_hex(UI_MENU_NAV_BTN_BORDER_HEX);
    text = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_TEXT_HEX)
                  : lv_color_hex(UI_MENU_NAV_BTN_TEXT_HEX);

    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, border, LV_PART_MAIN);
    menu_nav_set_child_label_color(btn, text);
}

static void menu_nav_apply_active_state(menu_nav_state_t *state)
{
    if(state == NULL) {
        return;
    }

    menu_nav_set_top_tab_style(state->btn_home, state->current_page == MENU_NAV_PAGE_HOME);
    menu_nav_set_top_tab_style(state->btn_wifi_scan, state->current_page == MENU_NAV_PAGE_WIFI_SCAN);
    menu_nav_set_top_tab_style(state->btn_wifi_profile, state->current_page == MENU_NAV_PAGE_WIFI_PROFILE);
}

static void menu_nav_hide_internal_headers(menu_nav_state_t *state)
{
    lv_obj_t *main_header;

    if((state == NULL) || (state->menu == NULL)) {
        return;
    }

    main_header = lv_menu_get_main_header(state->menu);
    if(main_header != NULL) {
        lv_obj_add_flag(main_header, LV_OBJ_FLAG_HIDDEN);
    }
}

static void menu_nav_update_status(menu_nav_state_t *state)
{
    if((state == NULL) || (state->status_label == NULL)) {
        return;
    }

    lv_label_set_text_fmt(state->status_label,
                          "Page: %s",
                          menu_nav_page_name(state->current_page));
}

static void menu_nav_apply_page_now(menu_nav_state_t *state, menu_nav_page_id_t page_id)
{
    lv_obj_t *target_page;

    if((state == NULL) || (state->menu == NULL)) {
        return;
    }

    target_page = menu_nav_get_page_obj(state, page_id);
    if(target_page == NULL) {
        return;
    }

    state->current_page = page_id;
    lv_menu_set_page(state->menu, target_page);

    menu_nav_hide_internal_headers(state);
    menu_nav_update_status(state);
    menu_nav_apply_active_state(state);

#if MENU_NAV_LOG_ENABLE
    printf("[MENU_NAV] PAGE -> %s\r\n", menu_nav_page_name(page_id));
#endif
}

/* Apply page change on next LVGL cycle to avoid freeze during nested click handlers. */
static void menu_nav_async_apply_pending_page(void *user_data)
{
    menu_nav_state_t *state = (menu_nav_state_t *)user_data;

    if((state == NULL) || !state->page_switch_pending) {
        return;
    }

    state->page_switch_pending = false;
    menu_nav_apply_page_now(state, state->pending_page);
}

/* Coalesce rapid page requests so only the latest pending page is applied. */
static void menu_nav_queue_page_switch(menu_nav_state_t *state, menu_nav_page_id_t page_id)
{
    if((state == NULL) || (state->menu == NULL)) {
        return;
    }

    state->pending_page = page_id;

    if(!state->page_switch_pending) {
        state->page_switch_pending = true;
        lv_async_call(menu_nav_async_apply_pending_page, state);
    }
}

void menu_nav_logic_init(menu_nav_state_t *state,
                         lv_obj_t *menu,
                         lv_obj_t *home_page,
                         lv_obj_t *page_wifi_scan,
                         lv_obj_t *page_wifi_profile,
                         lv_obj_t *status_label)
{
    if(state == NULL) {
        return;
    }

    state->menu = menu;
    state->home_page = home_page;
    state->page_wifi_scan = page_wifi_scan;
    state->page_wifi_profile = page_wifi_profile;
    state->status_label = status_label;

    state->btn_home = NULL;
    state->btn_wifi_scan = NULL;
    state->btn_wifi_profile = NULL;

    state->page_switch_pending = false;
    state->current_page = MENU_NAV_PAGE_HOME;
    state->pending_page = MENU_NAV_PAGE_HOME;

    menu_nav_apply_page_now(state, MENU_NAV_PAGE_HOME);
    menu_nav_log("INIT page=Home");
}

void menu_nav_logic_bind_nav_widgets(menu_nav_state_t *state,
                                     lv_obj_t *btn_home,
                                     lv_obj_t *btn_wifi_scan,
                                     lv_obj_t *btn_wifi_profile)
{
    if(state == NULL) {
        return;
    }

    state->btn_home = btn_home;
    state->btn_wifi_scan = btn_wifi_scan;
    state->btn_wifi_profile = btn_wifi_profile;

    menu_nav_apply_active_state(state);
}

void menu_nav_logic_go_profile(menu_nav_state_t *state)
{
    menu_nav_queue_page_switch(state, MENU_NAV_PAGE_WIFI_PROFILE);
}

void menu_nav_logic_home_btn_event_cb(lv_event_t *e)
{
    menu_nav_state_t *state;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_queue_page_switch(state, MENU_NAV_PAGE_HOME);
}

void menu_nav_logic_wifi_btn_event_cb(lv_event_t *e)
{
    menu_nav_state_t *state;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_queue_page_switch(state, MENU_NAV_PAGE_WIFI_SCAN);
}

void menu_nav_logic_profile_btn_event_cb(lv_event_t *e)
{
    menu_nav_state_t *state;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_queue_page_switch(state, MENU_NAV_PAGE_WIFI_PROFILE);
}

void menu_nav_logic_back_btn_event_cb(lv_event_t *e)
{
    menu_nav_state_t *state;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    state = (menu_nav_state_t *)lv_event_get_user_data(e);
    if(state == NULL) {
        return;
    }

    if(state->current_page != MENU_NAV_PAGE_HOME) {
        menu_nav_queue_page_switch(state, MENU_NAV_PAGE_HOME);
        menu_nav_log("BACK -> Home");
        return;
    }

    menu_nav_log("BACK ignored (already Home)");
}


