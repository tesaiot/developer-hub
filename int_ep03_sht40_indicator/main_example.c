/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep03: SHT4x Humidity & Temperature Indicator
 *
 *  Displays relative humidity and temperature from the Sensirion SHT4x over
 *  the shared sensor I2C bus. The master template handles all bring-up and
 *  exposes the HAL handle via sensor_bus.h.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"

#include "sht4x/sht4x_presenter.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    sht4x_presenter_start(&sensor_i2c_controller_hal_obj);
}
