#include "dps368_reader.h"

#include "dps368_driver.h"
#include "xensiv_dps3xx.h"

#define DPS368_READER_RSLT_NOT_READY CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0xD368U, 0x02U)

static dps368_sample_t s_last_sample;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_initialized = false;

cy_rslt_t dps368_reader_init(mtb_hal_i2c_t *i2c_bus)
{
    s_last_sample.pressure_hpa = 0.0f;
    s_last_sample.temperature_c = 0.0f;
    s_last_sample.sample_count = 0U;
    s_last_error = CY_RSLT_SUCCESS;

    cy_rslt_t rslt = dps368_driver_init(i2c_bus);
    s_initialized = (CY_RSLT_SUCCESS == rslt);
    return rslt;
}

bool dps368_reader_poll(dps368_sample_t *out_sample)
{
    if ((!s_initialized) || (!dps368_driver_is_ready()))
    {
        s_last_error = DPS368_READER_RSLT_NOT_READY;
        return false;
    }

    float pressure_hpa = 0.0f;
    float temperature_c = 0.0f;
    cy_rslt_t rslt = dps368_driver_read_hpa_c(&pressure_hpa, &temperature_c);

    if (CY_RSLT_SUCCESS == rslt)
    {
        s_last_sample.pressure_hpa = pressure_hpa;
        s_last_sample.temperature_c = temperature_c;
        s_last_sample.sample_count++;
        s_last_error = CY_RSLT_SUCCESS;

        if (NULL != out_sample)
        {
            *out_sample = s_last_sample;
        }

        return true;
    }

    /* At low sample rate, polling can happen before conversion is ready. */
    if (rslt == XENSIV_DPS3XX_RSLT_ERR_DATA_NOT_READY)
    {
        s_last_error = CY_RSLT_SUCCESS;
        return false;
    }

    s_last_error = rslt;
    return false;
}

cy_rslt_t dps368_reader_get_last_error(void)
{
    return s_last_error;
}