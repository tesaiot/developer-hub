/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep02: BMI270 Motion Visualization
 *
 *  Reads accelerometer and gyroscope data from the Bosch BMI270 and renders
 *  a 6-axis motion visualization on the LVGL display. The sensor I2C bus is
 *  already initialized by the master template; we only pass its HAL handle
 *  to the vendored BMI270 presenter.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"

#include "bmi270/bmi270_presenter.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    bmi270_presenter_start(&sensor_i2c_controller_hal_obj);
}
