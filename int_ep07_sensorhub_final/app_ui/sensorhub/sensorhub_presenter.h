#ifndef SENSORHUB_PRESENTER_H
#define SENSORHUB_PRESENTER_H

#include "cy_pdl.h"
#include "cy_result.h"
#include "mtb_hal.h"

cy_rslt_t sensorhub_presenter_start(mtb_hal_i2c_t *sensor_i2c,
                                    I3C_CORE_Type *sensor_i3c,
                                    cy_stc_i3c_context_t *sensor_i3c_context);

#endif /* SENSORHUB_PRESENTER_H */
