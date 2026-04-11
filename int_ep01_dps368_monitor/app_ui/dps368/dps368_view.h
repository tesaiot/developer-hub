#ifndef DPS368_VIEW_H
#define DPS368_VIEW_H

#include "cy_result.h"

#include "dps368/dps368_types.h"

void dps368_view_create(void);
void dps368_view_set_init_failed(void);
void dps368_view_set_ready(void);
void dps368_view_set_read_error(cy_rslt_t rslt);
void dps368_view_update_sample(const dps368_sample_t *sample);

#endif /* DPS368_VIEW_H */