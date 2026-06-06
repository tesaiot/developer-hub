#ifndef SHT4X_DRIVER_H
#define SHT4X_DRIVER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

#include "sht4x_types.h"

cy_rslt_t sht4x_driver_init(mtb_hal_i2c_t *i2c_bus);
cy_rslt_t sht4x_driver_read_sample(sht4x_sample_t *sample);
bool sht4x_driver_is_ready(void);

#endif /* SHT4X_DRIVER_H */
