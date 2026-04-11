#include "sensorhub_presenter.h"

#include <stdbool.h>
#include <stdio.h>

#include "lvgl.h"

#include "bmi270_config.h"
#include "bmi270_reader.h"
#include "bmm350_config.h"
#include "bmm350_reader.h"
#include "dps368_config.h"
#include "dps368_reader.h"
#include "mic_presenter.h"
#include "sensorhub_view.h"
#include "sht4x_config.h"
#include "sht4x_reader.h"

#define HUB_UI_POLL_MS                (100U)
#define HUB_STATUS_LOG_INTERVAL       (20U)
#define HUB_CALIB_LOG_STEP_PCT        (10U)

typedef struct
{
    bool started;

    bool dps_ready;
    bool sht_ready;
    bool bmi_ready;
    bool bmm_ready;
    bool mic_live;

    cy_rslt_t dps_last_error;
    cy_rslt_t sht_last_error;
    cy_rslt_t bmi_last_error;
    cy_rslt_t bmm_last_error;

    bool bmm_cal_auto_started;
    bool bmm_cal_done;
    uint32_t bmm_cal_last_sample_pct;
    uint32_t bmm_cal_last_coverage_pct;

    dps368_sample_t dps_sample;
    sht4x_sample_t sht_sample;
    bmi270_sample_t bmi_sample;
    bmm350_sample_t bmm_sample;
    mic_presenter_sample_t mic_sample;

    bool has_dps;
    bool has_sht;
    bool has_bmi;
    bool has_bmm;

    uint32_t last_mic_frame;

    uint32_t next_dps_ms;
    uint32_t next_sht_ms;
    uint32_t next_bmi_ms;
    uint32_t next_bmm_ms;

    uint32_t status_counter;
    sensorhub_page_t active_page;

    lv_timer_t *poll_timer;
} sensorhub_presenter_ctx_t;

static sensorhub_presenter_ctx_t s_ctx;

/* Convert tab enum to short text for UI footer and serial logs. */
static const char *page_to_text(sensorhub_page_t page)
{
    switch (page)
    {
        case SENSORHUB_PAGE_HOME: return "Home";
        case SENSORHUB_PAGE_ENV: return "Env";
        case SENSORHUB_PAGE_MOTION: return "Motion";
        case SENSORHUB_PAGE_COMPASS: return "Compass";
        case SENSORHUB_PAGE_AUDIO: return "Audio";
        default: return "Unknown";
    }
}

static char status_flag(bool ready)
{
    return ready ? 'Y' : '-';
}

/* Keep footer status rendering in one place. */
static void sensorhub_update_footer(void)
{
    char line[128];

    lv_snprintf(line,
                sizeof(line),
                "Page:%s | DPS:%c SHT:%c BMI:%c BMM:%c MIC:%c",
                page_to_text(s_ctx.active_page),
                status_flag(s_ctx.dps_ready),
                status_flag(s_ctx.sht_ready),
                status_flag(s_ctx.bmi_ready),
                status_flag(s_ctx.bmm_ready),
                status_flag(s_ctx.mic_live));

    sensorhub_view_set_footer_status(line);
}

static void sensorhub_refresh_bmm_calibration_ui(void)
{
    if (!s_ctx.bmm_ready)
    {
        return;
    }

    bmm350_calibration_status_t cal = {0};
    bmm350_reader_get_calibration_status(&cal);
    sensorhub_view_update_compass_calibration(cal.sample_progress_pct,
                                              cal.coverage_progress_pct,
                                              cal.remaining_samples,
                                              cal.done);
}

/* Manual calibration entry-point from Compass tab button. */
static void sensorhub_on_compass_calibrate(void *user_data)
{
    (void)user_data;

    if (!s_ctx.bmm_ready)
    {
        printf("[EP07][BMM350] MANUAL_CAL_SKIP (sensor not ready)\r\n");
        return;
    }

    bmm350_reader_start_calibration();
    s_ctx.bmm_cal_auto_started = true;
    s_ctx.bmm_cal_done = false;
    s_ctx.bmm_cal_last_sample_pct = 0U;
    s_ctx.bmm_cal_last_coverage_pct = 0U;

    sensorhub_view_set_compass_calibration_overlay(true);
    sensorhub_refresh_bmm_calibration_ui();

    printf("[EP07][BMM350] MANUAL_CAL_START\r\n");
}

static void sensorhub_on_tab(sensorhub_page_t page, void *user_data)
{
    (void)user_data;

    if (page >= SENSORHUB_PAGE_COUNT)
    {
        return;
    }

    s_ctx.active_page = page;
    sensorhub_view_set_active_page(page);
    sensorhub_update_footer();

    printf("[EP07][HUB] PAGE -> %s\r\n", page_to_text(page));
}

/* Poll DPS368 on its own sampling period and push value to Env page. */
static void sensorhub_poll_dps(uint32_t now_ms)
{
    if ((!s_ctx.dps_ready) || ((int32_t)(now_ms - s_ctx.next_dps_ms) < 0))
    {
        return;
    }

    s_ctx.next_dps_ms = now_ms + DPS368_SAMPLE_PERIOD_MS;

    dps368_sample_t sample;
    if (dps368_reader_poll(&sample))
    {
        s_ctx.dps_sample = sample;
        s_ctx.has_dps = true;
        s_ctx.dps_last_error = CY_RSLT_SUCCESS;
        sensorhub_view_update_env(&s_ctx.dps_sample, s_ctx.has_sht ? &s_ctx.sht_sample : NULL);

        if ((sample.sample_count % HUB_STATUS_LOG_INTERVAL) == 0U)
        {
            printf("[EP07][DPS368] p=%.1f hPa t=%.1f C\r\n",
                   (double)sample.pressure_hpa,
                   (double)sample.temperature_c);
        }

        return;
    }

    cy_rslt_t rslt = dps368_reader_get_last_error();
    if ((CY_RSLT_SUCCESS != rslt) && (s_ctx.dps_last_error != rslt))
    {
        printf("[EP07][DPS368] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        s_ctx.dps_last_error = rslt;
    }
}

/* Poll SHT4x on its own sampling period and push value to Env page. */
static void sensorhub_poll_sht(uint32_t now_ms)
{
    if ((!s_ctx.sht_ready) || ((int32_t)(now_ms - s_ctx.next_sht_ms) < 0))
    {
        return;
    }

    s_ctx.next_sht_ms = now_ms + SHT4X_SAMPLE_PERIOD_MS;

    sht4x_sample_t sample;
    if (sht4x_reader_poll(&sample))
    {
        s_ctx.sht_sample = sample;
        s_ctx.has_sht = true;
        s_ctx.sht_last_error = CY_RSLT_SUCCESS;
        sensorhub_view_update_env(s_ctx.has_dps ? &s_ctx.dps_sample : NULL, &s_ctx.sht_sample);

        if ((sample.sample_count % HUB_STATUS_LOG_INTERVAL) == 0U)
        {
            printf("[EP07][SHT4X] t=%.1f C rh=%.1f%%\r\n",
                   (double)sample.temperature_c,
                   (double)sample.humidity_rh);
        }

        return;
    }

    cy_rslt_t rslt = sht4x_reader_get_last_error();
    if ((CY_RSLT_SUCCESS != rslt) && (s_ctx.sht_last_error != rslt))
    {
        printf("[EP07][SHT4X] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        s_ctx.sht_last_error = rslt;
    }
}

/* Poll BMI270 and push motion data to Motion page widgets. */
static void sensorhub_poll_bmi(uint32_t now_ms)
{
    if ((!s_ctx.bmi_ready) || ((int32_t)(now_ms - s_ctx.next_bmi_ms) < 0))
    {
        return;
    }

    s_ctx.next_bmi_ms = now_ms + BMI270_SAMPLE_PERIOD_MS;

    bmi270_sample_t sample;
    if (bmi270_reader_poll(&sample))
    {
        s_ctx.bmi_sample = sample;
        s_ctx.has_bmi = true;
        s_ctx.bmi_last_error = CY_RSLT_SUCCESS;
        sensorhub_view_update_motion(&s_ctx.bmi_sample);

        if ((sample.sample_count % HUB_STATUS_LOG_INTERVAL) == 0U)
        {
            printf("[EP07][BMI270] acc=%.2f g gyr=%.1f dps\r\n",
                   (double)sample.acc_mag_g,
                   (double)sample.gyr_mag_dps);
        }

        return;
    }

    cy_rslt_t rslt = bmi270_reader_get_last_error();
    if ((CY_RSLT_SUCCESS != rslt) && (s_ctx.bmi_last_error != rslt))
    {
        printf("[EP07][BMI270] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        s_ctx.bmi_last_error = rslt;
    }
}

/* Poll BMM350 and push heading + axis values to Compass page. */
static void sensorhub_poll_bmm(uint32_t now_ms)
{
    if ((!s_ctx.bmm_ready) || ((int32_t)(now_ms - s_ctx.next_bmm_ms) < 0))
    {
        return;
    }

    s_ctx.next_bmm_ms = now_ms + BMM350_SAMPLE_PERIOD_MS;

    bmm350_sample_t sample;
    if (bmm350_reader_poll(&sample))
    {
        s_ctx.bmm_sample = sample;
        s_ctx.has_bmm = true;
        s_ctx.bmm_last_error = CY_RSLT_SUCCESS;
        sensorhub_view_update_compass(&s_ctx.bmm_sample);

        if ((sample.sample_count % HUB_STATUS_LOG_INTERVAL) == 0U)
        {
            printf("[EP07][BMM350] heading=%.1f field=%.1f\r\n",
                   (double)sample.heading_deg,
                   (double)sample.field_strength_ut);
        }

        return;
    }

    cy_rslt_t rslt = bmm350_reader_get_last_error();
    if ((CY_RSLT_SUCCESS != rslt) && (s_ctx.bmm_last_error != rslt))
    {
        printf("[EP07][BMM350] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        s_ctx.bmm_last_error = rslt;
    }
}

/* Track calibration progress and print only when progress buckets change. */
static void sensorhub_poll_bmm_calibration(void)
{
    if ((!s_ctx.bmm_ready) || (!s_ctx.bmm_cal_auto_started))
    {
        return;
    }

    bmm350_calibration_status_t cal = {0};
    bmm350_reader_get_calibration_status(&cal);
    sensorhub_view_update_compass_calibration(cal.sample_progress_pct,
                                              cal.coverage_progress_pct,
                                              cal.remaining_samples,
                                              cal.done);

    if (cal.done)
    {
        if (!s_ctx.bmm_cal_done)
        {
            s_ctx.bmm_cal_done = true;
            s_ctx.bmm_cal_last_sample_pct = cal.sample_progress_pct;
            s_ctx.bmm_cal_last_coverage_pct = cal.coverage_progress_pct;
            printf("[EP07][BMM350] AUTO_CAL_DONE sample=%lu%% coverage=%lu%%\r\n",
                   (unsigned long)cal.sample_progress_pct,
                   (unsigned long)cal.coverage_progress_pct);
        }

        return;
    }

    if (!cal.active)
    {
        return;
    }

    uint32_t sample_bucket = cal.sample_progress_pct / HUB_CALIB_LOG_STEP_PCT;
    uint32_t coverage_bucket = cal.coverage_progress_pct / HUB_CALIB_LOG_STEP_PCT;
    uint32_t prev_sample_bucket = s_ctx.bmm_cal_last_sample_pct / HUB_CALIB_LOG_STEP_PCT;
    uint32_t prev_coverage_bucket = s_ctx.bmm_cal_last_coverage_pct / HUB_CALIB_LOG_STEP_PCT;

    if ((sample_bucket != prev_sample_bucket) || (coverage_bucket != prev_coverage_bucket))
    {
        printf("[EP07][BMM350] AUTO_CAL sample=%lu%% coverage=%lu%% remain=%lu\r\n",
               (unsigned long)cal.sample_progress_pct,
               (unsigned long)cal.coverage_progress_pct,
               (unsigned long)cal.remaining_samples);
    }

    s_ctx.bmm_cal_last_sample_pct = cal.sample_progress_pct;
    s_ctx.bmm_cal_last_coverage_pct = cal.coverage_progress_pct;
}

/* Audio samples are produced by PDM logger task; latest-sample wins. */
static void sensorhub_poll_mic(void)
{
    mic_presenter_sample_t sample;
    if (!mic_presenter_get_latest_sample(&sample))
    {
        return;
    }

    if (sample.frame_count == s_ctx.last_mic_frame)
    {
        return;
    }

    s_ctx.last_mic_frame = sample.frame_count;
    s_ctx.mic_sample = sample;
    s_ctx.mic_live = true;
    sensorhub_view_update_audio(&s_ctx.mic_sample);
}

/* Single UI timer driving all sensor polling and view updates. */
static void sensorhub_poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    uint32_t now_ms = lv_tick_get();

    sensorhub_poll_dps(now_ms);
    sensorhub_poll_sht(now_ms);
    sensorhub_poll_bmi(now_ms);
    sensorhub_poll_bmm(now_ms);
    sensorhub_poll_bmm_calibration();
    sensorhub_poll_mic();

    s_ctx.status_counter++;
    if ((s_ctx.status_counter % 10U) == 0U)
    {
        sensorhub_update_footer();
    }
}

/* Print init status with result code only when init failed. */
static void sensorhub_log_init(const char *sensor_tag, bool ready, cy_rslt_t rslt)
{
    if (ready)
    {
        printf("[EP07][%s] INIT_OK\r\n", sensor_tag);
    }
    else
    {
        printf("[EP07][%s] INIT_FAIL rslt=0x%08lx\r\n",
               sensor_tag,
               (unsigned long)rslt);
    }
}

cy_rslt_t sensorhub_presenter_start(mtb_hal_i2c_t *sensor_i2c,
                                    I3C_CORE_Type *sensor_i3c,
                                    cy_stc_i3c_context_t *sensor_i3c_context)
{
    if ((sensor_i2c == NULL) || (sensor_i3c == NULL) || (sensor_i3c_context == NULL))
    {
        return CY_RSLT_TYPE_ERROR;
    }

    if (s_ctx.started)
    {
        return CY_RSLT_SUCCESS;
    }

    sensorhub_view_create();
    sensorhub_view_bind_tab_handler(sensorhub_on_tab, NULL);
    sensorhub_view_bind_compass_cal_handler(sensorhub_on_compass_calibrate, NULL);
    s_ctx.active_page = SENSORHUB_PAGE_HOME;
    sensorhub_view_set_active_page(SENSORHUB_PAGE_HOME);

    cy_rslt_t rslt = dps368_reader_init(sensor_i2c);
    s_ctx.dps_ready = (CY_RSLT_SUCCESS == rslt);
    s_ctx.dps_last_error = rslt;
    sensorhub_log_init("DPS368", s_ctx.dps_ready, rslt);

    rslt = sht4x_reader_init(sensor_i2c);
    s_ctx.sht_ready = (CY_RSLT_SUCCESS == rslt);
    s_ctx.sht_last_error = rslt;
    sensorhub_log_init("SHT4X", s_ctx.sht_ready, rslt);

    rslt = bmi270_reader_init(sensor_i2c);
    s_ctx.bmi_ready = (CY_RSLT_SUCCESS == rslt);
    s_ctx.bmi_last_error = rslt;
    sensorhub_log_init("BMI270", s_ctx.bmi_ready, rslt);

    rslt = bmm350_reader_init(sensor_i3c, sensor_i3c_context);
    s_ctx.bmm_ready = (CY_RSLT_SUCCESS == rslt);
    s_ctx.bmm_last_error = rslt;
    sensorhub_log_init("BMM350", s_ctx.bmm_ready, rslt);

    s_ctx.bmm_cal_auto_started = false;
    s_ctx.bmm_cal_done = false;
    s_ctx.bmm_cal_last_sample_pct = 0U;
    s_ctx.bmm_cal_last_coverage_pct = 0U;

    if (s_ctx.bmm_ready)
    {
        bmm350_reader_start_calibration();
        s_ctx.bmm_cal_auto_started = true;
        sensorhub_refresh_bmm_calibration_ui();
        printf("[EP07][BMM350] AUTO_CAL_START\r\n");
    }

    s_ctx.next_dps_ms = lv_tick_get();
    s_ctx.next_sht_ms = lv_tick_get();
    s_ctx.next_bmi_ms = lv_tick_get();
    s_ctx.next_bmm_ms = lv_tick_get();

    s_ctx.poll_timer = lv_timer_create(sensorhub_poll_timer_cb, HUB_UI_POLL_MS, NULL);
    if (s_ctx.poll_timer == NULL)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    sensorhub_update_footer();
    s_ctx.started = true;

    printf("[EP07][HUB] INIT page=Home\r\n");
    return CY_RSLT_SUCCESS;
}
