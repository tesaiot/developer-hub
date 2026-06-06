#ifndef SHT4X_TYPES_H
#define SHT4X_TYPES_H

#include <stdint.h>

typedef struct
{
    float temperature_c;
    float humidity_rh;
    uint32_t sample_count;
} sht4x_sample_t;

#endif /* SHT4X_TYPES_H */
