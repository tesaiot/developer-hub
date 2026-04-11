#ifndef BMM350_VIEW_H
#define BMM350_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#include "cy_result.h"

#include "bmm350/bmm350_types.h"

typedef void (*bmm350_view_calibrate_cb_t)(void *user_data);

void bmm350_view_create(void);
void bmm350_view_set_calibrate_handler(bmm350_view_calibrate_cb_t cb, void *user_data);

void bmm350_view_set_init_failed(void);
void bmm350_view_set_ready(void);
void bmm350_view_set_read_error(cy_rslt_t rslt);
void bmm350_view_set_calibration_status(bool active,
                                        bool done,
                                        uint32_t sample_progress_pct,
                                        uint32_t coverage_progress_pct,
                                        uint32_t remaining_sec);
void bmm350_view_set_calibration_overlay_visible(bool visible);
void bmm350_view_update_sample(const bmm350_sample_t *sample);

#endif /* BMM350_VIEW_H */