/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep05: BMI270 Radar Motion View
 *
 *  A second spin on the BMI270: instead of plotting raw axes, the radar
 *  presenter projects the acceleration/rotation vector onto a polar chart,
 *  creating a "motion radar" effect. The sensor I2C bus is already up.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"

#include "radar/radar_presenter.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    radar_presenter_start(&sensor_i2c_controller_hal_obj);
}
