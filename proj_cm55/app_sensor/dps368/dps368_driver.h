#ifndef DPS368_DRIVER_H
#define DPS368_DRIVER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

cy_rslt_t dps368_driver_init(mtb_hal_i2c_t *i2c_bus);
cy_rslt_t dps368_driver_read_hpa_c(float *pressure_hpa, float *temperature_c);
bool dps368_driver_is_ready(void);

#endif /* DPS368_DRIVER_H */