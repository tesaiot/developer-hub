#ifndef BMI270_CONFIG_H
#define BMI270_CONFIG_H

#include <stdint.h>

#include "mtb_bmi270.h"

/* Poll slower to reduce redraw pressure on small HMI panel. */
#define BMI270_SAMPLE_PERIOD_MS           (200U)
#define BMI270_SAMPLE_LOG_INTERVAL        (20U)

/* Update LVGL widgets every N samples to reduce visible flicker. */
#define BMI270_UI_UPDATE_DIV              (2U)

/* Visual options for teaching demo. */
#define BMI270_BAR_USE_ANIM               (0U)
#define BMI270_ALERT_FADE_ENABLE          (0U)

/* mtb_bmi270_config_default() uses ACC=+-2g and GYR=+-2000dps by default. */
#define BMI270_ACC_RANGE_G                (2.0f)
#define BMI270_GYR_RANGE_DPS              (2000.0f)

/* Motion thresholds with hysteresis to avoid ON/OFF toggling noise. */
#define BMI270_ALERT_ACC_ON_G             (1.45f)
#define BMI270_ALERT_ACC_OFF_G            (1.25f)
#define BMI270_ALERT_GYR_ON_DPS           (280.0f)
#define BMI270_ALERT_GYR_OFF_DPS          (220.0f)

#define BMI270_I2C_ADDR_PRIMARY           MTB_BMI270_ADDRESS_DEFAULT
#define BMI270_I2C_ADDR_ALTERNATE         MTB_BMI270_ADDRESS_SEC

#endif /* BMI270_CONFIG_H */
