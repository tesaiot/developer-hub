/**
 * a19_smart_watch - Multi-screen Smart Watch with Swipe Navigation
 *
 * Demonstrates a circular watch face with 4 swipeable screens using
 * LVGL tileview: Clock, Sensors, Steps, and Weather. Dot indicators
 * at the bottom show the current active screen.
 *
 * Ported from Developer Hub a19_smart_watch.
 * Uses direct app_sensor reads instead of IPC sensorhub snapshot.
 *
 * Board:  KIT_PSE84_AI, TESAIoT_PSE84_AI
 * Core:   CM55 (display + UI)
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"
#include "watch_screens.h"

/* app_sensor direct sensor readers */
#include "bmi270/bmi270_reader.h"
#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"
#include "bmm350/bmm350_reader.h"

/* ── Layout Constants ────────────────────────────────────────────── */
#define WATCH_DIAMETER   280
#define WATCH_RADIUS     (WATCH_DIAMETER / 2)
#define NUM_SCREENS      4
#define DOT_SIZE         8
#define DOT_GAP          14
#define DOT_Y_OFFSET     24   /* from bottom of watch face */

/* ── Module State ────────────────────────────────────────────────── */
static lv_obj_t *s_tileview    = NULL;
static lv_obj_t *s_dots[NUM_SCREENS];
static lv_obj_t *s_tiles[NUM_SCREENS];
static lv_timer_t *s_clock_timer = NULL;

/* Labels updated by clock timer (owned by watch_clock.c) */
lv_obj_t *g_clock_time_label = NULL;
lv_obj_t *g_clock_date_label = NULL;

/* Labels updated by sensor timer */
static lv_timer_t *s_sensor_timer = NULL;

/* Forward declarations for sensor/step update labels (set by sub-files) */
lv_obj_t *g_sensor_ax_label = NULL;
lv_obj_t *g_sensor_ay_label = NULL;
lv_obj_t *g_sensor_az_label = NULL;
lv_obj_t *g_sensor_temp_label = NULL;
lv_obj_t *g_sensor_hum_label  = NULL;

lv_obj_t *g_steps_arc   = NULL;
lv_obj_t *g_steps_label = NULL;
lv_obj_t *g_steps_cal_label = NULL;
static uint32_t s_step_count = 0;

lv_obj_t *g_weather_temp_label = NULL;
lv_obj_t *g_weather_hum_label  = NULL;
lv_obj_t *g_weather_alt_label  = NULL;

/* ── Dot Indicator Update ────────────────────────────────────────── */
static void update_dots(uint32_t active_idx)
{
    for (int i = 0; i < NUM_SCREENS; i++) {
        lv_color_t col = (i == (int)active_idx) ? UI_COLOR_PRIMARY : lv_color_hex(0x404040);
        lv_obj_set_style_bg_color(s_dots[i], col, 0);
    }
}

/* ── Tileview Changed Callback ───────────────────────────────────── */
static void tileview_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *active = lv_tileview_get_tile_active(tv);
    for (int i = 0; i < NUM_SCREENS; i++) {
        if (s_tiles[i] == active) {
            update_dots((uint32_t)i);
            break;
        }
    }
}

/* ── Clock Timer (1 second) ──────────────────────────────────────── */
static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t ticks = xTaskGetTickCount();
    uint32_t total_sec = ticks / configTICK_RATE_HZ;
    uint32_t hrs = (total_sec / 3600) % 24;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t sec = total_sec % 60;

    if (g_clock_time_label) {
        lv_label_set_text_fmt(g_clock_time_label, "%02lu:%02lu:%02lu",
                              (unsigned long)hrs, (unsigned long)min, (unsigned long)sec);
    }
    if (g_clock_date_label) {
        static const char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        uint32_t day_idx = (total_sec / 86400) % 7;
        lv_label_set_text_fmt(g_clock_date_label, "%s - Day %lu",
                              days[day_idx], (unsigned long)(total_sec / 86400 + 1));
    }
}

/* ── Sensor Update Timer (200ms) ─────────────────────────────────── */
static void sensor_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* IMU accelerometer — direct BMI270 read */
    bmi270_sample_t bmi_sample;
    if (bmi270_reader_poll(&bmi_sample)) {
        float ax = bmi_sample.acc_g_x;
        float ay = bmi_sample.acc_g_y;
        float az = bmi_sample.acc_g_z;
        if (g_sensor_ax_label)
            lv_label_set_text_fmt(g_sensor_ax_label, "X: %.2f g", (double)ax);
        if (g_sensor_ay_label)
            lv_label_set_text_fmt(g_sensor_ay_label, "Y: %.2f g", (double)ay);
        if (g_sensor_az_label)
            lv_label_set_text_fmt(g_sensor_az_label, "Z: %.2f g", (double)az);

        /* Simulate steps from accel magnitude changes */
        float mag = bmi_sample.acc_mag_g;
        if (mag > 1.3f) {
            s_step_count += 1;
            if (s_step_count > 10000) s_step_count = 10000;
        }
    }

    /* Step counter arc + label */
    if (g_steps_arc)
        lv_arc_set_value(g_steps_arc, (int32_t)s_step_count);
    if (g_steps_label)
        lv_label_set_text_fmt(g_steps_label, "%lu", (unsigned long)s_step_count);
    if (g_steps_cal_label) {
        uint32_t cal = s_step_count * 4 / 100;  /* ~0.04 kcal per step */
        lv_label_set_text_fmt(g_steps_cal_label, "%lu kcal", (unsigned long)cal);
    }

    /* Temperature + altitude — direct DPS368 read */
    dps368_sample_t dps_sample;
    if (dps368_reader_poll(&dps_sample)) {
        if (g_sensor_temp_label)
            lv_label_set_text_fmt(g_sensor_temp_label, "Temp: %.1f C",
                                  (double)dps_sample.temperature_c);
        if (g_weather_temp_label)
            lv_label_set_text_fmt(g_weather_temp_label, "%.1f C",
                                  (double)dps_sample.temperature_c);

        /* Altitude from barometric pressure (ISA formula) */
        float press_hpa = dps_sample.pressure_hpa;
        if (press_hpa > 0 && g_weather_alt_label) {
            float alt = 44330.0f * (1.0f - powf(press_hpa / 1013.25f, 0.1903f));
            lv_label_set_text_fmt(g_weather_alt_label, "Alt: %.0f m", (double)alt);
        }
    }

    /* Humidity — direct SHT4x read */
    sht4x_sample_t sht_sample;
    if (sht4x_reader_poll(&sht_sample)) {
        if (g_sensor_hum_label)
            lv_label_set_text_fmt(g_sensor_hum_label, "Hum: %.1f %%",
                                  (double)sht_sample.humidity_rh);
        if (g_weather_hum_label)
            lv_label_set_text_fmt(g_weather_hum_label, "Humidity: %.1f %%",
                                  (double)sht_sample.humidity_rh);
    }
}

/* ── Create Dot Indicators ───────────────────────────────────────── */
static void create_dot_indicators(lv_obj_t *parent)
{
    int total_w = NUM_SCREENS * DOT_SIZE + (NUM_SCREENS - 1) * DOT_GAP;
    int start_x = (WATCH_DIAMETER - total_w) / 2;
    int y = WATCH_DIAMETER - DOT_Y_OFFSET;

    for (int i = 0; i < NUM_SCREENS; i++) {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, DOT_SIZE / 2, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x404040), 0);
        lv_obj_set_pos(dot, start_x + i * (DOT_SIZE + DOT_GAP), y);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        s_dots[i] = dot;
    }
    update_dots(0);
}

/* ── Entry Point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    /* Initialize sensors */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);
    dps368_reader_init(&sensor_i2c_controller_hal_obj);
    sht4x_reader_init(&sensor_i2c_controller_hal_obj);
    bmm350_reader_init(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Smart Watch",
                                           &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    example_label_create(parent,
        "นาฬิกาอัจฉริยะ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* Circular watch face container */
    lv_obj_t *watch = lv_obj_create(parent);
    lv_obj_set_size(watch, WATCH_DIAMETER, WATCH_DIAMETER);
    lv_obj_align(watch, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_radius(watch, WATCH_RADIUS, 0);
    lv_obj_set_style_bg_color(watch, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(watch, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(watch, 2, 0);
    lv_obj_set_style_border_color(watch, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_all(watch, 0, 0);
    lv_obj_clear_flag(watch, LV_OBJ_FLAG_SCROLLABLE);

    /* Tileview inside watch face for horizontal swiping */
    s_tileview = lv_tileview_create(watch);
    lv_obj_set_size(s_tileview, WATCH_DIAMETER - 4, WATCH_DIAMETER - 4);
    lv_obj_center(s_tileview);
    lv_obj_set_style_bg_opa(s_tileview, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(s_tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Create 4 tiles (horizontal swipe) */
    s_tiles[0] = lv_tileview_add_tile(s_tileview, 0, 0, LV_DIR_RIGHT);
    s_tiles[1] = lv_tileview_add_tile(s_tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    s_tiles[2] = lv_tileview_add_tile(s_tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    s_tiles[3] = lv_tileview_add_tile(s_tileview, 3, 0, LV_DIR_LEFT);

    /* Populate each tile */
    watch_clock_create(s_tiles[0]);
    watch_sensors_create(s_tiles[1]);
    watch_steps_create(s_tiles[2]);
    watch_weather_create(s_tiles[3]);

    /* Dot indicators (on the outer watch container, above tileview) */
    create_dot_indicators(watch);

    /* Screen labels below watch */
    static const char *screen_names[] = {"Clock", "Sensors", "Steps", "Weather"};
    lv_obj_t *hint = example_label_create(parent, "Swipe left/right to navigate",
                                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    (void)screen_names;

    /* Timers */
    s_clock_timer  = lv_timer_create(clock_timer_cb, 1000, NULL);
    s_sensor_timer = lv_timer_create(sensor_timer_cb, 200, NULL);

    /* Initial tick */
    clock_timer_cb(NULL);
    sensor_timer_cb(NULL);
}
