#ifndef BMI270_READER_H
#define BMI270_READER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

#include "bmi270_types.h"

cy_rslt_t bmi270_reader_init(mtb_hal_i2c_t *i2c_bus);
bool bmi270_reader_poll(bmi270_sample_t *out_sample);
cy_rslt_t bmi270_reader_get_last_error(void);

#endif /* BMI270_READER_H */