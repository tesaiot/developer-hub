#ifndef BMM350_CONFIG_H
#define BMM350_CONFIG_H

#include <stdint.h>

#include "mtb_bmm350.h"

/* Poll period for LVGL timer (ms). */
#define BMM350_SAMPLE_PERIOD_MS                (120U)
#define BMM350_SAMPLE_LOG_INTERVAL             (8U)

/* UI behavior */
#define BMM350_AXIS_BAR_MAX_UT                 (800)

/* Runtime heading calibration (hard-iron + simple soft-iron scale)
 * At default 120 ms sample period and 140 samples = ~16.8 seconds.
 */
#define BMM350_CALIBRATION_SAMPLES             (140U)
#define BMM350_CALIBRATION_MIN_SPAN_UT         (20.0f)

/* Heading axis mapping for board orientation tuning.
 * Signs should be +1 or -1.
 */
#define BMM350_HEADING_SWAP_XY                 (0U)
#define BMM350_HEADING_X_SIGN                  (1)
#define BMM350_HEADING_Y_SIGN                  (1)
#define BMM350_HEADING_OFFSET_DEG              (0.0f)

#define BMM350_I3C_ADDR_PRIMARY                MTB_BMM350_ADDRESS_SEC
#define BMM350_I3C_ADDR_ALTERNATE              MTB_BMM350_ADDRESS_DEFAULT

#endif /* BMM350_CONFIG_H */
