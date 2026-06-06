#include "bmi270_reader.h"

#include "bmi270_driver.h"

#define BMI270_READER_RSLT_NOT_READY CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0xB270U, 0x02U)

/* Reader keeps last stable sample and error state for presenter polling. */
static bmi270_sample_t s_last_sample;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_initialized = false;

cy_rslt_t bmi270_reader_init(mtb_hal_i2c_t *i2c_bus)
{
    s_last_sample = (bmi270_sample_t){0};
    s_last_error = CY_RSLT_SUCCESS;

    cy_rslt_t rslt = bmi270_driver_init(i2c_bus);
    s_initialized = (CY_RSLT_SUCCESS == rslt);
    return rslt;
}

bool bmi270_reader_poll(bmi270_sample_t *out_sample)
{
    if ((!s_initialized) || (!bmi270_driver_is_ready()))
    {
        s_last_error = BMI270_READER_RSLT_NOT_READY;
        return false;
    }

    bmi270_sample_t sample = {0};
    cy_rslt_t rslt = bmi270_driver_read_sample(&sample);
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

cy_rslt_t bmi270_reader_get_last_error(void)
{
    return s_last_error;
}
