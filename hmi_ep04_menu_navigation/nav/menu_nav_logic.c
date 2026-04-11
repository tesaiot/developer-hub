#include "menu_nav_logic.h"
#include "ui_menu_layout.h"
#include <stdio.h>

#define MENU_NAV_LOG_ENABLE (1)

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
    case MENU_NAV_PAGE_WIFI:
        return "WiFi Manager";
    case MENU_NAV_PAGE_DISPLAY:
        return "Display Setting";
    case MENU_NAV_PAGE_DEVICE:
        return "Device Info";
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
    case MENU_NAV_PAGE_WIFI:
        return state->page_wifi;
    case MENU_NAV_PAGE_DISPLAY:
        return state->page_display;
    case MENU_NAV_PAGE_DEVICE:
        return state->page_device;
    default:
        return state->home_page;
    }
}

static void menu_nav_set_child_label_color(lv_obj_t *obj, lv_color_t text_color)
{
    if(obj == NULL) {
        return;
    }

    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            lv_obj_set_style_text_color(child, text_color, LV_PART_MAIN);
        }
    }
}

static void menu_nav_set_top_tab_style(lv_obj_t *btn, bool active)
{
    if(btn == NULL) {
        return;
    }

    lv_color_t bg = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_BG_HEX)
                           : lv_color_hex(UI_MENU_NAV_BTN_BG_HEX);
    lv_color_t border = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_BORDER_HEX)
                               : lv_color_hex(UI_MENU_NAV_BTN_BORDER_HEX);
    lv_color_t text = active ? lv_color_hex(UI_MENU_NAV_ACTIVE_TEXT_HEX)
                             : lv_color_hex(UI_MENU_NAV_BTN_TEXT_HEX);

    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, border, LV_PART_MAIN);
    menu_nav_set_child_label_color(btn, text);
}

static void menu_nav_set_sidebar_link_style(lv_obj_t *link, bool active)
{
    if(link == NULL) {
        return;
    }

    lv_color_t bg = active ? lv_color_hex(UI_MENU_SIDEBAR_ACTIVE_BG_HEX)
                           : lv_color_hex(UI_MENU_SIDEBAR_ITEM_BG_HEX);
    lv_color_t border = active ? lv_color_hex(UI_MENU_SIDEBAR_ACTIVE_BORDER_HEX)
                               : lv_color_hex(UI_MENU_SIDEBAR_ITEM_BORDER_HEX);
    lv_color_t text = active ? lv_color_hex(UI_MENU_SIDEBAR_ACTIVE_TEXT_HEX)
                             : lv_color_hex(UI_MENU_SIDEBAR_TEXT_HEX);

    lv_obj_set_style_bg_color(link, bg, LV_PART_MAIN);
    lv_obj_set_style_border_color(link, border, LV_PART_MAIN);
    menu_nav_set_child_label_color(link, text);
}

static void menu_nav_apply_active_state(menu_nav_state_t *state)
{
    if(state == NULL) {
        return;
    }

    menu_nav_set_top_tab_style(state->btn_home, state->current_page == MENU_NAV_PAGE_HOME);
    menu_nav_set_top_tab_style(state->btn_wifi, state->current_page == MENU_NAV_PAGE_WIFI);
    menu_nav_set_top_tab_style(state->btn_display, state->current_page == MENU_NAV_PAGE_DISPLAY);
    menu_nav_set_top_tab_style(state->btn_device, state->current_page == MENU_NAV_PAGE_DEVICE);

    menu_nav_set_sidebar_link_style(state->sidebar_link_wifi, state->current_page == MENU_NAV_PAGE_WIFI);
    menu_nav_set_sidebar_link_style(state->sidebar_link_display, state->current_page == MENU_NAV_PAGE_DISPLAY);
    menu_nav_set_sidebar_link_style(state->sidebar_link_device, state->current_page == MENU_NAV_PAGE_DEVICE);
}

static void menu_nav_hide_internal_headers(menu_nav_state_t *state)
{
    if(state == NULL || state->menu == NULL) {
        return;
    }

    lv_obj_t *main_header = lv_menu_get_main_header(state->menu);
    if(main_header != NULL) {
        lv_obj_add_flag(main_header, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *sidebar_header = lv_menu_get_sidebar_header(state->menu);
    if(sidebar_header != NULL) {
        lv_obj_add_flag(sidebar_header, LV_OBJ_FLAG_HIDDEN);
    }
}

static void menu_nav_update_status(menu_nav_state_t *state)
{
    if(state == NULL || state->status_label == NULL) {
        return;
    }

    lv_label_set_text_fmt(state->status_label,
                          "Page: %s | Sidebar: %s",
                          menu_nav_page_name(state->current_page),
                          state->sidebar_expanded ? "Expanded" : "Collapsed");
}

static lv_obj_t *menu_nav_find_sidebar_cont(lv_obj_t *menu)
{
    if(menu == NULL) {
        return NULL;
    }

    uint32_t child_cnt = lv_obj_get_child_count(menu);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(menu, i);
        if(lv_obj_check_type(child, &lv_menu_sidebar_cont_class)) {
            return child;
        }
    }

    return NULL;
}

static void menu_nav_apply_sidebar_state(menu_nav_state_t *state)
{
    lv_obj_t *sidebar_cont;

    if(state == NULL || state->menu == NULL || state->sidebar_page == NULL) {
        return;
    }

    /* Keep sidebar page attached; only hide/show its container on toggle. */
    lv_menu_set_sidebar_page(state->menu, state->sidebar_page);
    sidebar_cont = menu_nav_find_sidebar_cont(state->menu);

    if(state->sidebar_expanded) {
        if(sidebar_cont != NULL) {
            lv_obj_clear_flag(sidebar_cont, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_width(sidebar_cont, LV_PCT(30));
        }
        if(state->settings_icon_label != NULL) {
            lv_label_set_text(state->settings_icon_label, LV_SYMBOL_CLOSE);
        }
    } else {
        if(sidebar_cont != NULL) {
            lv_obj_set_width(sidebar_cont, 0);
            lv_obj_add_flag(sidebar_cont, LV_OBJ_FLAG_HIDDEN);
        }
        if(state->settings_icon_label != NULL) {
            lv_label_set_text(state->settings_icon_label, LV_SYMBOL_LIST);
        }
    }

    lv_obj_update_layout(state->menu);
    menu_nav_hide_internal_headers(state);
    menu_nav_update_status(state);
    menu_nav_apply_active_state(state);
}

static void menu_nav_apply_page_now(menu_nav_state_t *state, menu_nav_page_id_t page_id)
{
    lv_obj_t *target_page;

    if(state == NULL || state->menu == NULL) {
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

static void menu_nav_async_apply_pending_page(void *user_data)
{
    menu_nav_state_t *state = (menu_nav_state_t *)user_data;

    if(state == NULL || !state->page_switch_pending) {
        return;
    }

    state->page_switch_pending = false;
    menu_nav_apply_page_now(state, state->pending_page);
}

static void menu_nav_queue_page_switch(menu_nav_state_t *state, menu_nav_page_id_t page_id)
{
    if(state == NULL || state->menu == NULL) {
        return;
    }

    state->pending_page = page_id;

    if(!state->page_switch_pending) {
        state->page_switch_pending = true;
        lv_async_call(menu_nav_async_apply_pending_page, state);
    }
}

/* Keep page switching deterministic for workshop flow. */
static void menu_nav_set_page(menu_nav_state_t *state, menu_nav_page_id_t page_id, bool collapse_sidebar)
{
    if(state == NULL || state->menu == NULL) {
        return;
    }

    if(collapse_sidebar && state->sidebar_expanded) {
        state->sidebar_expanded = false;
        menu_nav_apply_sidebar_state(state);
    }

    /* Always defer actual page switching to avoid lv_menu state race after sidebar transitions. */
    menu_nav_queue_page_switch(state, page_id);
}

void menu_nav_logic_init(menu_nav_state_t *state,
                         lv_obj_t *menu,
                         lv_obj_t *home_page,
                         lv_obj_t *sidebar_page,
                         lv_obj_t *page_wifi,
                         lv_obj_t *page_display,
                         lv_obj_t *page_device,
                         lv_obj_t *settings_icon_label,
                         lv_obj_t *status_label)
{
    if(state == NULL) {
        return;
    }

    state->menu = menu;
    state->home_page = home_page;
    state->sidebar_page = sidebar_page;
    state->page_wifi = page_wifi;
    state->page_display = page_display;
    state->page_device = page_device;
    state->settings_icon_label = settings_icon_label;
    state->status_label = status_label;

    state->btn_home = NULL;
    state->btn_wifi = NULL;
    state->btn_display = NULL;
    state->btn_device = NULL;
    state->sidebar_link_wifi = NULL;
    state->sidebar_link_display = NULL;
    state->sidebar_link_device = NULL;

    state->sidebar_expanded = false;
    state->page_switch_pending = false;
    state->current_page = MENU_NAV_PAGE_HOME;
    state->pending_page = MENU_NAV_PAGE_HOME;

    menu_nav_apply_page_now(state, MENU_NAV_PAGE_HOME);
    menu_nav_log("INIT page=Home sidebar=collapsed");
}

void menu_nav_logic_bind_nav_widgets(menu_nav_state_t *state,
                                     lv_obj_t *btn_home,
                                     lv_obj_t *btn_wifi,
                                     lv_obj_t *btn_display,
                                     lv_obj_t *btn_device,
                                     lv_obj_t *sidebar_link_wifi,
                                     lv_obj_t *sidebar_link_display,
                                     lv_obj_t *sidebar_link_device)
{
    if(state == NULL) {
        return;
    }

    state->btn_home = btn_home;
    state->btn_wifi = btn_wifi;
    state->btn_display = btn_display;
    state->btn_device = btn_device;
    state->sidebar_link_wifi = sidebar_link_wifi;
    state->sidebar_link_display = sidebar_link_display;
    state->sidebar_link_device = sidebar_link_device;

    menu_nav_apply_active_state(state);
}

void menu_nav_logic_settings_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    if(state == NULL) {
        return;
    }

    state->sidebar_expanded = !state->sidebar_expanded;
    menu_nav_apply_sidebar_state(state);
    menu_nav_log(state->sidebar_expanded ? "SIDEBAR expanded" : "SIDEBAR collapsed");
}

void menu_nav_logic_home_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_set_page(state, MENU_NAV_PAGE_HOME, true);
}

void menu_nav_logic_wifi_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_set_page(state, MENU_NAV_PAGE_WIFI, true);
}

void menu_nav_logic_display_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_set_page(state, MENU_NAV_PAGE_DISPLAY, true);
}

void menu_nav_logic_device_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    menu_nav_set_page(state, MENU_NAV_PAGE_DEVICE, true);
}

void menu_nav_logic_back_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    menu_nav_state_t *state = (menu_nav_state_t *)lv_event_get_user_data(e);
    if(state == NULL) {
        return;
    }

    if(state->current_page != MENU_NAV_PAGE_HOME) {
        menu_nav_set_page(state, MENU_NAV_PAGE_HOME, true);
        menu_nav_log("BACK -> Home");
        return;
    }

    menu_nav_log("BACK ignored (already Home)");
}