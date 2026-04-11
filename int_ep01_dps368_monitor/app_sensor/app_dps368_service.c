#include "app_dps368_service.h"

#include <stdio.h>

#include "mtb_xensiv_dps3xx.h"
#include "xensiv_dps3xx.h"

#define APP_DPS368_RSLT_BAD_ARG CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0xD368U, 0x01U)

static xensiv_dps3xx_t s_dps368;
static bool s_ready = false;
static xensiv_dps3xx_i2c_addr_t s_i2c_addr = XENSIV_DPS3XX_I2C_ADDR_DEFAULT;

static cy_rslt_t app_dps368_configure(void)
{
    xensiv_dps3xx_config_t cfg;

    cy_rslt_t rslt = xensiv_dps3xx_get_config(&s_dps368, &cfg);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    cfg.dev_mode = XENSIV_DPS3XX_MODE_BACKGROUND_ALL;
    cfg.pressure_rate = XENSIV_DPS3XX_RATE_1;
    cfg.temperature_rate = XENSIV_DPS3XX_RATE_1;
    cfg.pressure_oversample = XENSIV_DPS3XX_OVERSAMPLE_8;
    cfg.temperature_oversample = XENSIV_DPS3XX_OVERSAMPLE_8;
    cfg.data_timeout = 100U;
    cfg.i2c_timeout = 100U;

    return xensiv_dps3xx_set_config(&s_dps368, &cfg);
}

cy_rslt_t app_dps368_service_init(mtb_hal_i2c_t *i2c_bus)
{
    cy_rslt_t rslt;

    if (NULL == i2c_bus)
    {
        return APP_DPS368_RSLT_BAD_ARG;
    }

    rslt = mtb_xensiv_dps3xx_init_i2c(&s_dps368, i2c_bus, XENSIV_DPS3XX_I2C_ADDR_DEFAULT);
    if (CY_RSLT_SUCCESS == rslt)
    {
        s_i2c_addr = XENSIV_DPS3XX_I2C_ADDR_DEFAULT;
    }
    else
    {
        rslt = mtb_xensiv_dps3xx_init_i2c(&s_dps368, i2c_bus, XENSIV_DPS3XX_I2C_ADDR_ALT);
        if (CY_RSLT_SUCCESS != rslt)
        {
            s_ready = false;
            printf("[EP01][DPS368] INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
            return rslt;
        }
        s_i2c_addr = XENSIV_DPS3XX_I2C_ADDR_ALT;
    }

    rslt = app_dps368_configure();
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

cy_rslt_t app_dps368_service_read(float *pressure_hpa, float *temperature_c)
{
    cy_float32_t pressure_pa = 0.0f;
    cy_float32_t temperature = 0.0f;

    if ((!s_ready) || (NULL == pressure_hpa) || (NULL == temperature_c))
    {
        return APP_DPS368_RSLT_BAD_ARG;
    }

    cy_rslt_t rslt = xensiv_dps3xx_read(&s_dps368, &pressure_pa, &temperature);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    *pressure_hpa = ((float)pressure_pa) / 100.0f;
    *temperature_c = (float)temperature;
    return CY_RSLT_SUCCESS;
}

bool app_dps368_service_is_ready(void)
{
    return s_ready;
}
