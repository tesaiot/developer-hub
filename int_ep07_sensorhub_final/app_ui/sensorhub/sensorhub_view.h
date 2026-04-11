#ifndef SENSORHUB_VIEW_H
#define SENSORHUB_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#include "bmi270_types.h"
#include "bmm350_types.h"
#include "dps368_types.h"
#include "mic_presenter.h"
#include "sht4x_types.h"

typedef enum
{
    SENSORHUB_PAGE_HOME = 0,
    SENSORHUB_PAGE_ENV,
    SENSORHUB_PAGE_MOTION,
    SENSORHUB_PAGE_COMPASS,
    SENSORHUB_PAGE_AUDIO,
    SENSORHUB_PAGE_COUNT
} sensorhub_page_t;

typedef void (*sensorhub_view_tab_cb_t)(sensorhub_page_t page, void *user_data);
typedef void (*sensorhub_view_compass_cal_cb_t)(void *user_data);

void sensorhub_view_create(void);
void sensorhub_view_bind_tab_handler(sensorhub_view_tab_cb_t cb, void *user_data);
void sensorhub_view_bind_compass_cal_handler(sensorhub_view_compass_cal_cb_t cb, void *user_data);
void sensorhub_view_set_active_page(sensorhub_page_t page);
void sensorhub_view_set_footer_status(const char *text);
void sensorhub_view_set_compass_calibration_overlay(bool visible);
void sensorhub_view_update_compass_calibration(uint32_t sample_pct,
                                               uint32_t coverage_pct,
                                               uint32_t remaining_samples,
                                               bool done);

void sensorhub_view_update_env(const dps368_sample_t *dps, const sht4x_sample_t *sht);
void sensorhub_view_update_motion(const bmi270_sample_t *motion);
void sensorhub_view_update_compass(const bmm350_sample_t *mag);
void sensorhub_view_update_audio(const mic_presenter_sample_t *audio);

#endif /* SENSORHUB_VIEW_H */

