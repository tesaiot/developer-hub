#include "radar_presenter.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lvgl.h"

#include "bmi270/bmi270_config.h"
#include "bmi270/bmi270_reader.h"
#include "radar_view.h"

#define RADAR_DEG_PER_RAD           (57.295779513f)
/* Collect startup baseline to suppress idle drift and false direction. */
#define RADAR_BASELINE_SAMPLES      (30U)
#define RADAR_STILL_ACC_DELTA_G     (0.06f)
#define RADAR_STILL_GYR_DELTA_DPS   (12.0f)

static bool s_last_read_ok = false;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_status_ready = false;
static cy_rslt_t s_last_status_error = CY_RSLT_SUCCESS;
static uint8_t s_ui_update_div_counter = 0U;

static bool s_baseline_ready = false;
static uint16_t s_baseline_count = 0U;
static float s_baseline_acc_x_sum = 0.0f;
static float s_baseline_acc_y_sum = 0.0f;
static float s_baseline_gyr_z_sum = 0.0f;
static float s_baseline_acc_x = 0.0f;
static float s_baseline_acc_y = 0.0f;
static float s_baseline_gyr_z = 0.0f;

static void radar_reset_baseline(void)
{
    s_baseline_ready = false;
    s_baseline_count = 0U;
    s_baseline_acc_x_sum = 0.0f;
    s_baseline_acc_y_sum = 0.0f;
    s_baseline_gyr_z_sum = 0.0f;
    s_baseline_acc_x = 0.0f;
    s_baseline_acc_y = 0.0f;
    s_baseline_gyr_z = 0.0f;
}

static float normalize_angle_deg(float angle_deg)
{
    while (angle_deg < 0.0f)
    {
        angle_deg += 360.0f;
    }

    while (angle_deg >= 360.0f)
    {
        angle_deg -= 360.0f;
    }

    return angle_deg;
}

static radar_motion_level_t radar_calc_motion_level(float acc_xy_delta_g, float gyr_z_delta_abs_dps)
{
    float acc_score = acc_xy_delta_g / 0.45f;
    float gyr_score = gyr_z_delta_abs_dps / 140.0f;
    float score = (acc_score > gyr_score) ? acc_score : gyr_score;

    if (score < 0.35f)
    {
        return RADAR_LEVEL_LOW;
    }

    if (score < 0.80f)
    {
        return RADAR_LEVEL_MEDIUM;
    }

    return RADAR_LEVEL_HIGH;
}

static void radar_update_baseline(const bmi270_sample_t *sample)
{
    if ((NULL == sample) || s_baseline_ready)
    {
        return;
    }

    /* Average first N samples to create a stable "still" reference frame. */
    s_baseline_acc_x_sum += sample->acc_g_x;
    s_baseline_acc_y_sum += sample->acc_g_y;
    s_baseline_gyr_z_sum += sample->gyr_dps_z;
    s_baseline_count++;

    if (s_baseline_count < RADAR_BASELINE_SAMPLES)
    {
        return;
    }

    s_baseline_acc_x = s_baseline_acc_x_sum / (float)s_baseline_count;
    s_baseline_acc_y = s_baseline_acc_y_sum / (float)s_baseline_count;
    s_baseline_gyr_z = s_baseline_gyr_z_sum / (float)s_baseline_count;
    s_baseline_ready = true;

    printf("[EP05][RADAR] BASELINE_READY acc=(%.3f,%.3f) gyr_z=%.3f\r\n",
           (double)s_baseline_acc_x,
           (double)s_baseline_acc_y,
           (double)s_baseline_gyr_z);
}

static void radar_poll_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    bmi270_sample_t sample;
    bool has_new_sample = bmi270_reader_poll(&sample);

    if (has_new_sample)
    {
        float acc_dx;
        float acc_dy;
        float acc_xy_delta_g;
        float gyr_z_delta_abs_dps;
        bool motion_active;
        float angle_deg;
        radar_motion_level_t level;

        radar_update_baseline(&sample);

        acc_dx = sample.acc_g_x - s_baseline_acc_x;
        acc_dy = sample.acc_g_y - s_baseline_acc_y;
        acc_xy_delta_g = sqrtf((acc_dx * acc_dx) + (acc_dy * acc_dy));
        gyr_z_delta_abs_dps = fabsf(sample.gyr_dps_z - s_baseline_gyr_z);

        /* Treat signal as STILL until delta crosses threshold from baseline. */
        motion_active = s_baseline_ready &&
                        ((acc_xy_delta_g >= RADAR_STILL_ACC_DELTA_G) ||
                         (gyr_z_delta_abs_dps >= RADAR_STILL_GYR_DELTA_DPS));

        angle_deg = motion_active ? (atan2f(acc_dy, acc_dx) * RADAR_DEG_PER_RAD) : 0.0f;
        level = motion_active ? radar_calc_motion_level(acc_xy_delta_g, gyr_z_delta_abs_dps) : RADAR_LEVEL_LOW;

        /* UI decimation hook: keep smoothness/CPU balance configurable. */
        s_ui_update_div_counter++;
        if ((s_ui_update_div_counter >= BMI270_UI_UPDATE_DIV) || (sample.sample_count <= 1U))
        {
            radar_view_update_motion(&sample,
                                     normalize_angle_deg(angle_deg),
                                     acc_xy_delta_g,
                                     gyr_z_delta_abs_dps,
                                     level,
                                     motion_active);
            s_ui_update_div_counter = 0U;
        }

        if (!s_status_ready)
        {
            radar_view_set_ready();
            s_status_ready = true;
            s_last_status_error = CY_RSLT_SUCCESS;
        }

        if ((!s_last_read_ok) || ((sample.sample_count % BMI270_SAMPLE_LOG_INTERVAL) == 0U))
        {
            if (!s_baseline_ready)
            {
                printf("[EP05][RADAR] BASELINE sample=%lu/%u\r\n",
                       (unsigned long)s_baseline_count,
                       (unsigned int)RADAR_BASELINE_SAMPLES);
            }
            else if (motion_active)
            {
                printf("[EP05][RADAR] SAMPLE angle=%.1f dacc_xy=%.3f g dgyr_z=%.1f dps\r\n",
                       (double)normalize_angle_deg(angle_deg),
                       (double)acc_xy_delta_g,
                       (double)gyr_z_delta_abs_dps);
            }
            else
            {
                printf("[EP05][RADAR] STILL dacc_xy=%.3f g dgyr_z=%.1f dps\r\n",
                       (double)acc_xy_delta_g,
                       (double)gyr_z_delta_abs_dps);
            }
        }

        s_last_read_ok = true;
        s_last_error = CY_RSLT_SUCCESS;
        return;
    }

    cy_rslt_t rslt = bmi270_reader_get_last_error();
    if (CY_RSLT_SUCCESS == rslt)
    {
        return;
    }

    if (s_last_status_error != rslt)
    {
        radar_view_set_read_error(rslt);
        s_last_status_error = rslt;
        s_status_ready = false;
    }

    if ((s_last_read_ok) || (s_last_error != rslt))
    {
        printf("[EP05][RADAR] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    s_last_read_ok = false;
    s_last_error = rslt;
}

void radar_presenter_start(mtb_hal_i2c_t *i2c_bus)
{
    radar_view_create();

    cy_rslt_t init_rslt = bmi270_reader_init(i2c_bus);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        radar_view_set_init_failed();
        s_status_ready = false;
        s_last_status_error = init_rslt;
        return;
    }

    radar_reset_baseline();
    radar_view_set_ready();
    s_status_ready = true;
    s_last_status_error = CY_RSLT_SUCCESS;
    s_ui_update_div_counter = 0U;

    (void)lv_timer_create(radar_poll_sensor_cb, BMI270_SAMPLE_PERIOD_MS, NULL);
}