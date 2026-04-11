#ifndef SHT4X_VIEW_H
#define SHT4X_VIEW_H

#include "cy_result.h"

#include "sht4x/sht4x_types.h"

void sht4x_view_create(void);
void sht4x_view_set_init_failed(void);
void sht4x_view_set_ready(void);
void sht4x_view_set_read_error(cy_rslt_t rslt);
void sht4x_view_update_sample(const sht4x_sample_t *sample);

#endif /* SHT4X_VIEW_H */
