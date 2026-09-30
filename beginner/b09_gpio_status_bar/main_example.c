/**
 * @file    main_example.c
 * @brief   GPIO Status Bar - Real-time board status indicators
 *
 * Three status indicators driven by real hardware signals:
 *   PWR - Always ON (board is powered)
 *   NET - WiFi connected state via ipc_sensorhub_wifi_connected()
 *   ACT - Sensor activity heartbeat via ipc_sensorhub_snapshot()
 *
 * @board  AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 */

#include "pse84_common.h"

#define NUM_INDICATORS 3

typedef struct {
    const char *name;
    lv_obj_t   *dot;
    lv_obj_t   *lbl;
    bool        active;
} indicator_t;

static indicator_t indicators[NUM_INDICATORS];

static void update_dot(indicator_t *ind)
{
    lv_obj_set_style_bg_color(ind->dot,
        ind->active ? lv_palette_main(LV_PALETTE_GREEN)
                    : lv_palette_main(LV_PALETTE_GREY), 0);
    lv_label_set_text_fmt(ind->lbl, "%s:%s",
                          ind->name, ind->active ? "ON" : "off");
}

/* Poll real hardware status every 500ms */
static void status_poll_cb(lv_timer_t *t)
{
    (void)t;

    /* PWR: always on (board is running) */
    indicators[0].active = true;

    /* NET: real WiFi connected state from CM33 via IPC */
    indicators[1].active = ipc_sensorhub_wifi_connected();

    /* ACT: sensor activity - true if any sensor has new data */
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);
    indicators[2].active = (snap.has_bmi270 && snap.bmi270_changed) ||
                           (snap.has_dps368 && snap.dps368_changed) ||
                           (snap.has_sht40  && snap.sht40_changed);

    for (int i = 0; i < NUM_INDICATORS; i++) {
        update_dot(&indicators[i]);
    }
}

static void create_status_item(lv_obj_t *bar, indicator_t *ind)
{
    lv_obj_t *item = lv_obj_create(bar);
    lv_obj_set_size(item, 110, 40);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(item, 4, 0);
    lv_obj_set_style_pad_column(item, 6, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(item, 0, 0);

    ind->dot = lv_obj_create(item);
    lv_obj_set_size(ind->dot, 16, 16);
    lv_obj_set_style_radius(ind->dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ind->dot, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(ind->dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ind->dot, 0, 0);

    char buf[16];
    snprintf(buf, sizeof(buf), "%s:off", ind->name);
    ind->lbl = lv_label_create(item);
    lv_label_set_text(ind->lbl, buf);
    lv_obj_set_style_text_font(ind->lbl, &lv_font_montserrat_14, 0);
}

void example_main(lv_obj_t *parent)
{
    const char *names[] = { "PWR", "NET", "ACT" };
    for (int i = 0; i < NUM_INDICATORS; i++) {
        indicators[i].name = names[i];
        indicators[i].active = false;
    }

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Status Bar Monitor",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* แถบสถานะ GPIO */
    thai_label(parent, "แถบสถานะ (สัญญาณจริง)", 14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Info */
    lv_obj_t *info = example_label_create(parent,
        "PWR=board power | NET=WiFi | ACT=sensor activity",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 30);

    /* Status bar */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 600, 55);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(bar, 8, 0);

    for (int i = 0; i < NUM_INDICATORS; i++) {
        create_status_item(bar, &indicators[i]);
    }

    /* Start polling timer (500ms) */
    lv_timer_create(status_poll_cb, 500, NULL);

    /* Initial update */
    status_poll_cb(NULL);
}
