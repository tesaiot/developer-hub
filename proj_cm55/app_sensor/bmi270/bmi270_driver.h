#ifndef BMI270_DRIVER_H
#define BMI270_DRIVER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

#include "bmi270_types.h"

cy_rslt_t bmi270_driver_init(mtb_hal_i2c_t *i2c_bus);
cy_rslt_t bmi270_driver_read_sample(bmi270_sample_t *sample);
bool bmi270_driver_is_ready(void);

#endif /* BMI270_DRIVER_H */