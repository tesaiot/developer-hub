/*******************************************************************************
 * @file    app_interface.h
 * @brief   TESAIoT Dev Kit Master — episode entry contract
 *
 *  ============================================================================
 *   ⚠️  DO NOT DELETE — MASTER TEMPLATE SYSTEM FILES  ⚠️
 *  ============================================================================
 *
 *  The files below MUST remain inside proj_cm55/apps/ at all times.
 *  They are part of the master template infrastructure, not of any episode.
 *  If you delete any of them, `make build` will fail.
 *
 *      proj_cm55/apps/
 *      ├── app_interface.h             ← THIS FILE — contract header
 *      └── _default/
 *          └── example_main_default.c  ← weak fallback (splash screen)
 *
 *  Everything ELSE inside proj_cm55/apps/ belongs to the currently installed
 *  episode and is SAFE to delete when switching episodes. The recommended
 *  way to switch episodes is to use the helper script:
 *
 *      tools/install_episode.sh <path-to-episode-folder>
 *      tools/clean_episode.sh                       # revert to default stub
 *
 *  ============================================================================
 *   EPISODE ENTRY CONTRACT
 *  ============================================================================
 *
 *  Every episode dropped into proj_cm55/apps/ MUST implement this function:
 *
 *      void example_main(lv_obj_t *parent);
 *
 *  The master template in main.c calls example_main() exactly once, after:
 *      - Board and clock initialization (cybsp_init)
 *      - Debug UART (retarget_io)
 *      - FreeRTOS scheduler is running inside the cm55_gfx_task
 *      - Sensor I2C + I3C buses are initialized (if hardware present)
 *      - VGLite GPU memory is allocated
 *      - LVGL library is initialized (lv_init, display, input device)
 *
 *  The `parent` argument is the active LVGL screen (lv_scr_act()) — a
 *  full-screen container the episode can populate with its own widgets.
 *
 *  The episode may:
 *      - Create widgets on `parent` and return (LVGL event loop keeps
 *        rendering them).
 *      - Start its own FreeRTOS tasks (e.g. sensor polling, WiFi service)
 *        before returning.
 *      - Never return if it owns the whole runtime (rare).
 *
 *  When proj_cm55/apps/ contains no episode (only app_interface.h and
 *  _default/), the weak symbol in _default/example_main_default.c is
 *  linked instead and shows a "download an episode" splash screen.
 *
 ******************************************************************************/
#ifndef TESAIOT_APP_INTERFACE_H
#define TESAIOT_APP_INTERFACE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Episode entry point.
 *
 * Called once by the master template after all infrastructure is up.
 * The active episode (the one in proj_cm55/apps/) provides a strong
 * definition of this symbol. The default weak stub is replaced at link time.
 *
 * @param parent  Full-screen LVGL container (lv_scr_act()). The episode
 *                should use this as the root of its UI tree.
 */
void example_main(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_APP_INTERFACE_H */
