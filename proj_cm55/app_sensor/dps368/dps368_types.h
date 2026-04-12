#ifndef DPS368_TYPES_H
#define DPS368_TYPES_H

#include <stdint.h>

typedef struct
{
    float pressure_hpa;
    float temperature_c;
    uint32_t sample_count;
} dps368_sample_t;

#endif /* DPS368_TYPES_H */