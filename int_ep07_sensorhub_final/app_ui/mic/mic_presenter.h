#ifndef MIC_PRESENTER_H
#define MIC_PRESENTER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cy_result.h"

typedef struct
{
    uint32_t frame_count;
    uint32_t left_peak_abs;
    uint32_t left_avg_abs;
    uint32_t right_peak_abs;
    uint32_t right_avg_abs;
    uint32_t left_peak_tenth_pct_fs;
    uint32_t left_avg_tenth_pct_fs;
    uint32_t right_peak_tenth_pct_fs;
    uint32_t right_avg_tenth_pct_fs;
    uint32_t left_ui_pct;
    uint32_t right_ui_pct;
    int32_t balance_lr;
} mic_presenter_sample_t;

cy_rslt_t mic_presenter_start(void);
void mic_presenter_publish_sample(const mic_presenter_sample_t *sample);
bool mic_presenter_get_latest_sample(mic_presenter_sample_t *out_sample);

#if defined(__cplusplus)
}
#endif

#endif /* MIC_PRESENTER_H */
