#ifndef UI_WIFI_LIST_PAGE_H
#define UI_WIFI_LIST_PAGE_H

#include "lvgl.h"
#include "wifi_scan_types.h"

typedef void (*ui_wifi_list_use_ap_cb_t)(const wifi_scan_ap_t *ap, void *user_data);

lv_obj_t *ui_wifi_list_page_create(lv_obj_t *menu);
void ui_wifi_list_page_set_use_ap_callback(ui_wifi_list_use_ap_cb_t cb, void *user_data);

#endif /* UI_WIFI_LIST_PAGE_H */