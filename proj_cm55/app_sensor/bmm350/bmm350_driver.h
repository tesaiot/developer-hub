#ifndef BMM350_DRIVER_H
#define BMM350_DRIVER_H

#include <stdbool.h>

#include "cy_result.h"
#include "cy_pdl.h"

#include "bmm350_types.h"

cy_rslt_t bmm350_driver_init(I3C_CORE_Type *i3c_hw, cy_stc_i3c_context_t *i3c_context);
cy_rslt_t bmm350_driver_read_sample(bmm350_sample_t *sample);
bool bmm350_driver_is_ready(void);

#endif /* BMM350_DRIVER_H */
