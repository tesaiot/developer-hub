#ifndef SHT4X_READER_H
#define SHT4X_READER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

#include "sht4x_types.h"

cy_rslt_t sht4x_reader_init(mtb_hal_i2c_t *i2c_bus);
bool sht4x_reader_poll(sht4x_sample_t *out_sample);
cy_rslt_t sht4x_reader_get_last_error(void);

#endif /* SHT4X_READER_H */
