#include "bmm350_driver.h"

#include <stdio.h>

#include "bmm350_config.h"
#include "mtb_bmm350.h"

#define BMM350_DRIVER_RSLT_BAD_ARG CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0x64A0U, 0x01U)

static bool s_ready = false;
static mtb_bmm350_t s_sensor;
static mtb_bmm350_address_t s_i3c_addr = BMM350_I3C_ADDR_PRIMARY;

static cy_stc_i3c_device_t s_i3c_device =
{
    .staticAddress = BMM350_I3C_ADDR_PRIMARY,
};

cy_rslt_t bmm350_driver_init(I3C_CORE_Type *i3c_hw, cy_stc_i3c_context_t *i3c_context)
{
    if ((NULL == i3c_hw) || (NULL == i3c_context))
    {
        return BMM350_DRIVER_RSLT_BAD_ARG;
    }

    /* Try both known addresses so this example can run on different boards/
     * strap states without changing code.
     */
    mtb_bmm350_address_t try_addr[2] =
    {
        BMM350_I3C_ADDR_PRIMARY,
        BMM350_I3C_ADDR_ALTERNATE,
    };

    cy_rslt_t rslt = CY_RSLT_TYPE_ERROR;
    for (uint32_t i = 0U; i < 2U; ++i)
    {
        s_i3c_device.staticAddress = try_addr[i];
        rslt = mtb_bmm350_init_i3c(&s_sensor, i3c_hw, i3c_context, &s_i3c_device);
        if (CY_RSLT_SUCCESS == rslt)
        {
            s_i3c_addr = try_addr[i];
            break;
        }
    }

    if (CY_RSLT_SUCCESS != rslt)
    {
        s_ready = false;
        printf("[EP07][BMM350] INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        return rslt;
    }

    rslt = mtb_bmm350_set_odr_performance(BMM350_DATA_RATE_25HZ,
                                          BMM350_AVERAGING_4,
                                          &s_sensor);
    if (CY_RSLT_SUCCESS != rslt)
    {
        s_ready = false;
        printf("[EP07][BMM350] CFG_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        return rslt;
    }

    s_ready = true;
    printf("[EP07][BMM350] INIT_OK addr=0x%02X\r\n", (unsigned int)s_i3c_addr);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t bmm350_driver_read_sample(bmm350_sample_t *sample)
{
    if ((!s_ready) || (NULL == sample))
    {
        return BMM350_DRIVER_RSLT_BAD_ARG;
    }

    mtb_bmm350_data_t data;
    cy_rslt_t rslt = mtb_bmm350_read(&s_sensor, &data);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    sample->x_ut = data.sensor_data.x;
    sample->y_ut = data.sensor_data.y;
    sample->z_ut = data.sensor_data.z;
    sample->temperature_c = data.sensor_data.temperature;
    return CY_RSLT_SUCCESS;
}

bool bmm350_driver_is_ready(void)
{
    return s_ready;
}

