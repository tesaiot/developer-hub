/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep07: SensorHub Finale
 *
 *  The course finale: a single dashboard that simultaneously reads
 *      - DPS368   (pressure / temperature, I2C)
 *      - SHT4x    (humidity / temperature, I2C)
 *      - BMI270   (6-axis IMU, I2C)
 *      - BMM350   (3-axis magnetometer, I3C)
 *      - PDM mic  (stereo MEMS microphones)
 *
 *  and cycles between a multi-tile sensor dashboard and a dedicated
 *  audio probe view.
 *
 *  The master template has already initialized BOTH sensor buses:
 *      - sensor_i2c_controller_hal_obj  (mtb_hal_i2c_t *) for I2C sensors
 *      - CYBSP_I3C_CONTROLLER_HW        (I3C_CORE_Type *) for BMM350
 *      - CYBSP_I3C_CONTROLLER_context   (cy_stc_i3c_context_t *)
 *
 *  See sensor_bus.h for the externs.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"

#include "sensorhub/sensorhub_presenter.h"
#include "pdm/pdm_probe_logger.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    /* Fire up the multi-sensor dashboard on both I2C and I3C buses. */
    (void)sensorhub_presenter_start(&sensor_i2c_controller_hal_obj,
                                    CYBSP_I3C_CONTROLLER_HW,
                                    &CYBSP_I3C_CONTROLLER_context);

    /* Bring the stereo PDM microphone service online. */
    (void)pdm_probe_logger_start();
}
