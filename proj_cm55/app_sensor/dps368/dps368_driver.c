#include "dps368_driver.h"

#include <stdio.h>

#include "dps368_config.h"
#include "mtb_xensiv_dps3xx.h"
#include "xensiv_dps3xx.h"

#define DPS368_DRIVER_RSLT_BAD_ARG CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0xD368U, 0x01U)

static xensiv_dps3xx_t s_sensor;
static bool s_ready = false;
static xensiv_dps3xx_i2c_addr_t s_i2c_addr = DPS368_I2C_ADDR_PRIMARY;

static cy_rslt_t dps368_driver_apply_config(void)
{
    xensiv_dps3xx_config_t cfg;

    cy_rslt_t rslt = xensiv_dps3xx_get_config(&s_sensor, &cfg);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    cfg.dev_mode = XENSIV_DPS3XX_MODE_BACKGROUND_ALL;
    cfg.pressure_rate = DPS368_PRESSURE_RATE;
    cfg.temperature_rate = DPS368_TEMPERATURE_RATE;
    cfg.pressure_oversample = DPS368_PRESSURE_OVERSAMPLE;
    cfg.temperature_oversample = DPS368_TEMPERATURE_OVERSAMPLE;
    cfg.data_timeout = DPS368_DATA_TIMEOUT_MS;
    cfg.i2c_timeout = DPS368_I2C_TIMEOUT_MS;

    return xensiv_dps3xx_set_config(&s_sensor, &cfg);
}

cy_rslt_t dps368_driver_init(mtb_hal_i2c_t *i2c_bus)
{
    if (NULL == i2c_bus)
    {
        return DPS368_DRIVER_RSLT_BAD_ARG;
    }

    /* Try default I2C address first, then fallback to alternate address. */
    cy_rslt_t rslt = mtb_xensiv_dps3xx_init_i2c(&s_sensor, i2c_bus, DPS368_I2C_ADDR_PRIMARY);
    if (CY_RSLT_SUCCESS == rslt)
    {
        s_i2c_addr = DPS368_I2C_ADDR_PRIMARY;
    }
    else
    {
        rslt = mtb_xensiv_dps3xx_init_i2c(&s_sensor, i2c_bus, DPS368_I2C_ADDR_ALTERNATE);
        if (CY_RSLT_SUCCESS != rslt)
        {
            s_ready = false;
            printf("[EP01][DPS368] INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
            return rslt;
        }
        s_i2c_addr = DPS368_I2C_ADDR_ALTERNATE;
    }

    rslt = dps368_driver_apply_config();
    if (CY_RSLT_SUCCESS != rslt)
    {
        s_ready = false;
        printf("[EP01][DPS368] CONFIG_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        return rslt;
    }

    s_ready = true;
    printf("[EP01][DPS368] INIT_OK addr=0x%02X\r\n", (unsigned int)s_i2c_addr);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t dps368_driver_read_hpa_c(float *pressure_hpa, float *temperature_c)
{
    if ((!s_ready) || (NULL == pressure_hpa) || (NULL == temperature_c))
    {
        return DPS368_DRIVER_RSLT_BAD_ARG;
    }

    cy_float32_t pressure_pa = 0.0f;
    cy_float32_t temperature = 0.0f;

    cy_rslt_t rslt = xensiv_dps3xx_read(&s_sensor, &pressure_pa, &temperature);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Convert pressure from Pa to hPa to match UI unit. */
    *pressure_hpa = ((float)pressure_pa) / 100.0f;
    *temperature_c = (float)temperature;
    return CY_RSLT_SUCCESS;
}

bool dps368_driver_is_ready(void)
{
    return s_ready;
}