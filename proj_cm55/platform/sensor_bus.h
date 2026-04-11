/*******************************************************************************
 * @file    sensor_bus.h
 * @brief   Public handles to the sensor I2C / I3C buses owned by main.c.
 *
 *  The master template initializes two dedicated buses before handing
 *  control to the episode's example_main():
 *
 *      1. Sensor I2C bus  — SCB0 on P8.0/P8.1 (1.8 V domain)
 *         For DPS368, SHT4x, BMI270 and any other I2C sensors on the dev kit.
 *
 *      2. Sensor I3C bus  — CYBSP_I3C_CONTROLLER
 *         For BMM350 magnetometer only (I3C targets).
 *
 *  An episode that uses sensors simply #include's this header and passes
 *  the handles to its vendored driver. The master main.c has already
 *  configured the SCB hardware, enabled the IRQ, and wrapped the context
 *  in an mtb_hal_i2c_t object — no further setup is required in the
 *  episode's example_main().
 *
 *  WiFi-only or display-only episodes can ignore this header entirely.
 ******************************************************************************/
#ifndef TESAIOT_SENSOR_BUS_H
#define TESAIOT_SENSOR_BUS_H

#include "cy_pdl.h"
#include "mtb_hal.h"
#include "cybsp.h"   /* exposes CYBSP_I3C_CONTROLLER_HW + _IRQ + _config, SCB0 macros, etc. */

#ifdef __cplusplus
extern "C" {
#endif

/* Sensor I2C — SCB0 on P8.0/P8.1 */
extern cy_stc_scb_i2c_context_t sensor_i2c_controller_context;
extern mtb_hal_i2c_t             sensor_i2c_controller_hal_obj;

/* Sensor I3C — CYBSP_I3C_CONTROLLER (BMM350 only) */
extern cy_stc_i3c_context_t CYBSP_I3C_CONTROLLER_context;

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_SENSOR_BUS_H */
