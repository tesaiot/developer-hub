#ifndef BMM350_TYPES_H
#define BMM350_TYPES_H

#include <stdint.h>

typedef struct
{
    float x_ut;
    float y_ut;
    float z_ut;
    float temperature_c;

    float heading_deg;
    float field_strength_ut;

    uint32_t sample_count;
} bmm350_sample_t;

#endif /* BMM350_TYPES_H */
