#include "sht4x_reader.h"

#include "sht4x_driver.h"

#define SHT4X_READER_RSLT_NOT_READY CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0x54A0U, 0x02U)

static sht4x_sample_t s_last_sample;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_initialized = false;

cy_rslt_t sht4x_reader_init(mtb_hal_i2c_t *i2c_bus)
{
    s_last_sample = (sht4x_sample_t){0};
    s_last_error = CY_RSLT_SUCCESS;

    cy_rslt_t rslt = sht4x_driver_init(i2c_bus);
    s_initialized = (CY_RSLT_SUCCESS == rslt);
    return rslt;
}

bool sht4x_reader_poll(sht4x_sample_t *out_sample)
{
    if ((!s_initialized) || (!sht4x_driver_is_ready()))
    {
        s_last_error = SHT4X_READER_RSLT_NOT_READY;
        return false;
    }

    sht4x_sample_t sample = {0};
    cy_rslt_t rslt = sht4x_driver_read_sample(&sample);
    if (CY_RSLT_SUCCESS != rslt)
    {
        s_last_error = rslt;
        return false;
    }

    sample.sample_count = s_last_sample.sample_count + 1U;
    s_last_sample = sample;
    s_last_error = CY_RSLT_SUCCESS;

    if (NULL != out_sample)
    {
        *out_sample = s_last_sample;
    }

    return true;
}

cy_rslt_t sht4x_reader_get_last_error(void)
{
    return s_last_error;
}
