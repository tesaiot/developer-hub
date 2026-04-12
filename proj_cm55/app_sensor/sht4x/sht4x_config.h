#ifndef SHT4X_CONFIG_H
#define SHT4X_CONFIG_H

#include <stdint.h>

#include "mtb_sht4x.h"

/* Indicator demo: update sensor once per second for stable UX. */
#define SHT4X_SAMPLE_PERIOD_MS            (1000U)
#define SHT4X_SAMPLE_LOG_INTERVAL         (5U)

/* UI behavior */
#define SHT4X_BAR_USE_ANIM                (0U)

#define SHT4X_I2C_ADDR_PRIMARY            MTB_SHT40_ADDRESS_DEFAULT
#define SHT4X_I2C_ADDR_ALTERNATE          MTB_SHT40_ADDRESS_SEC

#endif /* SHT4X_CONFIG_H */
