/**
 * i11_status_panel - Production System Status Panel
 *
 * Grid of status cards showing system health: CPU load (simulated),
 * FreeRTOS heap usage, uptime, sensor availability, and WiFi state.
 * Color-coded indicators: green=OK, yellow=warning, red=error.
 *
 * Ported from Developer Hub i11_status_panel.
 * Sensors read directly via app_sensor. WiFi status uses demo values
 * (WiFi runs on CM33_NS, not directly accessible from CM55).
 *
 * Board:  KIT_PSE84_AI, TESAIoT_PSE84_AI
 * Core:   CM55 (display + UI)
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"

/* app_sensor direct sensor readers */
#include "bmi270/bmi270_reader.h"
#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"
#include "bmm350/bmm350_reader.h"

/* ── Layout Constants ────────────────────────────────────────────── */
#define CARD_W           210
#define CARD_H           100
#define CARD_GAP         12
#define COL_LEFT_X       16
#define COL_RIGHT_X      (COL_LEFT_X + CARD_W + CARD_GAP)
#define ROW_START_Y      48
#define ROW_GAP          (CARD_H + CARD_GAP)
#define UPDATE_PERIOD_MS 500

/* ── Status Card State ───────────────────────────────────────────── */
typedef struct {
    lv_obj_t *card;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *value;
    lv_obj_t *indicator;
} status_card_t;

static status_card_t s_cpu_card;
static status_card_t s_mem_card;
static status_card_t s_uptime_card;
static status_card_t s_sensor_card;
static status_card_t s_wifi_card;
static status_card_t s_imu_card;

static lv_timer_t *s_timer = NULL;

/* ── Color for Status Level ──────────────────────────────────────── */
static lv_color_t status_color(int level)
{
    /* 0=ok(green), 1=warning(yellow), 2=error(red) */
    if (level == 0) return UI_COLOR_SUCCESS;
    if (level == 1) return UI_COLOR_WARNING;
    return UI_COLOR_ERROR;
}

/* ── Create One Status Card ──────────────────────────────────────── */
static void create_status_card(lv_obj_t *parent, status_card_t *sc,
                                const char *icon_sym, const char *title_text,
                                int x, int y)
{
    sc->card = example_card_create(parent, CARD_W, CARD_H, UI_COLOR_CARD_BG);
    lv_obj_set_pos(sc->card, x, y);

    /* Status indicator dot (top-right) */
    sc->indicator = lv_obj_create(sc->card);
    lv_obj_remove_style_all(sc->indicator);
    lv_obj_set_size(sc->indicator, 10, 10);
    lv_obj_set_style_radius(sc->indicator, 5, 0);
    lv_obj_set_style_bg_color(sc->indicator, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(sc->indicator, LV_OPA_COVER, 0);
    lv_obj_align(sc->indicator, LV_ALIGN_TOP_RIGHT, -2, 2);

    /* Icon */
    sc->icon = lv_label_create(sc->card);
    lv_label_set_text(sc->icon, icon_sym);
    lv_obj_set_style_text_font(sc->icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sc->icon, UI_COLOR_PRIMARY, 0);
    lv_obj_align(sc->icon, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Title */
    sc->title = lv_label_create(sc->card);
    lv_label_set_text(sc->title, title_text);
    lv_obj_set_style_text_font(sc->title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sc->title, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(sc->title, LV_ALIGN_TOP_LEFT, 28, 4);

    /* Value */
    sc->value = lv_label_create(sc->card);
    lv_label_set_text(sc->value, "---");
    lv_obj_set_style_text_font(sc->value, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sc->value, UI_COLOR_TEXT, 0);
    lv_obj_align(sc->value, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

/* ── Set Indicator Color ─────────────────────────────────────────── */
static void set_indicator(status_card_t *sc, int level)
{
    lv_obj_set_style_bg_color(sc->indicator, status_color(level), 0);
}

/* ── Update Timer Callback ───────────────────────────────────────── */
static void update_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Uptime from LVGL tick (ms) */
    uint32_t ms = lv_tick_get();
    uint32_t total_sec = ms / 1000;
    uint32_t hrs = total_sec / 3600;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t sec = total_sec % 60;

    /* CPU - simulated from uptime modulo */
    uint32_t cpu_pct = 15 + (total_sec % 40);  /* 15-55% simulated */
    lv_label_set_text_fmt(s_cpu_card.value, "%lu %%", (unsigned long)cpu_pct);
    set_indicator(&s_cpu_card, cpu_pct > 80 ? 2 : (cpu_pct > 60 ? 1 : 0));

    /* Memory - show core info (xPortGetFreeHeapSize not available on CM55) */
    lv_label_set_text(s_mem_card.value, "CM55 LVGL");
    set_indicator(&s_mem_card, 0);

    /* Uptime */
    lv_label_set_text_fmt(s_uptime_card.value, "%02lu:%02lu:%02lu",
                          (unsigned long)hrs, (unsigned long)min, (unsigned long)sec);
    set_indicator(&s_uptime_card, 0);

    /* IMU (BMI270) — direct read */
    bmi270_sample_t bmi_sample;
    if (bmi270_reader_poll(&bmi_sample)) {
        lv_label_set_text_fmt(s_imu_card.value, "%.1f %.1f %.1f",
                              (double)bmi_sample.acc_g_x,
                              (double)bmi_sample.acc_g_y,
                              (double)bmi_sample.acc_g_z);
        set_indicator(&s_imu_card, 0);
    } else {
        lv_label_set_text(s_imu_card.value, "No data");
        set_indicator(&s_imu_card, 1);
    }

    /* Sensor availability summary — direct poll each sensor */
    int sensor_count = 0;
    int sensor_total = 4;  /* BMI270 + DPS368 + SHT4x + BMM350 */

    bmi270_sample_t bmi_chk;
    if (bmi270_reader_poll(&bmi_chk)) sensor_count++;

    dps368_sample_t dps_chk;
    if (dps368_reader_poll(&dps_chk)) sensor_count++;

    sht4x_sample_t sht_chk;
    if (sht4x_reader_poll(&sht_chk)) sensor_count++;

    bmm350_sample_t bmm_chk;
    if (bmm350_reader_poll(&bmm_chk)) sensor_count++;

    lv_label_set_text_fmt(s_sensor_card.value, "%d / %d OK", sensor_count, sensor_total);
    set_indicator(&s_sensor_card, sensor_count < sensor_total ? 1 : 0);

    /* WiFi — runs on CM33_NS, not directly accessible from CM55.
     * TODO: Replace with actual WiFi status if IPC bridge is available. */
    lv_label_set_text(s_wifi_card.value, "Check CM33");
    set_indicator(&s_wifi_card, 1);
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
    lv_obj_t *title = example_label_create(parent, LV_SYMBOL_SETTINGS " System Status",
                                           &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub,
        "แผงสถานะระบบ");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 30);

    /* Row 0: CPU + Memory */
    create_status_card(parent, &s_cpu_card, LV_SYMBOL_CHARGE, "CPU",
                       COL_LEFT_X, ROW_START_Y);
    create_status_card(parent, &s_mem_card, LV_SYMBOL_DRIVE, "Heap",
                       COL_RIGHT_X, ROW_START_Y);

    /* Row 1: Uptime + Sensors */
    create_status_card(parent, &s_uptime_card, LV_SYMBOL_LOOP, "Uptime",
                       COL_LEFT_X, ROW_START_Y + ROW_GAP);
    create_status_card(parent, &s_sensor_card, LV_SYMBOL_EYE_OPEN, "Sensors",
                       COL_RIGHT_X, ROW_START_Y + ROW_GAP);

    /* Row 2: IMU + WiFi */
    create_status_card(parent, &s_imu_card, LV_SYMBOL_REFRESH, "BMI270",
                       COL_LEFT_X, ROW_START_Y + ROW_GAP * 2);
    create_status_card(parent, &s_wifi_card, LV_SYMBOL_WIFI, "WiFi",
                       COL_RIGHT_X, ROW_START_Y + ROW_GAP * 2);

    /* Start update timer */
    s_timer = lv_timer_create(update_timer_cb, UPDATE_PERIOD_MS, NULL);
    update_timer_cb(NULL);  /* Initial tick */
}
