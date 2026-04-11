#ifndef MENU_NAV_LOGIC_H
#define MENU_NAV_LOGIC_H

#include "lvgl.h"
#include <stdbool.h>

typedef enum {
    MENU_NAV_PAGE_HOME = 0,
    MENU_NAV_PAGE_WIFI,
    MENU_NAV_PAGE_DISPLAY,
    MENU_NAV_PAGE_DEVICE
} menu_nav_page_id_t;

typedef struct {
    lv_obj_t *menu;
    lv_obj_t *home_page;
    lv_obj_t *sidebar_page;
    lv_obj_t *page_wifi;
    lv_obj_t *page_display;
    lv_obj_t *page_device;
    lv_obj_t *settings_icon_label;
    lv_obj_t *status_label;

    /* Widgets used for active-state highlight. */
    lv_obj_t *btn_home;
    lv_obj_t *btn_wifi;
    lv_obj_t *btn_display;
    lv_obj_t *btn_device;
    lv_obj_t *sidebar_link_wifi;
    lv_obj_t *sidebar_link_display;
    lv_obj_t *sidebar_link_device;

    bool sidebar_expanded;
    bool page_switch_pending;
    menu_nav_page_id_t current_page;
    menu_nav_page_id_t pending_page;
} menu_nav_state_t;

void menu_nav_logic_init(menu_nav_state_t *state,
                         lv_obj_t *menu,
                         lv_obj_t *home_page,
                         lv_obj_t *sidebar_page,
                         lv_obj_t *page_wifi,
                         lv_obj_t *page_display,
                         lv_obj_t *page_device,
                         lv_obj_t *settings_icon_label,
                         lv_obj_t *status_label);

void menu_nav_logic_bind_nav_widgets(menu_nav_state_t *state,
                                     lv_obj_t *btn_home,
                                     lv_obj_t *btn_wifi,
                                     lv_obj_t *btn_display,
                                     lv_obj_t *btn_device,
                                     lv_obj_t *sidebar_link_wifi,
                                     lv_obj_t *sidebar_link_display,
                                     lv_obj_t *sidebar_link_device);

void menu_nav_logic_settings_btn_event_cb(lv_event_t *e);
void menu_nav_logic_home_btn_event_cb(lv_event_t *e);
void menu_nav_logic_wifi_btn_event_cb(lv_event_t *e);
void menu_nav_logic_display_btn_event_cb(lv_event_t *e);
void menu_nav_logic_device_btn_event_cb(lv_event_t *e);
void menu_nav_logic_back_btn_event_cb(lv_event_t *e);

#endif /* MENU_NAV_LOGIC_H */
