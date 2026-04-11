/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — HMI ep07: Final WiFi Manager
 *
 * This file provides a strong definition of example_main() that the master
 * template calls after LVGL and the display are initialized. It forwards the
 * call to the episode's legacy entry function.
 *
 * This episode uses the higher-level connection service which owns scan,
 * connect, retry and auto-reconnect logic. The connection service must be
 * initialized before any UI is built so that the UI can attach its observer.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include <stdio.h>

/* Declare the episode's existing entry function (defined in nav/ui_menu_navigation.h). */
#include "ui_menu_navigation.h"

/* WiFi connection service (defined in wifi_conn/wifi_connection_service.c). */
#include "wifi_connection_service.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;   /* The episode manages its own screen composition via lv_screen_active(). */

    /* Pre-initialize WiFi stack + connection state machine. The service owns
     * cy_wcm, the stored profile, the retry ladder and the ping watchdog. If
     * init fails we still build the UI so the user can see the error state. */
    if(!wifi_connection_service_init()) {
        printf("[WIFI_MGR] connection_service_init failed\r\n");
    }

    ui_wifi_manager_final_create();
}
