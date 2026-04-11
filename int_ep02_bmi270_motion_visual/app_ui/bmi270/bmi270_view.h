#ifndef BMI270_VIEW_H
#define BMI270_VIEW_H

#include <stdbool.h>

#include "cy_result.h"

#include "bmi270/bmi270_types.h"

void bmi270_view_create(void);
void bmi270_view_set_init_failed(void);
void bmi270_view_set_ready(void);
void bmi270_view_set_read_error(cy_rslt_t rslt);
void bmi270_view_update_sample(const bmi270_sample_t *sample);
void bmi270_view_set_alert(bool enabled);

#endif /* BMI270_VIEW_H */