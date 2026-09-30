/*******************************************************************************
 * File Name: main_example.c
 *
 * Description: Production-derived sensor dashboard with autoscale charts and
 *              change-gated rendering. Adapted from page_dashboard.c (90%+
 *              fidelity) for the Developer Hub example framework.
 *
 *              Layout: sensor cards row + dual motion charts + status bar.
 *              BSP-guarded: compiles on AI Kit (all sensors) and Eva Kit
 *              (BMI270 + CapSense only, no DPS368/SHT40).
 *
 * Key patterns demonstrated:
 *   - Module-static context struct for all widget pointers
 *   - autoscale_chart(): scans data arrays, adjusts Y range dynamically
 *   - Change-gated rendering: only updates labels when data actually changed
 *   - BSP conditional compilation (#if BSP_HAS_xxx)
 *   - Derived quantities: altitude, dew point, heat index, comfort level
 *
 ******************************************************************************/

#include "pse84_common.h"
#include <limits.h>

/*******************************************************************************
 * Constants - chart and layout
 ******************************************************************************/
#define CHART_POINTS         36     /* Data points per series                 */
#define CHART_HEIGHT        110     /* Chart widget height in pixels          */
#define SENSOR_CARD_H        90     /* Sensor card height                    */
#define UPDATE_PERIOD_MS    100     /* Timer period (10 Hz)                  */

/* Chart series colors (from production tesaiot_ui_theme.h accent palette) */
#define COLOR_ACCEL_X    0xE85B5B   /* Red                                   */
#define COLOR_ACCEL_Y    0x50D890   /* Green                                 */
#define COLOR_ACCEL_Z    0x448AFF   /* Blue                                  */
#define COLOR_GYRO_X     0xF2B84B   /* Orange                                */
#define COLOR_GYRO_Y     0xBB86FC   /* Purple                                */
#define COLOR_GYRO_Z     0xE040FB   /* Magenta                               */

/* Chart background surface */
#define COLOR_CHART_BG   0x1A2A4A

/* Count sensor cards at compile time for even-width distribution */
#define _CARD_COUNT  (1 /* BMI270 always */          \
    + (BSP_HAS_DPS368)                               \
    + (BSP_HAS_SHT40)                                \
    + (BSP_HAS_CAPSENSE))

/*******************************************************************************
 * Module-Static Context - all widget pointers in a single struct
 ******************************************************************************/
typedef struct {
    /* Sensor labels */
    lv_obj_t *imu_label;
    lv_obj_t *baro_label;
    lv_obj_t *env_label;
    lv_obj_t *controls_label;
    lv_obj_t *status_label;

    /* Charts */
    lv_obj_t *accel_chart;
    lv_obj_t *gyro_chart;

    /* Chart series */
    lv_chart_series_t *ax_ser;
    lv_chart_series_t *ay_ser;
    lv_chart_series_t *az_ser;
    lv_chart_series_t *gx_ser;
    lv_chart_series_t *gy_ser;
    lv_chart_series_t *gz_ser;

    /* Statistics */
    uint32_t update_count;
} dashboard_ctx_t;

static dashboard_ctx_t s_ctx;

/*******************************************************************************
 * Static text buffers (avoid heap allocation in timer callback)
 ******************************************************************************/
static char s_imu_buf[128];
#if BSP_HAS_DPS368
static char s_baro_buf[196];
#endif
#if BSP_HAS_SHT40
static char s_env_buf[96];
#endif
#if BSP_HAS_CAPSENSE
static char s_controls_buf[96];
#endif
static char s_status_buf[64];

/*******************************************************************************
 * autoscale_chart - scan 3 series data arrays and adjust Y-axis range
 *
 * Finds the global min/max across all data points in all 3 series, then sets
 * the chart Y range with 10% padding on each side. If the actual data range
 * is smaller than 2 * min_half_range, it clamps to that minimum span centered
 * on the midpoint. This prevents a flat line from collapsing the axis.
 *
 * @param chart          LVGL chart object
 * @param s1, s2, s3     The three chart series to scan
 * @param min_half_range Minimum half-range to prevent axis collapse
 ******************************************************************************/
static void autoscale_chart(lv_obj_t *chart,
                            lv_chart_series_t *s1,
                            lv_chart_series_t *s2,
                            lv_chart_series_t *s3,
                            int32_t min_half_range)
{
    if (!chart || !s1 || !s2 || !s3) return;

    uint32_t cnt = lv_chart_get_point_count(chart);
    int32_t *d1 = lv_chart_get_y_array(chart, s1);
    int32_t *d2 = lv_chart_get_y_array(chart, s2);
    int32_t *d3 = lv_chart_get_y_array(chart, s3);
    if (!d1 || !d2 || !d3) return;

    int32_t lo = INT32_MAX;
    int32_t hi = INT32_MIN;

    for (uint32_t i = 0; i < cnt; i++) {
        /* Skip uninitialized points (LVGL marks them as NONE) */
        if (d1[i] != LV_CHART_POINT_NONE) {
            if (d1[i] < lo) lo = d1[i];
            if (d1[i] > hi) hi = d1[i];
        }
        if (d2[i] != LV_CHART_POINT_NONE) {
            if (d2[i] < lo) lo = d2[i];
            if (d2[i] > hi) hi = d2[i];
        }
        if (d3[i] != LV_CHART_POINT_NONE) {
            if (d3[i] < lo) lo = d3[i];
            if (d3[i] > hi) hi = d3[i];
        }
    }

    /* No valid data yet */
    if (lo == INT32_MAX) return;

    /* Enforce minimum span to prevent flat-line axis collapse */
    int32_t mid = (hi + lo) / 2;
    if ((hi - lo) < min_half_range * 2) {
        lo = mid - min_half_range;
        hi = mid + min_half_range;
    }

    /* Add 10% padding on each side */
    int32_t pad = (hi - lo) / 10;
    if (pad < 1) pad = 1;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, lo - pad, hi + pad);
}

/*******************************************************************************
 * style_chart - configure chart appearance (line type, no dots, dark bg)
 ******************************************************************************/
static void style_chart(lv_obj_t *chart)
{
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -200, 200);
    lv_obj_set_width(chart, LV_PCT(100));
    lv_obj_set_height(chart, CHART_HEIGHT);
    lv_obj_set_style_bg_color(chart, lv_color_hex(COLOR_CHART_BG), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 4, 0);
    lv_obj_set_style_radius(chart, 8, 0);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);  /* No dot markers */
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
}

/*******************************************************************************
 * make_sensor_card - create a titled card with accent border and value label
 ******************************************************************************/
static lv_obj_t *make_sensor_card(lv_obj_t *parent, const char *title,
                                  int w, lv_color_t accent,
                                  lv_obj_t **out_label)
{
    lv_obj_t *card = example_card_create(parent, w, SENSOR_CARD_H,
                                         UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(card, 2, 0);

    /* Card title */
    example_label_create(card, title, &lv_font_montserrat_14, accent);

    /* Value label - caller updates this in timer callback */
    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, "---");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);

    if (out_label) *out_label = lbl;
    return card;
}

/*******************************************************************************
 * Timer callback - 10 Hz sensor update with change-gated rendering
 ******************************************************************************/
static void dashboard_timer_cb(lv_timer_t *t)
{
    (void)t;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    bool any_update = false;

    /*
     * BMI270 - Accelerometer + Gyroscope
     * Change-gated: only touch widgets when bmi270_changed is set.
     * The snapshot clears change flags after read, so consecutive calls
     * without new IPC data will skip this block entirely.
     */
    if (snap.has_bmi270 && snap.bmi270_changed && s_ctx.imu_label) {
        double ax = (double)snap.bmi270.ax / 16384.0 * 9.81;
        double ay = (double)snap.bmi270.ay / 16384.0 * 9.81;
        double az = (double)snap.bmi270.az / 16384.0 * 9.81;

        snprintf(s_imu_buf, sizeof(s_imu_buf),
                 "Accel (m/s2)\n"
                 "X:%+5.2f Y:%+5.2f Z:%+5.2f",
                 ax, ay, az);
        lv_label_set_text(s_ctx.imu_label, s_imu_buf);

        /* Feed accelerometer chart (scale to centi-m/s2 for integer chart) */
        if (s_ctx.accel_chart && s_ctx.ax_ser) {
            int32_t ax_val = (int32_t)((float)snap.bmi270.ax / 16384.0f * 981.0f);
            int32_t ay_val = (int32_t)((float)snap.bmi270.ay / 16384.0f * 981.0f);
            int32_t az_val = (int32_t)((float)snap.bmi270.az / 16384.0f * 981.0f);
            lv_chart_set_next_value(s_ctx.accel_chart, s_ctx.ax_ser, ax_val);
            lv_chart_set_next_value(s_ctx.accel_chart, s_ctx.ay_ser, ay_val);
            lv_chart_set_next_value(s_ctx.accel_chart, s_ctx.az_ser, az_val);
            autoscale_chart(s_ctx.accel_chart,
                            s_ctx.ax_ser, s_ctx.ay_ser, s_ctx.az_ser, 180);
        }

        /* Feed gyroscope chart (scale to deci-dps for integer chart) */
        if (s_ctx.gyro_chart && s_ctx.gx_ser) {
            int32_t gx_val = (int32_t)((float)snap.bmi270.gx / 16.4f * 10.0f);
            int32_t gy_val = (int32_t)((float)snap.bmi270.gy / 16.4f * 10.0f);
            int32_t gz_val = (int32_t)((float)snap.bmi270.gz / 16.4f * 10.0f);
            lv_chart_set_next_value(s_ctx.gyro_chart, s_ctx.gx_ser, gx_val);
            lv_chart_set_next_value(s_ctx.gyro_chart, s_ctx.gy_ser, gy_val);
            lv_chart_set_next_value(s_ctx.gyro_chart, s_ctx.gz_ser, gz_val);
            autoscale_chart(s_ctx.gyro_chart,
                            s_ctx.gx_ser, s_ctx.gy_ser, s_ctx.gz_ser, 100);
        }
        any_update = true;
    }

    /*
     * DPS368 - Barometric Pressure (AI Kit only)
     * Derives: altitude (hypsometric), dew point (Magnus), heat index, comfort.
     */
#if BSP_HAS_DPS368
    if (snap.has_dps368 && snap.dps368_changed && s_ctx.baro_label) {
        double pressure = (double)snap.dps368.pressure_x100 / 100.0;
        double temp     = (double)snap.dps368.temperature_x100 / 100.0;
        double rh       = 50.0;  /* default if SHT40 unavailable */

#if BSP_HAS_SHT40
        if (snap.has_sht40) {
            rh = (double)snap.sht40.humidity_x100 / 100.0;
        }
#endif

        /* Altitude from barometric pressure (hypsometric formula) */
        double altitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.190295));

        /* Dew point via Magnus-Tetens approximation */
        if (rh < 1.0) rh = 1.0;
        const double a = 17.27;
        const double b = 237.7;
        double gamma = (a * temp) / (b + temp) + log(rh / 100.0);
        double dew_point = (b * gamma) / (a - gamma);

        /* Heat index (Rothfusz regression, valid above 80F / 40% RH) */
        double heat_index_c = temp;
        double tf = temp * 9.0 / 5.0 + 32.0;
        if (tf >= 80.0 && rh >= 40.0) {
            double hi_f = -42.379
                        + 2.04901523  * tf
                        + 10.14333127 * rh
                        - 0.22475541  * tf * rh
                        - 0.00683783  * tf * tf
                        - 0.05481717  * rh * rh
                        + 0.00122874  * tf * tf * rh
                        + 0.00085282  * tf * rh * rh
                        - 0.00000199  * tf * tf * rh * rh;
            heat_index_c = (hi_f - 32.0) * 5.0 / 9.0;
        }

        /* Comfort level classification */
        const char *comfort;
        if (temp < 18.0)                                         comfort = "Cold";
        else if (temp > 27.0)                                    comfort = "Hot";
        else if (rh < 30.0)                                      comfort = "Dry";
        else if (rh > 70.0)                                      comfort = "Humid";
        else if (temp >= 20.0 && temp <= 25.0 &&
                 rh >= 30.0 && rh <= 60.0)                       comfort = "Comfort";
        else                                                     comfort = "OK";

        snprintf(s_baro_buf, sizeof(s_baro_buf),
                 "P:%.1f hPa T:%.1f C\n"
                 "Alt:%.0fm Dew:%.1f C\n"
                 "HI:%.1f C [%s]",
                 pressure, temp, altitude, dew_point, heat_index_c, comfort);
        lv_label_set_text(s_ctx.baro_label, s_baro_buf);
        any_update = true;
    }
#endif /* BSP_HAS_DPS368 */

    /*
     * SHT40 - Humidity + Temperature (AI Kit only)
     */
#if BSP_HAS_SHT40
    if (snap.has_sht40 && snap.sht40_changed && s_ctx.env_label) {
        double temp = (double)snap.sht40.temperature_x100 / 100.0;
        double hum  = (double)snap.sht40.humidity_x100 / 100.0;
        snprintf(s_env_buf, sizeof(s_env_buf),
                 "Temp:     %.1f C\n"
                 "Humidity: %.1f %%RH",
                 temp, hum);
        lv_label_set_text(s_ctx.env_label, s_env_buf);
        any_update = true;
    }
#endif /* BSP_HAS_SHT40 */

    /*
     * CapSense - Touch Buttons + Slider (Eva Kit only)
     */
#if BSP_HAS_CAPSENSE
    if (snap.has_capsense && snap.capsense_changed && s_ctx.controls_label) {
        snprintf(s_controls_buf, sizeof(s_controls_buf),
                 "Btn0:%s Btn1:%s\n"
                 "Slider: %u%%",
                 snap.capsense.btn0_pressed ? "ON" : "OFF",
                 snap.capsense.btn1_pressed ? "ON" : "OFF",
                 (unsigned)snap.capsense.slider);
        lv_label_set_text(s_ctx.controls_label, s_controls_buf);
        any_update = true;
    }
#endif /* BSP_HAS_CAPSENSE */

    /* Update count status label */
    if (any_update) {
        s_ctx.update_count++;
        snprintf(s_status_buf, sizeof(s_status_buf),
                 "Updates: %lu", (unsigned long)s_ctx.update_count);
        lv_label_set_text(s_ctx.status_label, s_status_buf);
    }
}

/*******************************************************************************
 * example_main - entry point called by the example framework
 ******************************************************************************/
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));

    /* ── Root container styling ─────────────────────────────────────── */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_pad_gap(parent, 6, 0);

    /* ── Title bar ──────────────────────────────────────────────────── */
    lv_obj_t *title = example_label_create(parent, "Sensor Dashboard",
                                           &lv_font_montserrat_20,
                                           UI_COLOR_PRIMARY);
    /* แดชบอร์ดเซ็นเซอร์ */
    example_label_create(parent,
        "แดชบอร์ดเซ็นเซอร์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    /* ── Sensor cards row ───────────────────────────────────────────── */
    lv_obj_t *card_row = lv_obj_create(parent);
    lv_obj_remove_style_all(card_row);
    lv_obj_set_size(card_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(card_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(card_row, 6, 0);
    lv_obj_clear_flag(card_row, LV_OBJ_FLAG_SCROLLABLE);

    /* Compute card width: distribute evenly across available width */
    int card_w = (DISPLAY_WIDTH - 16 - (_CARD_COUNT - 1) * 6) / _CARD_COUNT;

    /* BMI270 card - always present on all boards */
    make_sensor_card(card_row, "BMI270", card_w,
                     UI_COLOR_BMI270, &s_ctx.imu_label);

#if BSP_HAS_DPS368
    make_sensor_card(card_row, "DPS368", card_w,
                     UI_COLOR_DPS368, &s_ctx.baro_label);
#endif

#if BSP_HAS_SHT40
    make_sensor_card(card_row, "SHT40", card_w,
                     UI_COLOR_SHT40, &s_ctx.env_label);
#endif

#if BSP_HAS_CAPSENSE
    make_sensor_card(card_row, "CapSense", card_w,
                     lv_color_hex(0xE040FB), &s_ctx.controls_label);
#endif

    /* ── Motion trends container ────────────────────────────────────── */
    lv_obj_t *chart_card = example_card_create(parent, LV_PCT(100),
                                               LV_SIZE_CONTENT,
                                               UI_COLOR_CARD_BG);
    lv_obj_set_flex_flow(chart_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chart_card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(chart_card, 8, 0);
    lv_obj_set_style_pad_gap(chart_card, 4, 0);

    /* Accelerometer chart */
    example_label_create(chart_card, "Accel (cm/s2)  X / Y / Z",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    s_ctx.accel_chart = lv_chart_create(chart_card);
    style_chart(s_ctx.accel_chart);
    s_ctx.ax_ser = lv_chart_add_series(s_ctx.accel_chart,
                       lv_color_hex(COLOR_ACCEL_X), LV_CHART_AXIS_PRIMARY_Y);
    s_ctx.ay_ser = lv_chart_add_series(s_ctx.accel_chart,
                       lv_color_hex(COLOR_ACCEL_Y), LV_CHART_AXIS_PRIMARY_Y);
    s_ctx.az_ser = lv_chart_add_series(s_ctx.accel_chart,
                       lv_color_hex(COLOR_ACCEL_Z), LV_CHART_AXIS_PRIMARY_Y);

    /* Gyroscope chart */
    example_label_create(chart_card, "Gyro (deci-dps)  X / Y / Z",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    s_ctx.gyro_chart = lv_chart_create(chart_card);
    style_chart(s_ctx.gyro_chart);
    s_ctx.gx_ser = lv_chart_add_series(s_ctx.gyro_chart,
                       lv_color_hex(COLOR_GYRO_X), LV_CHART_AXIS_PRIMARY_Y);
    s_ctx.gy_ser = lv_chart_add_series(s_ctx.gyro_chart,
                       lv_color_hex(COLOR_GYRO_Y), LV_CHART_AXIS_PRIMARY_Y);
    s_ctx.gz_ser = lv_chart_add_series(s_ctx.gyro_chart,
                       lv_color_hex(COLOR_GYRO_Z), LV_CHART_AXIS_PRIMARY_Y);

    /* ── Status bar ─────────────────────────────────────────────────── */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Updates: 0");
    lv_obj_set_style_text_color(s_ctx.status_label,
                                lv_color_hex(0x50D890), 0);
    lv_obj_set_style_text_font(s_ctx.status_label,
                                &lv_font_montserrat_14, 0);

    /* ── Start 10 Hz update timer ───────────────────────────────────── */
    lv_timer_create(dashboard_timer_cb, UPDATE_PERIOD_MS, NULL);
}
