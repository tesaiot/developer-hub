#include "bmi270_view.h"

#include "lvgl.h"

#include "bmi270/bmi270_config.h"
#include "app_logo.h"

typedef struct
{
    lv_obj_t *status_label;
    lv_obj_t *alert_label;

    lv_obj_t *acc_xyz_label;
    lv_obj_t *acc_mag_label;
    lv_obj_t *acc_bar;

    lv_obj_t *gyr_xyz_label;
    lv_obj_t *gyr_mag_label;
    lv_obj_t *gyr_bar;

    lv_obj_t *trend_chart;
    lv_chart_series_t *acc_series;
    lv_chart_series_t *gyr_series;
} bmi270_view_ctx_t;

static bmi270_view_ctx_t s_view;

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

void bmi270_view_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, 110);
    lv_obj_align(logo, LV_ALIGN_TOP_RIGHT, -12, 8);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "BMI270 Motion Monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 10);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Low-flicker demo: bars + trend chart + motion alert");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    s_view.alert_label = lv_label_create(screen);
    lv_label_set_text(s_view.alert_label, "MOTION ALERT");
    lv_obj_set_style_text_color(s_view.alert_label, lv_color_hex(0xFB7185), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.alert_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_opa(s_view.alert_label, LV_OPA_0, LV_PART_MAIN);
    lv_obj_align(s_view.alert_label, LV_ALIGN_TOP_MID, 0, 48);

    /* Left card: accelerometer values + magnitude bar. */
    lv_obj_t *acc_card = lv_obj_create(screen);
    lv_obj_set_size(acc_card, 372, 150);
    lv_obj_align(acc_card, LV_ALIGN_TOP_LEFT, 16, 86);
    lv_obj_set_style_bg_color(acc_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(acc_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(acc_card, 10, LV_PART_MAIN);

    lv_obj_t *acc_title = lv_label_create(acc_card);
    lv_label_set_text(acc_title, "Accelerometer (g)");
    lv_obj_set_style_text_color(acc_title, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(acc_title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(acc_title, LV_ALIGN_TOP_LEFT, 10, 6);

    s_view.acc_xyz_label = lv_label_create(acc_card);
    lv_label_set_text(s_view.acc_xyz_label, "X:+0.00  Y:+0.00  Z:+0.00");
    lv_obj_set_style_text_color(s_view.acc_xyz_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.acc_xyz_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_view.acc_xyz_label, LV_ALIGN_TOP_LEFT, 10, 40);

    s_view.acc_mag_label = lv_label_create(acc_card);
    lv_label_set_text(s_view.acc_mag_label, "|Acc| = 0.00 g");
    lv_obj_set_style_text_color(s_view.acc_mag_label, lv_color_hex(0x86EFAC), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.acc_mag_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_view.acc_mag_label, LV_ALIGN_TOP_LEFT, 10, 72);

    s_view.acc_bar = lv_bar_create(acc_card);
    lv_obj_set_size(s_view.acc_bar, 342, 20);
    lv_obj_align(s_view.acc_bar, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_bar_set_range(s_view.acc_bar, 0, 350);
    lv_bar_set_value(s_view.acc_bar, 0, LV_ANIM_OFF);

    /* Right card: gyroscope values + magnitude bar. */
    lv_obj_t *gyr_card = lv_obj_create(screen);
    lv_obj_set_size(gyr_card, 372, 150);
    lv_obj_align(gyr_card, LV_ALIGN_TOP_RIGHT, -16, 86);
    lv_obj_set_style_bg_color(gyr_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(gyr_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(gyr_card, 10, LV_PART_MAIN);

    lv_obj_t *gyr_title = lv_label_create(gyr_card);
    lv_label_set_text(gyr_title, "Gyroscope (dps)");
    lv_obj_set_style_text_color(gyr_title, lv_color_hex(0xFCD34D), LV_PART_MAIN);
    lv_obj_set_style_text_font(gyr_title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(gyr_title, LV_ALIGN_TOP_LEFT, 10, 6);

    s_view.gyr_xyz_label = lv_label_create(gyr_card);
    lv_label_set_text(s_view.gyr_xyz_label, "X:+0.0  Y:+0.0  Z:+0.0");
    lv_obj_set_style_text_color(s_view.gyr_xyz_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.gyr_xyz_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_view.gyr_xyz_label, LV_ALIGN_TOP_LEFT, 10, 40);

    s_view.gyr_mag_label = lv_label_create(gyr_card);
    lv_label_set_text(s_view.gyr_mag_label, "|Gyro| = 0.0 dps");
    lv_obj_set_style_text_color(s_view.gyr_mag_label, lv_color_hex(0xFDBA74), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.gyr_mag_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_view.gyr_mag_label, LV_ALIGN_TOP_LEFT, 10, 72);

    s_view.gyr_bar = lv_bar_create(gyr_card);
    lv_obj_set_size(s_view.gyr_bar, 342, 20);
    lv_obj_align(s_view.gyr_bar, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_bar_set_range(s_view.gyr_bar, 0, 350);
    lv_bar_set_value(s_view.gyr_bar, 0, LV_ANIM_OFF);

    /* Bottom card: rolling trend chart. */
    lv_obj_t *trend_card = lv_obj_create(screen);
    lv_obj_set_size(trend_card, 768, 180);
    lv_obj_align(trend_card, LV_ALIGN_TOP_LEFT, 16, 246);
    lv_obj_set_style_bg_color(trend_card, lv_color_hex(0x101B2D), LV_PART_MAIN);
    lv_obj_set_style_border_color(trend_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(trend_card, 10, LV_PART_MAIN);

    lv_obj_t *trend_title = lv_label_create(trend_card);
    lv_label_set_text(trend_title, "Trend (Acc x100, Gyro / 10)");
    lv_obj_set_style_text_color(trend_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(trend_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(trend_title, LV_ALIGN_TOP_LEFT, 10, 8);

    s_view.trend_chart = lv_chart_create(trend_card);
    lv_obj_set_size(s_view.trend_chart, 744, 132);
    lv_obj_align(s_view.trend_chart, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_chart_set_type(s_view.trend_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_view.trend_chart, 60);
    lv_chart_set_range(s_view.trend_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 380);
    lv_chart_set_update_mode(s_view.trend_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(s_view.trend_chart, 5, 8);

    s_view.acc_series = lv_chart_add_series(s_view.trend_chart, lv_color_hex(0x22C55E), LV_CHART_AXIS_PRIMARY_Y);
    s_view.gyr_series = lv_chart_add_series(s_view.trend_chart, lv_color_hex(0xF97316), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(s_view.trend_chart, s_view.acc_series, 0);
    lv_chart_set_all_value(s_view.trend_chart, s_view.gyr_series, 0);

    s_view.status_label = lv_label_create(screen);
    lv_label_set_text(s_view.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_view.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(s_view.status_label, LV_ALIGN_BOTTOM_LEFT, 16, -8);
}

void bmi270_view_set_init_failed(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: init failed");
}

void bmi270_view_set_ready(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: ready");
}

void bmi270_view_set_read_error(cy_rslt_t rslt)
{
    char line[64];

    lv_snprintf(line, sizeof(line), "Sensor: read error (0x%08lx)", (unsigned long)rslt);
    lv_label_set_text(s_view.status_label, line);
}

void bmi270_view_update_sample(const bmi270_sample_t *sample)
{
    char line[96];
    int32_t acc_scaled;
    int32_t gyr_scaled;

    if (NULL == sample)
    {
        return;
    }

    lv_snprintf(line, sizeof(line), "X:%+0.2f  Y:%+0.2f  Z:%+0.2f",
                (double)sample->acc_g_x,
                (double)sample->acc_g_y,
                (double)sample->acc_g_z);
    lv_label_set_text(s_view.acc_xyz_label, line);

    lv_snprintf(line, sizeof(line), "|Acc| = %.2f g", (double)sample->acc_mag_g);
    lv_label_set_text(s_view.acc_mag_label, line);

    lv_snprintf(line, sizeof(line), "X:%+0.1f  Y:%+0.1f  Z:%+0.1f",
                (double)sample->gyr_dps_x,
                (double)sample->gyr_dps_y,
                (double)sample->gyr_dps_z);
    lv_label_set_text(s_view.gyr_xyz_label, line);

    lv_snprintf(line, sizeof(line), "|Gyro| = %.1f dps", (double)sample->gyr_mag_dps);
    lv_label_set_text(s_view.gyr_mag_label, line);

    acc_scaled = clamp_i32((int32_t)(sample->acc_mag_g * 100.0f), 0, 350);
    gyr_scaled = clamp_i32((int32_t)(sample->gyr_mag_dps / 10.0f), 0, 350);

#if BMI270_BAR_USE_ANIM
    lv_bar_set_value(s_view.acc_bar, acc_scaled, LV_ANIM_ON);
    lv_bar_set_value(s_view.gyr_bar, gyr_scaled, LV_ANIM_ON);
#else
    lv_bar_set_value(s_view.acc_bar, acc_scaled, LV_ANIM_OFF);
    lv_bar_set_value(s_view.gyr_bar, gyr_scaled, LV_ANIM_OFF);
#endif

    /* Push next samples; LVGL handles invalidation without forced refresh call. */
    lv_chart_set_next_value(s_view.trend_chart, s_view.acc_series, acc_scaled);
    lv_chart_set_next_value(s_view.trend_chart, s_view.gyr_series, gyr_scaled);
}

void bmi270_view_set_alert(bool enabled)
{
#if BMI270_ALERT_FADE_ENABLE
    if (enabled)
    {
        lv_obj_fade_in(s_view.alert_label, 150, 0);
    }
    else
    {
        lv_obj_fade_out(s_view.alert_label, 220, 0);
    }
#else
    lv_obj_set_style_opa(s_view.alert_label, enabled ? LV_OPA_100 : LV_OPA_0, LV_PART_MAIN);
#endif
}
