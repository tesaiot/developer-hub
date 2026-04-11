/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — HMI ep04: Menu Navigation
 *
 * This file provides a strong definition of example_main() that the master
 * template calls after LVGL and the display are initialized. It forwards the
 * call to the episode's legacy entry function.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */

/* Declare the episode's existing entry function (defined in nav/ui_menu_navigation.h). */
#include "ui_menu_navigation.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;   /* The episode manages its own screen composition via lv_screen_active(). */

    ui_menu_navigation_create();
}
