#ifndef BMM350_READER_H
#define BMM350_READER_H

#include <stdbool.h>
#include <stdint.h>

#include "cy_result.h"
#include "cy_pdl.h"

#include "bmm350_types.h"

typedef struct
{
    bool active;
    bool done;
    uint32_t sample_progress_pct;
    uint32_t coverage_progress_pct;
    uint32_t remaining_samples;
} bmm350_calibration_status_t;

cy_rslt_t bmm350_reader_init(I3C_CORE_Type *i3c_hw, cy_stc_i3c_context_t *i3c_context);
bool bmm350_reader_poll(bmm350_sample_t *out_sample);
cy_rslt_t bmm350_reader_get_last_error(void);

void bmm350_reader_start_calibration(void);
void bmm350_reader_get_calibration_status(bmm350_calibration_status_t *out_status);

#endif /* BMM350_READER_H */