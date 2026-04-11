#ifndef RADAR_VIEW_H
#define RADAR_VIEW_H

#include <stdbool.h>

#include "cy_result.h"

#include "bmi270/bmi270_types.h"

typedef enum
{
    RADAR_LEVEL_LOW = 0,
    RADAR_LEVEL_MEDIUM,
    RADAR_LEVEL_HIGH
} radar_motion_level_t;

void radar_view_create(void);
void radar_view_set_init_failed(void);
void radar_view_set_ready(void);
void radar_view_set_read_error(cy_rslt_t rslt);
void radar_view_update_motion(const bmi270_sample_t *sample,
                              float angle_deg,
                              float acc_xy_delta_g,
                              float gyr_z_delta_abs_dps,
                              radar_motion_level_t level,
                              bool motion_active);

#endif /* RADAR_VIEW_H */
