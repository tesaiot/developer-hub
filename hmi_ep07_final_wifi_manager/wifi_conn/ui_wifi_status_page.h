#ifndef UI_WIFI_STATUS_PAGE_H
#define UI_WIFI_STATUS_PAGE_H

#include "lvgl.h"

lv_obj_t *ui_wifi_status_page_create(lv_obj_t *menu);
void ui_wifi_status_page_startup_auto_connect(void);

#endif /* UI_WIFI_STATUS_PAGE_H */
