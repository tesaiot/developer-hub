/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — HMI ep05: WiFi List
 *
 * This file provides a strong definition of example_main() that the master
 * template calls after LVGL and the display are initialized. It forwards the
 * call to the episode's legacy entry function.
 *
 * This episode pre-initializes the WiFi scan service so that the first scan
 * triggered from the UI does not incur a long synchronous delay.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include <stdio.h>

/* Declare the episode's existing entry function (defined in nav/ui_menu_navigation.h). */
#include "ui_menu_navigation.h"

/* WiFi scan service pre-init (defined in wifi_list/wifi_scan_service.c). */
#include "wifi_scan_service.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;   /* The episode manages its own screen composition via lv_screen_active(). */

    /* Warm up the WiFi stack before the UI triggers its first scan, so the
     * user does not see a long initial pause on the first "Scan" tap. */
    cy_rslt_t wifi_preinit_rslt = wifi_scan_service_preinit();
    if(CY_RSLT_SUCCESS != wifi_preinit_rslt) {
        printf("[WIFI_LIST] STARTUP_WIFI_INIT_FAIL rslt=0x%08lx\r\n",
               (unsigned long)wifi_preinit_rslt);
    }

    ui_wifi_list_create();
}
