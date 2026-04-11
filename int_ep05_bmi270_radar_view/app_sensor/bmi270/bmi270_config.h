#ifndef BMI270_CONFIG_H
#define BMI270_CONFIG_H

#include <stdint.h>

#include "mtb_bmi270.h"

/* Sensor polling period used by presenter timer (milliseconds). */
#define BMI270_SAMPLE_PERIOD_MS           (50U)
/* Log every N samples to keep console readable. */
#define BMI270_SAMPLE_LOG_INTERVAL        (80U)

/* Update LVGL widgets every N samples (1 = update every sample). */
#define BMI270_UI_UPDATE_DIV              (1U)

/* mtb_bmi270_config_default() uses ACC=+-2g and GYR=+-2000dps by default. */
#define BMI270_ACC_RANGE_G                (2.0f)
#define BMI270_GYR_RANGE_DPS              (2000.0f)

#define BMI270_I2C_ADDR_PRIMARY           MTB_BMI270_ADDRESS_DEFAULT
#define BMI270_I2C_ADDR_ALTERNATE         MTB_BMI270_ADDRESS_SEC

#endif /* BMI270_CONFIG_H */