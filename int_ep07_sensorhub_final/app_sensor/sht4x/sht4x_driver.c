#include "sht4x_driver.h"

#include <stdio.h>

#include "sht4x_config.h"
#include "mtb_sht4x.h"

#define SHT4X_DRIVER_RSLT_BAD_ARG CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0x54A0U, 0x01U)

static bool s_ready = false;
static mtb_sht40_address_t s_i2c_addr = SHT4X_I2C_ADDR_PRIMARY;
static mtb_hal_i2c_t *s_i2c_bus = NULL;

cy_rslt_t sht4x_driver_init(mtb_hal_i2c_t *i2c_bus)
{
    if (NULL == i2c_bus)
    {
        return SHT4X_DRIVER_RSLT_BAD_ARG;
    }

    s_i2c_bus = i2c_bus;

    /* Try default address first, then fallback address for board variations. */
    cy_rslt_t rslt = mtb_sht4x_init(s_i2c_bus, SHT4X_I2C_ADDR_PRIMARY);
    if (CY_RSLT_SUCCESS == rslt)
    {
        s_i2c_addr = SHT4X_I2C_ADDR_PRIMARY;
    }
    else
    {
        rslt = mtb_sht4x_init(s_i2c_bus, SHT4X_I2C_ADDR_ALTERNATE);
        if (CY_RSLT_SUCCESS != rslt)
        {
            s_ready = false;
            printf("[EP07][SHT4X] INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
            return rslt;
        }

        s_i2c_addr = SHT4X_I2C_ADDR_ALTERNATE;
    }

    s_ready = true;
    printf("[EP07][SHT4X] INIT_OK addr=0x%02X\r\n", (unsigned int)s_i2c_addr);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t sht4x_driver_read_sample(sht4x_sample_t *sample)
{
    if ((!s_ready) || (NULL == sample) || (NULL == s_i2c_bus))
    {
        return SHT4X_DRIVER_RSLT_BAD_ARG;
    }

    int32_t temp_milli_c = 0;
    int32_t hum_milli_rh = 0;

    cy_rslt_t rslt = mtb_sht4x_measure_high_precision(s_i2c_bus, &temp_milli_c, &hum_milli_rh);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Middleware returns milli-units; convert once here for UI/presenter layers. */
    sample->temperature_c = ((float)temp_milli_c) / 1000.0f;
    sample->humidity_rh = ((float)hum_milli_rh) / 1000.0f;
    return CY_RSLT_SUCCESS;
}

bool sht4x_driver_is_ready(void)
{
    return s_ready;
}

