#include "bmi270_driver.h"

#include <math.h>
#include <stdio.h>

#include "bmi270_config.h"
#include "mtb_bmi270.h"

#define BMI270_DRIVER_RSLT_BAD_ARG CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0xB270U, 0x01U)

static mtb_bmi270_t s_bmi270;
static bool s_ready = false;
static mtb_bmi270_address_t s_i2c_addr = BMI270_I2C_ADDR_PRIMARY;

static float bmi270_lsb_to_g(int16_t val, float g_range, uint8_t bit_width)
{
    float half_scale = (float)(1UL << (bit_width - 1U));
    return ((float)val * g_range) / half_scale;
}

static float bmi270_lsb_to_dps(int16_t val, float dps_range, uint8_t bit_width)
{
    float half_scale = (float)(1UL << (bit_width - 1U));
    return ((float)val * dps_range) / half_scale;
}

cy_rslt_t bmi270_driver_init(mtb_hal_i2c_t *i2c_bus)
{
    if (NULL == i2c_bus)
    {
        return BMI270_DRIVER_RSLT_BAD_ARG;
    }

    /* Try primary I2C address first, then alternate for board variants. */
    cy_rslt_t rslt = mtb_bmi270_init_i2c(&s_bmi270, i2c_bus, BMI270_I2C_ADDR_PRIMARY);
    if (CY_RSLT_SUCCESS == rslt)
    {
        s_i2c_addr = BMI270_I2C_ADDR_PRIMARY;
    }
    else
    {
        rslt = mtb_bmi270_init_i2c(&s_bmi270, i2c_bus, BMI270_I2C_ADDR_ALTERNATE);
        if (CY_RSLT_SUCCESS != rslt)
        {
            s_ready = false;
            printf("[EP05][BMI270] INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
            return rslt;
        }
        s_i2c_addr = BMI270_I2C_ADDR_ALTERNATE;
    }

    rslt = mtb_bmi270_config_default(&s_bmi270);
    if (CY_RSLT_SUCCESS != rslt)
    {
        s_ready = false;
        printf("[EP05][BMI270] CONFIG_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        return rslt;
    }

    s_ready = true;
    printf("[EP05][BMI270] INIT_OK addr=0x%02X\r\n", (unsigned int)s_i2c_addr);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t bmi270_driver_read_sample(bmi270_sample_t *sample)
{
    if ((!s_ready) || (NULL == sample))
    {
        return BMI270_DRIVER_RSLT_BAD_ARG;
    }

    mtb_bmi270_data_t sensor_data;
    cy_rslt_t rslt = mtb_bmi270_read(&s_bmi270, &sensor_data);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Convert raw sensor words into physical units for UI layer. */
    sample->acc_g_x = bmi270_lsb_to_g(sensor_data.sensor_data.acc.x,
                                      BMI270_ACC_RANGE_G,
                                      s_bmi270.sensor.resolution);
    sample->acc_g_y = bmi270_lsb_to_g(sensor_data.sensor_data.acc.y,
                                      BMI270_ACC_RANGE_G,
                                      s_bmi270.sensor.resolution);
    sample->acc_g_z = bmi270_lsb_to_g(sensor_data.sensor_data.acc.z,
                                      BMI270_ACC_RANGE_G,
                                      s_bmi270.sensor.resolution);

    sample->gyr_dps_x = bmi270_lsb_to_dps(sensor_data.sensor_data.gyr.x,
                                          BMI270_GYR_RANGE_DPS,
                                          s_bmi270.sensor.resolution);
    sample->gyr_dps_y = bmi270_lsb_to_dps(sensor_data.sensor_data.gyr.y,
                                          BMI270_GYR_RANGE_DPS,
                                          s_bmi270.sensor.resolution);
    sample->gyr_dps_z = bmi270_lsb_to_dps(sensor_data.sensor_data.gyr.z,
                                          BMI270_GYR_RANGE_DPS,
                                          s_bmi270.sensor.resolution);

    sample->acc_mag_g = sqrtf((sample->acc_g_x * sample->acc_g_x) +
                              (sample->acc_g_y * sample->acc_g_y) +
                              (sample->acc_g_z * sample->acc_g_z));

    sample->gyr_mag_dps = sqrtf((sample->gyr_dps_x * sample->gyr_dps_x) +
                                (sample->gyr_dps_y * sample->gyr_dps_y) +
                                (sample->gyr_dps_z * sample->gyr_dps_z));

    return CY_RSLT_SUCCESS;
}

bool bmi270_driver_is_ready(void)
{
    return s_ready;
}
