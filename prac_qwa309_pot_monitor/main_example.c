/*******************************************************************************
 * @file    main_example.c
 * @brief   Practise entry wrapper — QWA309: 4-channel potentiometer monitor.
 *
 * Provides the strong definition of example_main() that the master template
 * calls once, after LVGL + display + touch are up. It forwards to the UI
 * module, which self-initializes the AUTANALOG SAR ADC (P15.4-P15.7) — the
 * master framework does NOT bring up the SAR, so this practise owns it.
 *
 * BUILD: install into proj_cm55/apps/ and build with
 *            make build BOARD=TESAIOT_DEV_KIT
 *        The SAR config (autonomous_analog_init, CYBSP_SAR_ADC_gpio_ch_cfg)
 *        is supplied by the BSP design.modus and is already present in the
 *        APP_KIT_PSE84_AI GeneratedSource, so this links without a Makefile
 *        or design.modus edit. Runtime channel mapping (that GPIO4-7 == the
 *        four pots) must be confirmed on QWA309 hardware — see PILOT.md.
 ******************************************************************************/
#include "app_interface.h"
#include "pot_monitor_ui.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;   /* pot_monitor_ui_create() composes on lv_screen_active(). */

    pot_monitor_ui_create();
}
