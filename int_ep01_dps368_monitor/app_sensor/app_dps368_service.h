#ifndef APP_DPS368_SERVICE_H
#define APP_DPS368_SERVICE_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

cy_rslt_t app_dps368_service_init(mtb_hal_i2c_t *i2c_bus);
cy_rslt_t app_dps368_service_read(float *pressure_hpa, float *temperature_c);
bool app_dps368_service_is_ready(void);

#endif /* APP_DPS368_SERVICE_H */
