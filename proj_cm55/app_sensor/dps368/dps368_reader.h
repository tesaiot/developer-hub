#ifndef DPS368_READER_H
#define DPS368_READER_H

#include <stdbool.h>

#include "cy_result.h"
#include "mtb_hal.h"

#include "dps368_types.h"

cy_rslt_t dps368_reader_init(mtb_hal_i2c_t *i2c_bus);
bool dps368_reader_poll(dps368_sample_t *out_sample);
cy_rslt_t dps368_reader_get_last_error(void);

#endif /* DPS368_READER_H */