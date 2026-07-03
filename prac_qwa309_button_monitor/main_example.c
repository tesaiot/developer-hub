/*******************************************************************************
 * @file    main_example.c
 * @brief   Practise entry wrapper — QWA309: two external push-button monitor.
 *
 * Reads SW9 (P17.5) and SW10 (P17.7), active-low with internal pull-ups, and
 * shows their state on an LVGL screen. Buttons are GPIO — the UI module inits
 * them itself; no framework/design.modus support is required.
 *
 * BUILD: install into proj_cm55/apps/ and build with tools_3.8:
 *            make build BOARD=TESAIOT_DEV_KIT
 ******************************************************************************/
#include "app_interface.h"
#include "button_monitor_ui.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;   /* button_monitor_ui_create() composes on lv_screen_active(). */
    button_monitor_ui_create();
}
