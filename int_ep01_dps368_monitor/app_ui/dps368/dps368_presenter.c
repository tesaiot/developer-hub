#include "dps368_presenter.h"

#include <stdbool.h>
#include <stdio.h>

#include "lvgl.h"

#include "dps368/dps368_config.h"
#include "dps368/dps368_reader.h"
#include "dps368_view.h"

/* Track last states to avoid repeated UI/log updates for the same condition. */
static bool s_last_read_ok = false;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_status_ready = false;
static cy_rslt_t s_last_status_error = CY_RSLT_SUCCESS;

static void dps368_poll_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    dps368_sample_t sample;
    bool has_new_sample = dps368_reader_poll(&sample);

    if (has_new_sample)
    {
        dps368_view_update_sample(&sample);

        if (!s_status_ready)
        {
            dps368_view_set_ready();
            s_status_ready = true;
            s_last_status_error = CY_RSLT_SUCCESS;
        }

        /* Keep UI realtime, but limit SAMPLE logs for readable serial output. */
        if ((!s_last_read_ok) || ((sample.sample_count % 10U) == 0U))
        {
            printf("[EP01][DPS368] SAMPLE pressure=%.2f hPa temp=%.2f C\r\n",
                   (double)sample.pressure_hpa,
                   (double)sample.temperature_c);
        }

        s_last_read_ok = true;
        s_last_error = CY_RSLT_SUCCESS;
        return;
    }

    cy_rslt_t rslt = dps368_reader_get_last_error();
    if (CY_RSLT_SUCCESS == rslt)
    {
        return;
    }

    if (s_last_status_error != rslt)
    {
        dps368_view_set_read_error(rslt);
        s_last_status_error = rslt;
        s_status_ready = false;
    }

    if ((s_last_read_ok) || (s_last_error != rslt))
    {
        printf("[EP01][DPS368] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    s_last_read_ok = false;
    s_last_error = rslt;
}

void dps368_presenter_start(mtb_hal_i2c_t *i2c_bus)
{
    dps368_view_create();

    cy_rslt_t init_rslt = dps368_reader_init(i2c_bus);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        dps368_view_set_init_failed();
        s_status_ready = false;
        s_last_status_error = init_rslt;
        return;
    }

    dps368_view_set_ready();
    s_status_ready = true;
    s_last_status_error = CY_RSLT_SUCCESS;

    /* Poll period is configured in dps368_config.h for teaching/demo tuning. */
    (void)lv_timer_create(dps368_poll_sensor_cb, DPS368_SAMPLE_PERIOD_MS, NULL);
}