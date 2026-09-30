/**
 * @file    main_example.c
 * @brief   Radar Presence Detection - BGT60TR13C sweep + energy display
 *
 * @description
 *   Animated radar sweep, presence indicator, and signal energy bar using the
 *   BGT60TR13C 60 GHz radar sensor. Reads volatile globals from tesaiot_radar_task.
 *
 * @board    AI Kit (KIT_PSE84_AI) only - BGT60TR13C
 * @author   TESAIoT
 */

#include "pse84_common.h"

#if __has_include("radar_task.h")
#include "radar_task.h"
#else
#error "This example requires BGT60TR13C radar (AI Kit only)"
#endif

/* ── Layout constants ───────────────────────────────────────────── */
#define RADAR_ARC_SIZE   240
#define SWEEP_ARC_SPAN    30     /* Degrees for the sweep arc        */
#define SWEEP_STEP         3     /* Degrees per timer tick            */
#define ENERGY_BAR_W     300
#define ENERGY_BAR_H      20
#define ENERGY_BAR_MAX   100    /* Bar range 0-100%                  */
#define ENERGY_FLOOR   100000.0f    /* 100K = 0% (quiet baseline)    */
#define ENERGY_CEIL  10000000.0f    /* 10M = 100% (close presence)   */

/* ── State ──────────────────────────────────────────────────────── */
static lv_obj_t *s_sweep_arc;
static lv_obj_t *s_presence_dot;
static lv_obj_t *s_lbl_state;
static lv_obj_t *s_lbl_energy;
static lv_obj_t *s_energy_bar;
static int32_t   s_sweep_angle = 0;

/* ── Sweep animation timer ──────────────────────────────────────── */
static void sweep_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Rotate sweep arc */
    s_sweep_angle = (s_sweep_angle + SWEEP_STEP) % 360;
    lv_arc_set_angles(s_sweep_arc,
                      s_sweep_angle,
                      s_sweep_angle + SWEEP_ARC_SPAN);

    /* Read radar state from volatile globals (set by tesaiot_radar_task) */
    if (!tesaiot_radar_initialized) {
        lv_label_set_text(s_lbl_state, "RADAR NOT READY...");
        lv_obj_set_style_text_color(s_lbl_state, lv_color_hex(0xFF9800), 0);
        return;
    }

    bool presence = tesaiot_radar_presence_detected;
    float energy  = tesaiot_radar_current_energy;

    /* Update presence indicator */
    if (presence) {
        lv_obj_set_style_bg_color(s_presence_dot, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_shadow_color(s_presence_dot, UI_COLOR_SUCCESS, 0);
        lv_label_set_text(s_lbl_state, "PRESENCE DETECTED");
        lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_SUCCESS, 0);
    } else {
        lv_obj_set_style_bg_color(s_presence_dot, lv_color_hex(0x333333), 0);
        lv_obj_set_style_shadow_color(s_presence_dot, lv_color_hex(0x333333), 0);
        lv_label_set_text(s_lbl_state, "NO PRESENCE");
        lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_TEXT_DIM, 0);
    }

    /* Update energy bar: linear 0-100% between FLOOR and CEIL */
    int32_t pct = 0;
    if (energy > ENERGY_FLOOR) {
        float range = ENERGY_CEIL - ENERGY_FLOOR;
        pct = (int32_t)(((energy - ENERGY_FLOOR) / range) * 100.0f);
        if (pct < 0)   pct = 0;
        if (pct > 100)  pct = 100;
    }
    lv_bar_set_value(s_energy_bar, pct, LV_ANIM_ON);
    lv_label_set_text_fmt(s_lbl_energy, "%d%%  (raw: %.0f)", (int)pct, (double)energy);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "A18 - Radar Presence");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_RADAR, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *th_sub = thai_label(parent, "เรดาร์ตรวจจับคน", 14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);

    /* === Radar display area === */
    lv_obj_t *radar_bg = example_card_create(parent, RADAR_ARC_SIZE + 20,
                                              RADAR_ARC_SIZE + 20,
                                              lv_color_hex(0x0A1628));
    lv_obj_align(radar_bg, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_radius(radar_bg, LV_RADIUS_CIRCLE, 0);

    /* Concentric range rings */
    for (int i = 1; i <= 3; i++) {
        lv_obj_t *ring = lv_arc_create(radar_bg);
        int sz = (RADAR_ARC_SIZE * i) / 3;
        lv_obj_set_size(ring, sz, sz);
        lv_arc_set_bg_angles(ring, 0, 360);
        lv_arc_set_value(ring, 0);
        lv_obj_set_style_arc_color(ring, lv_color_hex(0x1a3050), LV_PART_MAIN);
        lv_obj_set_style_arc_width(ring, 1, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(ring, LV_OPA_TRANSP, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(ring);
    }

    /* Sweep arc */
    s_sweep_arc = lv_arc_create(radar_bg);
    lv_obj_set_size(s_sweep_arc, RADAR_ARC_SIZE, RADAR_ARC_SIZE);
    lv_arc_set_bg_angles(s_sweep_arc, 0, 360);
    lv_arc_set_angles(s_sweep_arc, 0, SWEEP_ARC_SPAN);
    lv_obj_set_style_arc_color(s_sweep_arc, UI_COLOR_RADAR, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_sweep_arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_sweep_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_sweep_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_remove_flag(s_sweep_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(s_sweep_arc);

    /* Center presence dot */
    s_presence_dot = lv_obj_create(radar_bg);
    lv_obj_set_size(s_presence_dot, 40, 40);
    lv_obj_set_style_bg_color(s_presence_dot, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(s_presence_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_presence_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_presence_dot, 0, 0);
    lv_obj_set_style_shadow_width(s_presence_dot, 16, 0);
    lv_obj_set_style_shadow_color(s_presence_dot, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(s_presence_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(s_presence_dot);

    /* State label */
    s_lbl_state = lv_label_create(parent);
    lv_label_set_text(s_lbl_state, "INITIALIZING...");
    lv_obj_set_style_text_font(s_lbl_state, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_state, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(s_lbl_state, radar_bg, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    /* Energy bar */
    lv_obj_t *energy_card = example_card_create(parent, ENERGY_BAR_W + 40,
                                                60, UI_COLOR_CARD_BG);
    lv_obj_align(energy_card, LV_ALIGN_BOTTOM_MID, 0, -10);

    s_lbl_energy = lv_label_create(energy_card);
    lv_label_set_text(s_lbl_energy, "Energy: --");
    lv_obj_set_style_text_font(s_lbl_energy, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_energy, UI_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_energy, LV_ALIGN_TOP_LEFT, 0, 0);

    s_energy_bar = lv_bar_create(energy_card);
    lv_obj_set_size(s_energy_bar, ENERGY_BAR_W, ENERGY_BAR_H);
    lv_bar_set_range(s_energy_bar, 0, ENERGY_BAR_MAX);
    lv_bar_set_value(s_energy_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_energy_bar, lv_color_hex(0x1a3050), 0);
    lv_obj_set_style_bg_color(s_energy_bar, UI_COLOR_RADAR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_energy_bar, 4, 0);
    lv_obj_set_style_radius(s_energy_bar, 4, LV_PART_INDICATOR);
    lv_obj_align(s_energy_bar, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* Radar task is started automatically by firmware — no init needed here.
     * We just read the volatile globals set by tesaiot_radar_task(). */

    /* Timer: 50ms sweep animation + sensor poll */
    lv_timer_create(sweep_timer_cb, 50, NULL);
}
