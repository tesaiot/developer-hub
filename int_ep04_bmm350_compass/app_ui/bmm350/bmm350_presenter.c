#include "bmm350_presenter.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lvgl.h"

#include "bmm350/bmm350_config.h"
#include "bmm350/bmm350_reader.h"
#include "bmm350_view.h"

/* Keep UI status/log transitions clean by printing only when state changes
 * or every N samples (configured by BMM350_SAMPLE_LOG_INTERVAL).
 */
static bool s_last_read_ok = false;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_status_ready = false;
static cy_rslt_t s_last_status_error = CY_RSLT_SUCCESS;

static bool s_last_cal_active = false;
static bool s_last_cal_done = false;
static uint32_t s_last_cal_sample_pct = 0xFFFFFFFFUL;
static uint32_t s_last_cal_coverage_pct = 0xFFFFFFFFUL;
static uint32_t s_last_cal_remaining_sec = 0xFFFFFFFFUL;
static bool s_overlay_visible = false;

static void bmm350_update_calibration_ui(void)
{
    bmm350_calibration_status_t status = {0};
    bmm350_reader_get_calibration_status(&status);

    uint32_t remaining_sec = 0U;
    if (status.active)
    {
        remaining_sec = (status.remaining_samples * BMM350_SAMPLE_PERIOD_MS + 999U) / 1000U;
    }

    if ((status.active != s_last_cal_active) ||
        (status.done != s_last_cal_done) ||
        (status.sample_progress_pct != s_last_cal_sample_pct) ||
        (status.coverage_progress_pct != s_last_cal_coverage_pct) ||
        (remaining_sec != s_last_cal_remaining_sec))
    {
        bmm350_view_set_calibration_status(status.active,
                                           status.done,
                                           status.sample_progress_pct,
                                           status.coverage_progress_pct,
                                           remaining_sec);
    }

    /* Overlay should block touches only while calibration is running. */
    if (status.active != s_overlay_visible)
    {
        bmm350_view_set_calibration_overlay_visible(status.active);
        s_overlay_visible = status.active;
    }

    if (status.done && !s_last_cal_done)
    {
        printf("[EP04][BMM350] CAL_DONE\r\n");
    }

    s_last_cal_active = status.active;
    s_last_cal_done = status.done;
    s_last_cal_sample_pct = status.sample_progress_pct;
    s_last_cal_coverage_pct = status.coverage_progress_pct;
    s_last_cal_remaining_sec = remaining_sec;
}

static void bmm350_calibrate_requested_cb(void *user_data)
{
    (void)user_data;

    /* Restart calibration from scratch when user presses the button. */
    bmm350_reader_start_calibration();
    printf("[EP04][BMM350] CAL_START manual\r\n");
    bmm350_update_calibration_ui();
}

static void bmm350_poll_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    bmm350_sample_t sample;
    bool has_new_sample = bmm350_reader_poll(&sample);

    if (has_new_sample)
    {
        bmm350_view_update_sample(&sample);
        bmm350_update_calibration_ui();

        if (!s_status_ready)
        {
            bmm350_view_set_ready();
            s_status_ready = true;
            s_last_status_error = CY_RSLT_SUCCESS;
        }

        if ((!s_last_read_ok) || ((sample.sample_count % BMM350_SAMPLE_LOG_INTERVAL) == 0U))
        {
            printf("[EP04][BMM350] SAMPLE x=%.1f y=%.1f z=%.1f heading=%.1f\r\n",
                   (double)sample.x_ut,
                   (double)sample.y_ut,
                   (double)sample.z_ut,
                   (double)sample.heading_deg);
        }

        s_last_read_ok = true;
        s_last_error = CY_RSLT_SUCCESS;
        return;
    }

    cy_rslt_t rslt = bmm350_reader_get_last_error();
    if (CY_RSLT_SUCCESS == rslt)
    {
        return;
    }

    /* Update error text only when error code changed to avoid LVGL churn. */
    if (s_last_status_error != rslt)
    {
        bmm350_view_set_read_error(rslt);
        s_last_status_error = rslt;
        s_status_ready = false;
    }

    if ((s_last_read_ok) || (s_last_error != rslt))
    {
        printf("[EP04][BMM350] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    s_last_read_ok = false;
    s_last_error = rslt;
}

void bmm350_presenter_start(I3C_CORE_Type *i3c_hw, cy_stc_i3c_context_t *i3c_context)
{
    bmm350_view_create();
    bmm350_view_set_calibrate_handler(bmm350_calibrate_requested_cb, NULL);

    cy_rslt_t init_rslt = bmm350_reader_init(i3c_hw, i3c_context);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        bmm350_view_set_init_failed();
        s_status_ready = false;
        s_last_status_error = init_rslt;
        return;
    }

    bmm350_view_set_ready();
    s_status_ready = true;
    s_last_status_error = CY_RSLT_SUCCESS;

    bmm350_update_calibration_ui();

    /* Periodic sensor polling runs in LVGL task context. */
    (void)lv_timer_create(bmm350_poll_sensor_cb, BMM350_SAMPLE_PERIOD_MS, NULL);
}