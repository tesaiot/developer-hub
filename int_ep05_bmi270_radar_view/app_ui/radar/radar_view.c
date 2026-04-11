#include "radar_view.h"

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef struct
{
    lv_obj_t *status_label;

    lv_obj_t *radar_scale;
    lv_obj_t *radar_needle;

    lv_obj_t *angle_label;
    lv_obj_t *level_label;

    lv_obj_t *intensity_acc_arc;
    lv_obj_t *intensity_gyr_arc;
    lv_obj_t *intensity_value_label;
    lv_obj_t *intensity_state_label;
    lv_obj_t *delta_acc_label;
    lv_obj_t *delta_gyr_label;
} radar_view_ctx_t;

static radar_view_ctx_t s_view;

static int32_t clamp_i32(int32_t val, int32_t min_val, int32_t max_val)
{
    if (val < min_val)
    {
        return min_val;
    }

    if (val > max_val)
    {
        return max_val;
    }

    return val;
}

static const char *radar_level_text(radar_motion_level_t level)
{
    switch (level)
    {
        case RADAR_LEVEL_LOW:
            return "Level: LOW";
        case RADAR_LEVEL_MEDIUM:
            return "Level: MEDIUM";
        case RADAR_LEVEL_HIGH:
            return "Level: HIGH";
        default:
            return "Level: UNKNOWN";
    }
}

static lv_color_t radar_level_color(radar_motion_level_t level)
{
    switch (level)
    {
        case RADAR_LEVEL_LOW:
            return lv_color_hex(0x22C55E);
        case RADAR_LEVEL_MEDIUM:
            return lv_color_hex(0xF59E0B);
        case RADAR_LEVEL_HIGH:
            return lv_color_hex(0xEF4444);
        default:
            return lv_color_hex(0xE2E8F0);
    }
}

void radar_view_create(void)
{
    static const char *radar_text[] =
    {
        "0", "45", "90", "135", "180", "225", "270", "315", "360", NULL
    };

    /* Root screen: vertical flex with header, content, and status line. */
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(screen, 8, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(header, 2, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "BMI270 Motion Radar");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(header);
    lv_label_set_text(subtitle, "Relative motion from accel delta + gyro");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *reference_label = lv_label_create(header);
    lv_label_set_text(reference_label, "Reference: board X/Y axis (not magnetic north)");
    lv_obj_set_style_text_color(reference_label, lv_color_hex(0xA5B4FC), LV_PART_MAIN);
    lv_obj_set_style_text_font(reference_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(content, 8, LV_PART_MAIN);

    lv_obj_t *radar_card = lv_obj_create(content);
    lv_obj_set_width(radar_card, 350);
    lv_obj_set_height(radar_card, LV_PCT(100));
    lv_obj_set_layout(radar_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(radar_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(radar_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(radar_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(radar_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(radar_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(radar_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(radar_card, 6, LV_PART_MAIN);

    lv_obj_t *radar_title = lv_label_create(radar_card);
    lv_label_set_text(radar_title, "Motion Scope");
    lv_obj_set_style_text_color(radar_title, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(radar_title, &lv_font_montserrat_20, LV_PART_MAIN);

    s_view.radar_scale = lv_scale_create(radar_card);
    lv_obj_set_size(s_view.radar_scale, 258, 258);
    lv_scale_set_mode(s_view.radar_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(s_view.radar_scale, 0, 360);
    lv_scale_set_total_tick_count(s_view.radar_scale, 41);
    lv_scale_set_major_tick_every(s_view.radar_scale, 5);
    lv_scale_set_text_src(s_view.radar_scale, radar_text);
    lv_scale_set_label_show(s_view.radar_scale, true);
    lv_scale_set_angle_range(s_view.radar_scale, 360);
    lv_scale_set_rotation(s_view.radar_scale, 0);
    lv_obj_set_style_bg_opa(s_view.radar_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_view.radar_scale, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_view.radar_scale, lv_color_hex(0x2C4364), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_view.radar_scale, lv_color_hex(0xD0E7FF), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(s_view.radar_scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_length(s_view.radar_scale, 10, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(s_view.radar_scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(s_view.radar_scale, lv_color_hex(0x8DD4FF), LV_PART_INDICATOR);
    lv_obj_set_style_length(s_view.radar_scale, 5, LV_PART_ITEMS);
    lv_obj_set_style_line_width(s_view.radar_scale, 1, LV_PART_ITEMS);
    lv_obj_set_style_line_color(s_view.radar_scale, lv_color_hex(0x4A658A), LV_PART_ITEMS);

    s_view.radar_needle = lv_line_create(s_view.radar_scale);
    lv_obj_set_style_line_width(s_view.radar_needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(s_view.radar_needle, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_view.radar_needle, lv_color_hex(0x22D3EE), LV_PART_MAIN);
    lv_scale_set_line_needle_value(s_view.radar_scale, s_view.radar_needle, 18, 0);
    /* Hide needle in STILL state; presenter shows it only when motion is active. */
    lv_obj_add_flag(s_view.radar_needle, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *center_dot = lv_obj_create(s_view.radar_scale);
    lv_obj_set_size(center_dot, 10, 10);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(center_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(center_dot, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_center(center_dot);

    s_view.angle_label = lv_label_create(radar_card);
    lv_label_set_text(s_view.angle_label, "Motion Angle: -- (still)");
    lv_obj_set_style_text_color(s_view.angle_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.angle_label, &lv_font_montserrat_20, LV_PART_MAIN);

    s_view.level_label = lv_label_create(radar_card);
    lv_label_set_text(s_view.level_label, "Level: STILL");
    lv_obj_set_style_text_color(s_view.level_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.level_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *intensity_card = lv_obj_create(content);
    lv_obj_set_flex_grow(intensity_card, 1);
    lv_obj_set_height(intensity_card, LV_PCT(100));
    lv_obj_set_layout(intensity_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(intensity_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(intensity_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(intensity_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(intensity_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(intensity_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(intensity_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(intensity_card, 8, LV_PART_MAIN);

    lv_obj_t *intensity_title = lv_label_create(intensity_card);
    lv_label_set_text(intensity_title, "Motion Intensity Rings");
    lv_obj_set_style_text_color(intensity_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(intensity_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *ring_wrap = lv_obj_create(intensity_card);
    lv_obj_set_size(ring_wrap, 220, 220);
    lv_obj_set_style_bg_opa(ring_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ring_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ring_wrap, 0, LV_PART_MAIN);

    s_view.intensity_acc_arc = lv_arc_create(ring_wrap);
    lv_obj_set_size(s_view.intensity_acc_arc, 220, 220);
    lv_obj_center(s_view.intensity_acc_arc);
    lv_arc_set_range(s_view.intensity_acc_arc, 0, 100);
    lv_arc_set_value(s_view.intensity_acc_arc, 0);
    lv_arc_set_rotation(s_view.intensity_acc_arc, 270);
    lv_arc_set_bg_angles(s_view.intensity_acc_arc, 0, 360);
    lv_obj_remove_style(s_view.intensity_acc_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_view.intensity_acc_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_view.intensity_acc_arc, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_view.intensity_acc_arc, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_view.intensity_acc_arc, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_view.intensity_acc_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

    s_view.intensity_gyr_arc = lv_arc_create(ring_wrap);
    lv_obj_set_size(s_view.intensity_gyr_arc, 162, 162);
    lv_obj_center(s_view.intensity_gyr_arc);
    lv_arc_set_range(s_view.intensity_gyr_arc, 0, 100);
    lv_arc_set_value(s_view.intensity_gyr_arc, 0);
    lv_arc_set_rotation(s_view.intensity_gyr_arc, 270);
    lv_arc_set_bg_angles(s_view.intensity_gyr_arc, 0, 360);
    lv_obj_remove_style(s_view.intensity_gyr_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_view.intensity_gyr_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_view.intensity_gyr_arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_view.intensity_gyr_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_view.intensity_gyr_arc, lv_color_hex(0x1B2A3A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_view.intensity_gyr_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

    s_view.intensity_value_label = lv_label_create(ring_wrap);
    lv_label_set_text(s_view.intensity_value_label, "A:0%\nG:0%");
    lv_obj_set_style_text_color(s_view.intensity_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_view.intensity_value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.intensity_value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(s_view.intensity_value_label);

    s_view.intensity_state_label = lv_label_create(intensity_card);
    lv_label_set_text(s_view.intensity_state_label, "State: STILL");
    lv_obj_set_style_text_color(s_view.intensity_state_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.intensity_state_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_view.delta_acc_label = lv_label_create(intensity_card);
    lv_label_set_text(s_view.delta_acc_label, "Delta Acc XY: 0.000 g");
    lv_obj_set_style_text_color(s_view.delta_acc_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.delta_acc_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_view.delta_gyr_label = lv_label_create(intensity_card);
    lv_label_set_text(s_view.delta_gyr_label, "|Delta Gyro Z|: 0.0 dps");
    lv_obj_set_style_text_color(s_view.delta_gyr_label, lv_color_hex(0xFDBA74), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.delta_gyr_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_view.status_label = lv_label_create(screen);
    lv_label_set_text(s_view.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_view.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.status_label, &lv_font_montserrat_16, LV_PART_MAIN);
}

void radar_view_set_init_failed(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: init failed");
}

void radar_view_set_ready(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: ready");
}

void radar_view_set_read_error(cy_rslt_t rslt)
{
    char line[64];

    lv_snprintf(line, sizeof(line), "Sensor: read error (0x%08lx)", (unsigned long)rslt);
    lv_label_set_text(s_view.status_label, line);
}

void radar_view_update_motion(const bmi270_sample_t *sample,
                              float angle_deg,
                              float acc_xy_delta_g,
                              float gyr_z_delta_abs_dps,
                              radar_motion_level_t level,
                              bool motion_active)
{
    char line[96];
    int32_t angle_i;
    int32_t needle_len;
    int32_t acc_pct;
    int32_t gyr_pct;

    if (NULL == sample)
    {
        return;
    }

    /* Map physical delta values to 0..100 for dual-ring indicator. */
    acc_pct = clamp_i32((int32_t)((acc_xy_delta_g / 1.20f) * 100.0f + 0.5f), 0, 100);
    gyr_pct = clamp_i32((int32_t)((gyr_z_delta_abs_dps / 320.0f) * 100.0f + 0.5f), 0, 100);

    if (motion_active)
    {
        angle_i = clamp_i32((int32_t)(angle_deg + 0.5f), 0, 359);
        needle_len = clamp_i32((int32_t)(18.0f + (acc_xy_delta_g * 120.0f)), 18, 108);
        lv_obj_remove_flag(s_view.radar_needle, LV_OBJ_FLAG_HIDDEN);
        lv_scale_set_line_needle_value(s_view.radar_scale, s_view.radar_needle, needle_len, angle_i);

        lv_snprintf(line, sizeof(line), "Motion Angle: %03ld deg", (long)angle_i);
        lv_label_set_text(s_view.angle_label, line);

        lv_label_set_text(s_view.level_label, radar_level_text(level));
        lv_obj_set_style_text_color(s_view.level_label, radar_level_color(level), LV_PART_MAIN);

        lv_snprintf(line, sizeof(line), "State: %s",
                    (RADAR_LEVEL_HIGH == level) ? "HIGH" :
                    (RADAR_LEVEL_MEDIUM == level) ? "MEDIUM" : "LOW");
        lv_label_set_text(s_view.intensity_state_label, line);
        lv_obj_set_style_text_color(s_view.intensity_state_label, radar_level_color(level), LV_PART_MAIN);

        lv_obj_set_style_arc_color(s_view.intensity_acc_arc, radar_level_color(level), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(s_view.intensity_gyr_arc, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    }
    else
    {
        /* In STILL state, hide directional needle to avoid misleading angle cues. */
        lv_obj_add_flag(s_view.radar_needle, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_view.angle_label, "Motion Angle: -- (still)");
        lv_label_set_text(s_view.level_label, "Level: STILL");
        lv_obj_set_style_text_color(s_view.level_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);

        lv_label_set_text(s_view.intensity_state_label, "State: STILL");
        lv_obj_set_style_text_color(s_view.intensity_state_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);

        lv_obj_set_style_arc_color(s_view.intensity_acc_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(s_view.intensity_gyr_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

        acc_pct = 0;
        gyr_pct = 0;
    }

    lv_arc_set_value(s_view.intensity_acc_arc, acc_pct);
    lv_arc_set_value(s_view.intensity_gyr_arc, gyr_pct);

    lv_snprintf(line, sizeof(line), "A:%ld%%\nG:%ld%%", (long)acc_pct, (long)gyr_pct);
    lv_label_set_text(s_view.intensity_value_label, line);

    lv_snprintf(line, sizeof(line), "Delta Acc XY: %.3f g", (double)acc_xy_delta_g);
    lv_label_set_text(s_view.delta_acc_label, line);

    lv_snprintf(line, sizeof(line), "|Delta Gyro Z|: %.1f dps", (double)gyr_z_delta_abs_dps);
    lv_label_set_text(s_view.delta_gyr_label, line);
}