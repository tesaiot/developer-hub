#ifndef MIC_VIEW_H
#define MIC_VIEW_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "mic_presenter.h"

cy_rslt_t mic_view_create(void);
void mic_view_apply(const mic_presenter_sample_t *sample);

#if defined(__cplusplus)
}
#endif

#endif /* MIC_VIEW_H */