#include "bmm350_reader.h"

#include <math.h>

#include "bmm350_config.h"
#include "bmm350_driver.h"

#define BMM350_READER_RSLT_NOT_READY CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, 0x64A0U, 0x02U)
#define BMM350_READER_DEG_PER_RAD (57.295779513f)

typedef struct
{
    bool active;
    bool done;
    uint32_t samples;

    float x_min;
    float x_max;
    float y_min;
    float y_max;

    float offset_x;
    float offset_y;
    float scale_x;
    float scale_y;
} bmm350_calibration_t;

static bmm350_sample_t s_last_sample;
static cy_rslt_t s_last_error = CY_RSLT_SUCCESS;
static bool s_initialized = false;
static bmm350_calibration_t s_cal;

static float absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float normalize_heading(float heading_deg)
{
    while (heading_deg < 0.0f)
    {
        heading_deg += 360.0f;
    }

    while (heading_deg >= 360.0f)
    {
        heading_deg -= 360.0f;
    }

    return heading_deg;
}

static void map_heading_axes(float x_in, float y_in, float *x_out, float *y_out)
{
    float x = x_in;
    float y = y_in;

#if (BMM350_HEADING_SWAP_XY != 0U)
    float tmp = x;
    x = y;
    y = tmp;
#endif

    x *= (float)BMM350_HEADING_X_SIGN;
    y *= (float)BMM350_HEADING_Y_SIGN;

    *x_out = x;
    *y_out = y;
}

static void calibration_clear(void)
{
    s_cal.active = false;
    s_cal.done = false;
    s_cal.samples = 0U;

    s_cal.x_min = 1000000.0f;
    s_cal.x_max = -1000000.0f;
    s_cal.y_min = 1000000.0f;
    s_cal.y_max = -1000000.0f;

    s_cal.offset_x = 0.0f;
    s_cal.offset_y = 0.0f;
    s_cal.scale_x = 1.0f;
    s_cal.scale_y = 1.0f;
}

static float calibration_span_x(void)
{
    if (s_cal.x_max < s_cal.x_min)
    {
        return 0.0f;
    }

    return s_cal.x_max - s_cal.x_min;
}

static float calibration_span_y(void)
{
    if (s_cal.y_max < s_cal.y_min)
    {
        return 0.0f;
    }

    return s_cal.y_max - s_cal.y_min;
}

static bool calibration_is_coverage_ready(void)
{
    return ((calibration_span_x() >= BMM350_CALIBRATION_MIN_SPAN_UT) &&
            (calibration_span_y() >= BMM350_CALIBRATION_MIN_SPAN_UT));
}

static uint32_t calibration_sample_progress_pct(void)
{
    if (s_cal.done)
    {
        return 100U;
    }

    if (!s_cal.active)
    {
        return 0U;
    }

    if (BMM350_CALIBRATION_SAMPLES == 0U)
    {
        return 100U;
    }

    uint32_t pct = (s_cal.samples * 100U) / BMM350_CALIBRATION_SAMPLES;
    return (pct > 99U) ? 99U : pct;
}

static uint32_t calibration_coverage_progress_pct(void)
{
    if (s_cal.done)
    {
        return 100U;
    }

    if (!s_cal.active)
    {
        return 0U;
    }

    float span_x = calibration_span_x();
    float span_y = calibration_span_y();
    float min_span = (span_x < span_y) ? span_x : span_y;

    if (min_span <= 0.0f)
    {
        return 0U;
    }

    float pct_f = (min_span * 100.0f) / BMM350_CALIBRATION_MIN_SPAN_UT;
    if (pct_f >= 100.0f)
    {
        return 100U;
    }

    uint32_t pct = (uint32_t)pct_f;
    return (pct > 99U) ? 99U : pct;
}

static void calibration_update(float x_mapped, float y_mapped)
{
    if (!s_cal.active)
    {
        return;
    }

    if (x_mapped < s_cal.x_min) s_cal.x_min = x_mapped;
    if (x_mapped > s_cal.x_max) s_cal.x_max = x_mapped;
    if (y_mapped < s_cal.y_min) s_cal.y_min = y_mapped;
    if (y_mapped > s_cal.y_max) s_cal.y_max = y_mapped;

    s_cal.samples++;

    if (s_cal.samples < BMM350_CALIBRATION_SAMPLES)
    {
        return;
    }

    /* Complete calibration only when both conditions are met:
     * 1) enough samples collected and
     * 2) XY rotation span covers minimum required range.
     */
    if (!calibration_is_coverage_ready())
    {
        return;
    }

    float span_x = calibration_span_x();
    float span_y = calibration_span_y();

    if (span_x < 0.000001f)
    {
        span_x = 0.000001f;
    }

    if (span_y < 0.000001f)
    {
        span_y = 0.000001f;
    }

    s_cal.offset_x = (s_cal.x_max + s_cal.x_min) * 0.5f;
    s_cal.offset_y = (s_cal.y_max + s_cal.y_min) * 0.5f;

    /* Simple ellipse-to-circle scale correction on XY plane. */
    float mean_span = (span_x + span_y) * 0.5f;
    s_cal.scale_x = mean_span / span_x;
    s_cal.scale_y = mean_span / span_y;

    s_cal.active = false;
    s_cal.done = true;
}

static float heading_from_xy(float y_mapped, float x_mapped)
{
    float x_corr = x_mapped;
    float y_corr = y_mapped;

    if (s_cal.done)
    {
        x_corr = (x_mapped - s_cal.offset_x) * s_cal.scale_x;
        y_corr = (y_mapped - s_cal.offset_y) * s_cal.scale_y;
    }

    if ((absf(x_corr) < 0.000001f) && (absf(y_corr) < 0.000001f))
    {
        return 0.0f;
    }

    float heading_deg = atan2f(y_corr, x_corr) * BMM350_READER_DEG_PER_RAD;
    heading_deg += BMM350_HEADING_OFFSET_DEG;
    return normalize_heading(heading_deg);
}

/* Magnitude approximation to keep math light on MCU. */
static float field_strength_approx(float x_ut, float y_ut, float z_ut)
{
    float ax = absf(x_ut);
    float ay = absf(y_ut);
    float az = absf(z_ut);

    float max_v = ax;
    float min_v = ax;

    if (ay > max_v) max_v = ay;
    if (az > max_v) max_v = az;

    if (ay < min_v) min_v = ay;
    if (az < min_v) min_v = az;

    float mid_v = (ax + ay + az) - max_v - min_v;
    return max_v + (0.5f * mid_v) + (0.25f * min_v);
}

cy_rslt_t bmm350_reader_init(I3C_CORE_Type *i3c_hw, cy_stc_i3c_context_t *i3c_context)
{
    s_last_sample = (bmm350_sample_t){0};
    s_last_error = CY_RSLT_SUCCESS;

    calibration_clear();

    cy_rslt_t rslt = bmm350_driver_init(i3c_hw, i3c_context);
    s_initialized = (CY_RSLT_SUCCESS == rslt);
    return rslt;
}

bool bmm350_reader_poll(bmm350_sample_t *out_sample)
{
    if ((!s_initialized) || (!bmm350_driver_is_ready()))
    {
        s_last_error = BMM350_READER_RSLT_NOT_READY;
        return false;
    }

    bmm350_sample_t sample = {0};
    cy_rslt_t rslt = bmm350_driver_read_sample(&sample);
    if (CY_RSLT_SUCCESS != rslt)
    {
        s_last_error = rslt;
        return false;
    }

    float x_mapped;
    float y_mapped;
    map_heading_axes(sample.x_ut, sample.y_ut, &x_mapped, &y_mapped);

    calibration_update(x_mapped, y_mapped);

    sample.heading_deg = heading_from_xy(y_mapped, x_mapped);
    sample.field_strength_ut = field_strength_approx(sample.x_ut, sample.y_ut, sample.z_ut);
    sample.sample_count = s_last_sample.sample_count + 1U;

    s_last_sample = sample;
    s_last_error = CY_RSLT_SUCCESS;

    if (NULL != out_sample)
    {
        *out_sample = s_last_sample;
    }

    return true;
}

cy_rslt_t bmm350_reader_get_last_error(void)
{
    return s_last_error;
}

void bmm350_reader_start_calibration(void)
{
    calibration_clear();
    s_cal.active = true;
}

void bmm350_reader_get_calibration_status(bmm350_calibration_status_t *out_status)
{
    if (NULL == out_status)
    {
        return;
    }

    out_status->active = s_cal.active;
    out_status->done = s_cal.done;
    out_status->sample_progress_pct = calibration_sample_progress_pct();
    out_status->coverage_progress_pct = calibration_coverage_progress_pct();

    if ((!s_cal.active) || (s_cal.samples >= BMM350_CALIBRATION_SAMPLES))
    {
        out_status->remaining_samples = 0U;
    }
    else
    {
        out_status->remaining_samples = BMM350_CALIBRATION_SAMPLES - s_cal.samples;
    }
}