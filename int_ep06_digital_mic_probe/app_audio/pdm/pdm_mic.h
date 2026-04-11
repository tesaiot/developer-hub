#ifndef PDM_MIC_H
#define PDM_MIC_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#include "cy_result.h"

/* 10 ms frame at 16 kHz */
#define PDM_MIC_SAMPLE_RATE_HZ              (16000U)
#define PDM_MIC_FRAME_SAMPLES_PER_CHANNEL   (160U)
#define PDM_MIC_NUM_CHANNELS                (2U)

typedef struct
{
    int16_t *left;
    int16_t *right;
    uint32_t sample_count;
} pdm_mic_frame_t;

cy_rslt_t pdm_mic_init(void);
cy_rslt_t pdm_mic_get_frame(pdm_mic_frame_t *frame);
cy_rslt_t pdm_mic_deinit(void);

#if defined(__cplusplus)
}
#endif

#endif /* PDM_MIC_H */