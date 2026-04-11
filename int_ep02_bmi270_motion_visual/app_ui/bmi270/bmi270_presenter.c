#include "bmi270_presenter.h"

#include <stdbool.h>
#include <stdio.h>

#include "lvgl.h"

#include "bmi270/bmi270_config.h"
#include "bmi270/bmi270_reader.h"
#include "bmi270_view.h"

static bool s_last_read_ok = false;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_status_ready = false;
static cy_rslt_t s_last_status_error = CY_RSLT_SUCCESS;
static bool s_alert_active = false;
static uint8_t s_ui_update_div_counter = 0U;

/*
 * Hysteresis: ON threshold is higher than OFF threshold,
 * so alert does not flicker around boundary noise.
 */
static bool bmi270_should_alert(const bmi270_sample_t *sample, bool is_alert_active)
{
    if (NULL == sample)
    {
        return is_alert_active;
    }

    if (is_alert_active)
    {
        bool below_acc = (sample->acc_mag_g < BMI270_ALERT_ACC_OFF_G);
        bool below_gyr = (sample->gyr_mag_dps < BMI270_ALERT_GYR_OFF_DPS);
        return !(below_acc && below_gyr);
    }

    return ((sample->acc_mag_g >= BMI270_ALERT_ACC_ON_G) ||
            (sample->gyr_mag_dps >= BMI270_ALERT_GYR_ON_DPS));
}

static void bmi270_poll_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    bmi270_sample_t sample;
    bool has_new_sample = bmi270_reader_poll(&sample);

    if (has_new_sample)
    {
        /* Throttle UI redraw while still reading sensor each period. */
        s_ui_update_div_counter++;
        if ((s_ui_update_div_counter >= BMI270_UI_UPDATE_DIV) ||
            (sample.sample_count <= 1U))
        {
            bmi270_view_update_sample(&sample);
            s_ui_update_div_counter = 0U;
        }

        if (!s_status_ready)
        {
            bmi270_view_set_ready();
            s_status_ready = true;
            s_last_status_error = CY_RSLT_SUCCESS;
        }

        bool alert_now = bmi270_should_alert(&sample, s_alert_active);
        if (alert_now != s_alert_active)
        {
            s_alert_active = alert_now;
            bmi270_view_set_alert(alert_now);
            printf("[EP02][BMI270] ALERT %s acc=%.2f g gyro=%.1f dps\r\n",
                   alert_now ? "ON" : "OFF",
                   (double)sample.acc_mag_g,
                   (double)sample.gyr_mag_dps);
        }

        if ((!s_last_read_ok) || ((sample.sample_count % BMI270_SAMPLE_LOG_INTERVAL) == 0U))
        {
            printf("[EP02][BMI270] SAMPLE acc=(%.2f, %.2f, %.2f)g gyro=(%.1f, %.1f, %.1f)dps\r\n",
                   (double)sample.acc_g_x,
                   (double)sample.acc_g_y,
                   (double)sample.acc_g_z,
                   (double)sample.gyr_dps_x,
                   (double)sample.gyr_dps_y,
                   (double)sample.gyr_dps_z);
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

    /* Update error text only when error code changed to avoid noise. */
    if (s_last_status_error != rslt)
    {
        bmi270_view_set_read_error(rslt);
        s_last_status_error = rslt;
        s_status_ready = false;
    }

    if ((s_last_read_ok) || (s_last_error != rslt))
    {
        printf("[EP02][BMI270] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    if (s_alert_active)
    {
        s_alert_active = false;
        bmi270_view_set_alert(false);
    }

    s_last_read_ok = false;
    s_last_error = rslt;
}

void bmi270_presenter_start(mtb_hal_i2c_t *i2c_bus)
{
    bmi270_view_create();

    cy_rslt_t init_rslt = bmi270_reader_init(i2c_bus);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        bmi270_view_set_init_failed();
        s_status_ready = false;
        s_last_status_error = init_rslt;
        return;
    }

    bmi270_view_set_ready();
    s_status_ready = true;
    s_last_status_error = CY_RSLT_SUCCESS;
    s_ui_update_div_counter = 0U;

    (void)lv_timer_create(bmi270_poll_sensor_cb, BMI270_SAMPLE_PERIOD_MS, NULL);
}
