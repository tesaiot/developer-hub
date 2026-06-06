#ifndef BMI270_TYPES_H
#define BMI270_TYPES_H

#include <stdint.h>

typedef struct
{
    float acc_g_x;
    float acc_g_y;
    float acc_g_z;
    float gyr_dps_x;
    float gyr_dps_y;
    float gyr_dps_z;
    float acc_mag_g;
    float gyr_mag_dps;
    uint32_t sample_count;
} bmi270_sample_t;

#endif /* BMI270_TYPES_H */