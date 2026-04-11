#ifndef UI_WIFI_PROFILE_PAGE_H
#define UI_WIFI_PROFILE_PAGE_H

#include "lvgl.h"
#include "wifi_scan_types.h"

lv_obj_t *ui_wifi_profile_page_create(lv_obj_t *menu);
void ui_wifi_profile_page_apply_ap(const wifi_scan_ap_t *ap);

#endif /* UI_WIFI_PROFILE_PAGE_H */
