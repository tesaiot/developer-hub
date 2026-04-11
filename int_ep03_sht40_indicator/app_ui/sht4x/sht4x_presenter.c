#include "sht4x_presenter.h"

#include <stdbool.h>
#include <stdio.h>

#include "lvgl.h"

#include "sht4x/sht4x_config.h"
#include "sht4x/sht4x_reader.h"
#include "sht4x_view.h"

static bool s_last_read_ok = false;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_status_ready = false;
static cy_rslt_t s_last_status_error = CY_RSLT_SUCCESS;

/* LVGL timer callback: read one sample, then update UI/log with de-dup guards. */
static void sht4x_poll_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    sht4x_sample_t sample;
    bool has_new_sample = sht4x_reader_poll(&sample);

    if (has_new_sample)
    {
        sht4x_view_update_sample(&sample);

        if (!s_status_ready)
        {
            sht4x_view_set_ready();
            s_status_ready = true;
            s_last_status_error = CY_RSLT_SUCCESS;
        }

        /* Print first sample immediately, then every N samples to keep UART clean. */
        if ((!s_last_read_ok) || ((sample.sample_count % SHT4X_SAMPLE_LOG_INTERVAL) == 0U))
        {
            printf("[EP03][SHT4X] SAMPLE hum=%.1f %%RH temp=%.1f C\r\n",
                   (double)sample.humidity_rh,
                   (double)sample.temperature_c);
        }

        s_last_read_ok = true;
        s_last_error = CY_RSLT_SUCCESS;
        return;
    }

    cy_rslt_t rslt = sht4x_reader_get_last_error();
    if (CY_RSLT_SUCCESS == rslt)
    {
        return;
    }

    /* Only refresh error text when code changed to avoid UI flicker/noise. */
    if (s_last_status_error != rslt)
    {
        sht4x_view_set_read_error(rslt);
        s_last_status_error = rslt;
        s_status_ready = false;
    }

    if ((s_last_read_ok) || (s_last_error != rslt))
    {
        printf("[EP03][SHT4X] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    s_last_read_ok = false;
    s_last_error = rslt;
}

void sht4x_presenter_start(mtb_hal_i2c_t *i2c_bus)
{
    sht4x_view_create();

    cy_rslt_t init_rslt = sht4x_reader_init(i2c_bus);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        sht4x_view_set_init_failed();
        s_status_ready = false;
        s_last_status_error = init_rslt;
        return;
    }

    sht4x_view_set_ready();
    s_status_ready = true;
    s_last_status_error = CY_RSLT_SUCCESS;

    (void)lv_timer_create(sht4x_poll_sensor_cb, SHT4X_SAMPLE_PERIOD_MS, NULL);
}
