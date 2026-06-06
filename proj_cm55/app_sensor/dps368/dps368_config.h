#ifndef DPS368_CONFIG_H
#define DPS368_CONFIG_H

#include <stdint.h>

#include "xensiv_dps3xx.h"

/* UI poll period (also controls screen refresh cadence). */
#define DPS368_SAMPLE_PERIOD_MS          (1000U)

/* Sensor conversion profile used at init time. */
#define DPS368_PRESSURE_RATE             XENSIV_DPS3XX_RATE_1
#define DPS368_TEMPERATURE_RATE          XENSIV_DPS3XX_RATE_1
#define DPS368_PRESSURE_OVERSAMPLE       XENSIV_DPS3XX_OVERSAMPLE_8
#define DPS368_TEMPERATURE_OVERSAMPLE    XENSIV_DPS3XX_OVERSAMPLE_8
#define DPS368_DATA_TIMEOUT_MS           (100U)
#define DPS368_I2C_TIMEOUT_MS            (100U)

/* Try primary address first, fallback to alternate if needed. */
#define DPS368_I2C_ADDR_PRIMARY          XENSIV_DPS3XX_I2C_ADDR_DEFAULT
#define DPS368_I2C_ADDR_ALTERNATE        XENSIV_DPS3XX_I2C_ADDR_ALT

#endif /* DPS368_CONFIG_H */