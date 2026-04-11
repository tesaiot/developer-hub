/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep01: DPS368 Pressure & Temperature Monitor
 *
 *  The master template's main.c has already brought the platform up:
 *      - Clocks, retarget_io, VGLite, LVGL, touch
 *      - Sensor I2C controller (SCB0 on P8.0/P8.1, 1.8 V domain)
 *
 *  This episode only needs to reach into the shared bus via sensor_bus.h
 *  and hand the HAL handle to the vendored DPS368 presenter.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"    /* extern handles to sensor I2C / I3C buses */

#include "dps368/dps368_presenter.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    /* Start the DPS368 presenter on the pre-initialized sensor I2C bus. */
    dps368_presenter_start(&sensor_i2c_controller_hal_obj);
}
