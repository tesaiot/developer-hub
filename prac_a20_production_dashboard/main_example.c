/**
 * @file    main_example.c
 * @brief   Production Dashboard - 4 sensor cards + chart + status
 *
 * @description
 *   Industrial dashboard with direct sensor reads from app_sensor,
 *   rolling chart, and system status indicators.
 *   Ported from Developer Hub a20_production_dashboard.
 *
 * @board    KIT_PSE84_AI, TESAIoT_PSE84_AI
 * @author   TESAIoT
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

/* ── Layout ─────────────────────────────────────────────────────── */
#define CARD_W       175
#define CARD_H        80
#define CARD_GAP       8
#define CHART_W      380
#define CHART_H      140
#define CHART_PTS     60
#define STATUS_W     380
#define STATUS_H      36

/* ── Card indices ───────────────────────────────────────────────── */
enum { CARD_IMU = 0, CARD_PRESS, CARD_HUM, CARD_MAG, CARD_COUNT };

/* ── State ──────────────────────────────────────────────────────── */
static lv_obj_t  *s_val_lbl[CARD_COUNT];
static lv_obj_t  *s_chart;
static lv_chart_series_t *s_series;
static lv_obj_t  *s_status_dots[4];
static lv_obj_t  *s_lbl_uptime;

static bool s_bmi270_ok = false;
static bool s_dps368_ok = false;
static bool s_sht4x_ok = false;
static bool s_bmm350_ok = false;

static const char *s_status_names[] = { "IMU", "ENV", "MAG", "SYS" };

/* ── Create sensor card ─────────────────────────────────────────── */
static lv_obj_t *create_sensor_card(lv_obj_t *parent, const char *title,
                                     const char *icon, lv_color_t accent,
                                     int col, int row, lv_obj_t **val_out)
{
    int x_off = -((CARD_W + CARD_GAP) / 2) + col * (CARD_W + CARD_GAP);
    int y_base = 32;

    lv_obj_t *card = example_card_create(parent, CARD_W, CARD_H, UI_COLOR_CARD_BG);
    lv_obj_align(card, LV_ALIGN_TOP_MID, x_off, y_base + row * (CARD_H + CARD_GAP));

    /* Accent bar (left edge) */
    lv_obj_t *bar = lv_obj_create(card);
    lv_obj_set_size(bar, 4, CARD_H - 24);
    lv_obj_set_style_bg_color(bar, accent, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_align(bar, LV_ALIGN_LEFT_MID, -4, 0);

    /* Icon + title */
    lv_obj_t *hdr = lv_label_create(card);
    char buf[48];
    snprintf(buf, sizeof(buf), "%s %s", icon, title);
    lv_label_set_text(hdr, buf);
    lv_obj_set_style_text_color(hdr, accent, 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 6, 0);

    /* Value */
    *val_out = lv_label_create(card);
    lv_label_set_text(*val_out, "--");
    lv_obj_set_style_text_color(*val_out, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(*val_out, &lv_font_montserrat_24, 0);
    lv_obj_align(*val_out, LV_ALIGN_BOTTOM_LEFT, 6, 0);

    return card;
}

/* ── Timer: update sensor values from real hardware ────────────── */
static void dashboard_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* IMU — BMI270 direct read */
    float accel_mag = 0.0f;
    bmi270_sample_t bmi_sample;
    if (bmi270_reader_poll(&bmi_sample)) {
        accel_mag = bmi_sample.acc_mag_g;
        lv_label_set_text_fmt(s_val_lbl[CARD_IMU], "%.2f g", (double)accel_mag);
        lv_obj_set_style_bg_color(s_status_dots[0], UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_val_lbl[CARD_IMU], "N/A");
        lv_obj_set_style_bg_color(s_status_dots[0], UI_COLOR_TEXT_DIM, 0);
    }

    /* Pressure — DPS368 direct read */
    dps368_sample_t dps_sample;
    if (dps368_reader_poll(&dps_sample)) {
        lv_label_set_text_fmt(s_val_lbl[CARD_PRESS], "%.0f hPa",
                              (double)dps_sample.pressure_hpa);
        lv_obj_set_style_bg_color(s_status_dots[1], UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_val_lbl[CARD_PRESS], "--");
        lv_obj_set_style_bg_color(s_status_dots[1], UI_COLOR_TEXT_DIM, 0);
    }

    /* Humidity — SHT4x direct read */
    sht4x_sample_t sht_sample;
    if (sht4x_reader_poll(&sht_sample)) {
        lv_label_set_text_fmt(s_val_lbl[CARD_HUM], "%.0f %%RH",
                              (double)sht_sample.humidity_rh);
    } else {
        lv_label_set_text(s_val_lbl[CARD_HUM], "--");
    }

    /* Magnetometer — BMM350 direct read */
    bmm350_sample_t bmm_sample;
    if (bmm350_reader_poll(&bmm_sample)) {
        int heading = (int)bmm_sample.heading_deg;
        lv_label_set_text_fmt(s_val_lbl[CARD_MAG], "%d\xc2\xb0", heading);
        lv_obj_set_style_bg_color(s_status_dots[2], UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_val_lbl[CARD_MAG], "--");
        lv_obj_set_style_bg_color(s_status_dots[2], UI_COLOR_TEXT_DIM, 0);
    }

    /* Chart: add real accel magnitude */
    lv_chart_set_next_value(s_chart, s_series, (int32_t)(accel_mag * 100));

    /* SYS status: always green (board running) */
    lv_obj_set_style_bg_color(s_status_dots[3], UI_COLOR_SUCCESS, 0);

    /* Uptime */
    uint32_t sec = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;
    lv_label_set_text_fmt(s_lbl_uptime, "Uptime: %lum %02lus",
                          (unsigned long)(sec / 60), (unsigned long)(sec % 60));
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    /* Initialize all 4 sensors */
    s_bmi270_ok = (bmi270_reader_init(&sensor_i2c_controller_hal_obj) == CY_RSLT_SUCCESS);
    s_dps368_ok = (dps368_reader_init(&sensor_i2c_controller_hal_obj) == CY_RSLT_SUCCESS);
    s_sht4x_ok  = (sht4x_reader_init(&sensor_i2c_controller_hal_obj) == CY_RSLT_SUCCESS);
    s_bmm350_ok = (bmm350_reader_init(CYBSP_I3C_CONTROLLER_HW,
                                       &CYBSP_I3C_CONTROLLER_context) == CY_RSLT_SUCCESS);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "A20 - Production Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t *th_sub = example_label_create(parent,
        "แดชบอร์ดการผลิต",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);

    /* === 4 Sensor Cards (2x2 grid) === */
    create_sensor_card(parent, "IMU", LV_SYMBOL_REFRESH, UI_COLOR_BMI270,
                       0, 0, &s_val_lbl[CARD_IMU]);
    create_sensor_card(parent, "Pressure", LV_SYMBOL_SETTINGS, UI_COLOR_DPS368,
                       1, 0, &s_val_lbl[CARD_PRESS]);
    create_sensor_card(parent, "Humidity", LV_SYMBOL_CHARGE, UI_COLOR_SHT40,
                       0, 1, &s_val_lbl[CARD_HUM]);
    create_sensor_card(parent, "Compass", LV_SYMBOL_GPS, UI_COLOR_BMM350,
                       1, 1, &s_val_lbl[CARD_MAG]);

    /* === Rolling chart === */
    lv_obj_t *chart_card = example_card_create(parent, CHART_W, CHART_H + 30,
                                                UI_COLOR_CARD_BG);
    lv_obj_align(chart_card, LV_ALIGN_CENTER, 0, 60);

    lv_obj_t *chart_title = lv_label_create(chart_card);
    lv_label_set_text(chart_title, "Accel Magnitude (g x100)");
    lv_obj_set_style_text_color(chart_title, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_14, 0);
    lv_obj_align(chart_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_chart = lv_chart_create(chart_card);
    lv_obj_set_size(s_chart, CHART_W - 24, CHART_H - 10);
    lv_obj_align(s_chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, CHART_PTS);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 80, 120);
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_width(s_chart, 0, 0);
    lv_obj_set_style_line_color(s_chart, lv_color_hex(0x1a3050), LV_PART_MAIN);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);

    s_series = lv_chart_add_series(s_chart, UI_COLOR_BMI270,
                                    LV_CHART_AXIS_PRIMARY_Y);

    /* Pre-fill chart */
    for (int i = 0; i < CHART_PTS; i++) {
        lv_chart_set_next_value(s_chart, s_series, 98);
    }

    /* === Status bar === */
    lv_obj_t *status_bar = example_card_create(parent, STATUS_W, STATUS_H,
                                                UI_COLOR_CARD_BG);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(status_bar, 12, 0);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *cont = lv_obj_create(status_bar);
        lv_obj_set_size(cont, 70, 20);
        lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_style_pad_all(cont, 0, 0);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

        s_status_dots[i] = lv_obj_create(cont);
        lv_obj_set_size(s_status_dots[i], 10, 10);
        lv_obj_set_style_bg_color(s_status_dots[i], UI_COLOR_TEXT_DIM, 0);
        lv_obj_set_style_bg_opa(s_status_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_status_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_status_dots[i], 0, 0);
        lv_obj_align(s_status_dots[i], LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *lbl = lv_label_create(cont);
        lv_label_set_text(lbl, s_status_names[i]);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DIM, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 14, 0);
    }

    /* Uptime label */
    s_lbl_uptime = lv_label_create(parent);
    lv_label_set_text(s_lbl_uptime, "Uptime: 0m 00s");
    lv_obj_set_style_text_color(s_lbl_uptime, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(s_lbl_uptime, &lv_font_montserrat_14, 0);
    lv_obj_align_to(s_lbl_uptime, status_bar, LV_ALIGN_OUT_TOP_RIGHT, 0, -4);

    /* Poll timer: 500ms */
    lv_timer_create(dashboard_timer_cb, 500, NULL);
}
