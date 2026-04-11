
#include "sensorhub_view.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#include "lvgl.h"

#include "app_logo.h"

typedef enum
{
    HUB_MOTION_STILL = 0,
    HUB_MOTION_LOW,
    HUB_MOTION_MEDIUM,
    HUB_MOTION_HIGH
} hub_motion_level_t;

typedef struct
{
    lv_obj_t *tabs[SENSORHUB_PAGE_COUNT];
    lv_obj_t *tab_labels[SENSORHUB_PAGE_COUNT];
    lv_obj_t *pages[SENSORHUB_PAGE_COUNT];
    lv_obj_t *footer_status;

    lv_obj_t *home_env_label;
    lv_obj_t *home_motion_label;
    lv_obj_t *home_compass_label;
    lv_obj_t *home_audio_label;

    lv_obj_t *env_pressure_label;
    lv_obj_t *env_pressure_bar;
    lv_obj_t *env_temp_label;
    lv_obj_t *env_temp_bar;
    lv_obj_t *env_hum_label;
    lv_obj_t *env_hum_bar;

    lv_obj_t *motion_scope_scale;
    lv_obj_t *motion_scope_needle;
    lv_obj_t *motion_angle_label;
    lv_obj_t *motion_level_label;
    lv_obj_t *motion_intensity_acc_arc;
    lv_obj_t *motion_intensity_gyr_arc;
    lv_obj_t *motion_intensity_value_label;
    lv_obj_t *motion_intensity_state_label;
    lv_obj_t *motion_delta_acc_label;
    lv_obj_t *motion_delta_gyr_label;

    bmi270_sample_t motion_prev_sample;
    bool motion_has_prev;

    lv_obj_t *compass_scale;
    lv_obj_t *compass_needle;
    lv_obj_t *compass_heading_label;
    lv_obj_t *compass_dir_label;
    lv_obj_t *compass_x_value_label;
    lv_obj_t *compass_y_value_label;
    lv_obj_t *compass_z_value_label;
    lv_obj_t *compass_x_bar;
    lv_obj_t *compass_y_bar;
    lv_obj_t *compass_z_bar;
    lv_obj_t *compass_field_label;
    lv_obj_t *compass_temp_label;

    lv_obj_t *compass_cal_overlay;
    lv_obj_t *compass_cal_title_label;
    lv_obj_t *compass_cal_progress_label;
    lv_obj_t *compass_cal_hint_label;
    lv_obj_t *compass_cal_sample_arc;
    lv_obj_t *compass_cal_coverage_arc;
    lv_obj_t *compass_cal_value_label;

    lv_obj_t *audio_left_label;
    lv_obj_t *audio_left_bar;
    lv_obj_t *audio_right_label;
    lv_obj_t *audio_right_bar;
    lv_obj_t *audio_balance_label;
    lv_obj_t *audio_balance_bar;

    sensorhub_view_tab_cb_t tab_cb;
    void *tab_user_data;

    sensorhub_view_compass_cal_cb_t compass_cal_cb;
    void *compass_cal_user_data;

    sensorhub_page_t active_page;
    bool created;
} sensorhub_view_ctx_t;

static sensorhub_view_ctx_t s_ctx;

#define HUB_BG_HEX                     0x081127
#define HUB_HEADER_BG_HEX              0x0C1730
#define HUB_HEADER_BORDER_HEX          0x1E335A
#define HUB_TAB_BG_HEX                 0x09152D
#define HUB_TAB_BORDER_HEX             0x1E335A
#define HUB_TAB_BTN_BG_HEX             0x12264A
#define HUB_TAB_BTN_BORDER_HEX         0x35588D
#define HUB_TAB_BTN_TEXT_HEX           0xEAF2FF
#define HUB_TAB_BTN_ACTIVE_BG_HEX      0x2563EB
#define HUB_TAB_BTN_ACTIVE_BORDER_HEX  0x93C5FD
#define HUB_TAB_BTN_ACTIVE_TEXT_HEX    0xFFFFFF
#define HUB_CONTENT_BG_HEX             0x0A142A
#define HUB_CONTENT_BORDER_HEX         0x1E335A
#define HUB_CARD_BG_HEX                0x122034
#define HUB_CARD_BORDER_HEX            0x2B4668
#define HUB_TEXT_TITLE_HEX             0xF8FAFC
#define HUB_TEXT_SUB_HEX               0x93C5FD
#define HUB_TEXT_MAIN_HEX              0xE2E8F0
#define HUB_TEXT_MUTED_HEX             0x94A3B8
#define HUB_FOOTER_BG_HEX              0x0C1730
#define HUB_FOOTER_BORDER_HEX          0x1E335A

#define HUB_PI_F                       3.1415926f
#define HUB_COMPASS_AXIS_MAX_UT        120

static float clampf(float value, float min_v, float max_v)
{
    if (value < min_v)
    {
        return min_v;
    }

    if (value > max_v)
    {
        return max_v;
    }

    return value;
}

static int32_t clamp_i32(int32_t value, int32_t min_v, int32_t max_v)
{
    if (value < min_v)
    {
        return min_v;
    }

    if (value > max_v)
    {
        return max_v;
    }

    return value;
}

static uint32_t clamp_pct_u32(uint32_t value)
{
    if (value > 100U)
    {
        return 100U;
    }

    return value;
}
static uint32_t clamp_pct_from_float(float value)
{
    if (value <= 0.0f)
    {
        return 0U;
    }

    if (value >= 100.0f)
    {
        return 100U;
    }

    return (uint32_t)(value + 0.5f);
}

static const char *heading_to_cardinal(float heading_deg)
{
    if ((heading_deg >= 337.5f) || (heading_deg < 22.5f)) return "N";
    if (heading_deg < 67.5f) return "NE";
    if (heading_deg < 112.5f) return "E";
    if (heading_deg < 157.5f) return "SE";
    if (heading_deg < 202.5f) return "S";
    if (heading_deg < 247.5f) return "SW";
    if (heading_deg < 292.5f) return "W";
    return "NW";
}

static hub_motion_level_t motion_level_from_delta(float acc_xy_delta_g, float gyr_z_delta_abs_dps)
{
    if ((acc_xy_delta_g >= 0.25f) || (gyr_z_delta_abs_dps >= 100.0f))
    {
        return HUB_MOTION_HIGH;
    }

    if ((acc_xy_delta_g >= 0.08f) || (gyr_z_delta_abs_dps >= 30.0f))
    {
        return HUB_MOTION_MEDIUM;
    }

    return HUB_MOTION_LOW;
}

static const char *motion_level_text(hub_motion_level_t level)
{
    switch (level)
    {
        case HUB_MOTION_LOW:
            return "Level: LOW";
        case HUB_MOTION_MEDIUM:
            return "Level: MEDIUM";
        case HUB_MOTION_HIGH:
            return "Level: HIGH";
        default:
            return "Level: STILL";
    }
}

static lv_color_t motion_level_color(hub_motion_level_t level)
{
    switch (level)
    {
        case HUB_MOTION_LOW:
            return lv_color_hex(0x22C55E);
        case HUB_MOTION_MEDIUM:
            return lv_color_hex(0xF59E0B);
        case HUB_MOTION_HIGH:
            return lv_color_hex(0xEF4444);
        default:
            return lv_color_hex(HUB_TEXT_MUTED_HEX);
    }
}

static void style_card(lv_obj_t *card)
{
    lv_obj_set_style_bg_color(card, lv_color_hex(HUB_CARD_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(HUB_CARD_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(card, 6, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *create_metric_card(lv_obj_t *parent,
                                    const char *title,
                                    lv_color_t bar_color,
                                    lv_obj_t **out_value_label,
                                    lv_obj_t **out_bar)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(33));
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_PCT(100));
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    style_card(card);

    lv_obj_t *title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN);

    *out_value_label = lv_label_create(card);
    lv_label_set_text(*out_value_label, "--");
    lv_obj_set_style_text_color(*out_value_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(*out_value_label, &lv_font_montserrat_20, LV_PART_MAIN);

    *out_bar = lv_bar_create(card);
    lv_obj_set_size(*out_bar, LV_PCT(100), 14);
    lv_bar_set_range(*out_bar, 0, 100);
    lv_bar_set_value(*out_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(*out_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(*out_bar, 8, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(*out_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, bar_color, LV_PART_INDICATOR);

    return card;
}

static lv_obj_t *create_compass_axis_row(lv_obj_t *parent,
                                         const char *axis_name,
                                         lv_color_t color,
                                         lv_obj_t **out_value_label,
                                         lv_obj_t **out_bar)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(row, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(row, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x132238), LV_PART_MAIN);
    lv_obj_set_style_border_color(row, lv_color_hex(0x355D8E), LV_PART_MAIN);
    lv_obj_set_style_radius(row, 8, LV_PART_MAIN);

    lv_obj_t *row_top = lv_obj_create(row);
    lv_obj_set_width(row_top, LV_PCT(100));
    lv_obj_set_height(row_top, LV_SIZE_CONTENT);
    lv_obj_set_layout(row_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_top,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row_top, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row_top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row_top, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(row_top);
    lv_label_set_text(title, axis_name);
    lv_obj_set_style_text_color(title, color, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);

    *out_value_label = lv_label_create(row_top);
    lv_label_set_text(*out_value_label, "0.0 uT");
    lv_obj_set_style_text_color(*out_value_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(*out_value_label, &lv_font_montserrat_16, LV_PART_MAIN);

    *out_bar = lv_bar_create(row);
    lv_obj_set_width(*out_bar, LV_PCT(100));
    lv_obj_set_height(*out_bar, 14);
    lv_bar_set_range(*out_bar, -HUB_COMPASS_AXIS_MAX_UT, HUB_COMPASS_AXIS_MAX_UT);
    lv_bar_set_mode(*out_bar, LV_BAR_MODE_RANGE);
    lv_bar_set_start_value(*out_bar, 0, LV_ANIM_OFF);
    lv_bar_set_value(*out_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(*out_bar, 7, LV_PART_MAIN);
    lv_obj_set_style_border_width(*out_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, color, LV_PART_INDICATOR);

    return row;
}

static void tab_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    sensorhub_page_t page = (sensorhub_page_t)(uintptr_t)lv_event_get_user_data(event);
    if (s_ctx.tab_cb != NULL)
    {
        s_ctx.tab_cb(page, s_ctx.tab_user_data);
    }
}

static void compass_cal_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (s_ctx.compass_cal_cb != NULL)
    {
        s_ctx.compass_cal_cb(s_ctx.compass_cal_user_data);
    }
}

static void compass_cal_close_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    (void)event;
    sensorhub_view_set_compass_calibration_overlay(false);
}
void sensorhub_view_bind_tab_handler(sensorhub_view_tab_cb_t cb, void *user_data)
{
    s_ctx.tab_cb = cb;
    s_ctx.tab_user_data = user_data;
}

void sensorhub_view_bind_compass_cal_handler(sensorhub_view_compass_cal_cb_t cb, void *user_data)
{
    s_ctx.compass_cal_cb = cb;
    s_ctx.compass_cal_user_data = user_data;
}

void sensorhub_view_set_compass_calibration_overlay(bool visible)
{
    if ((!s_ctx.created) || (s_ctx.compass_cal_overlay == NULL))
    {
        return;
    }

    if (visible)
    {
        lv_obj_move_foreground(s_ctx.compass_cal_overlay);
        lv_obj_clear_flag(s_ctx.compass_cal_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(s_ctx.compass_cal_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

void sensorhub_view_update_compass_calibration(uint32_t sample_pct,
                                               uint32_t coverage_pct,
                                               uint32_t remaining_samples,
                                               bool done)
{
    if ((!s_ctx.created) ||
        (s_ctx.compass_cal_sample_arc == NULL) ||
        (s_ctx.compass_cal_coverage_arc == NULL) ||
        (s_ctx.compass_cal_value_label == NULL))
    {
        return;
    }

    uint32_t sample_clamped = clamp_pct_u32(sample_pct);
    uint32_t coverage_clamped = clamp_pct_u32(coverage_pct);

    lv_arc_set_value(s_ctx.compass_cal_sample_arc, (int32_t)sample_clamped);
    lv_arc_set_value(s_ctx.compass_cal_coverage_arc, (int32_t)coverage_clamped);

    lv_label_set_text_fmt(s_ctx.compass_cal_value_label,
                          "%lu%%\n%lu%%",
                          (unsigned long)sample_clamped,
                          (unsigned long)coverage_clamped);

    if (done)
    {
        if (s_ctx.compass_cal_title_label != NULL)
        {
            lv_label_set_text(s_ctx.compass_cal_title_label, "Compass Calibration Done");
        }

        if (s_ctx.compass_cal_progress_label != NULL)
        {
            lv_label_set_text(s_ctx.compass_cal_progress_label, "Sample and coverage reached 100%");
        }

        if (s_ctx.compass_cal_hint_label != NULL)
        {
            lv_label_set_text(s_ctx.compass_cal_hint_label, "Close or tap Calibrate again");
        }

        lv_obj_set_style_arc_color(s_ctx.compass_cal_sample_arc, lv_color_hex(0x22C55E), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(s_ctx.compass_cal_coverage_arc, lv_color_hex(0x10B981), LV_PART_INDICATOR);
        return;
    }

    if (s_ctx.compass_cal_title_label != NULL)
    {
        lv_label_set_text(s_ctx.compass_cal_title_label, "Compass Calibration");
    }

    if (s_ctx.compass_cal_progress_label != NULL)
    {
        lv_label_set_text_fmt(s_ctx.compass_cal_progress_label,
                              "Sample:%lu%%  Coverage:%lu%%\nRemaining:%lu",
                              (unsigned long)sample_clamped,
                              (unsigned long)coverage_clamped,
                              (unsigned long)remaining_samples);
    }

    if (s_ctx.compass_cal_hint_label != NULL)
    {
        lv_label_set_text(s_ctx.compass_cal_hint_label,
                          "Rotate in figure-8 and sweep all directions");
    }

    lv_obj_set_style_arc_color(s_ctx.compass_cal_sample_arc, lv_color_hex(0x60A5FA), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ctx.compass_cal_coverage_arc, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);
}

void sensorhub_view_set_active_page(sensorhub_page_t page)
{
    if ((!s_ctx.created) || (page >= SENSORHUB_PAGE_COUNT))
    {
        return;
    }

    s_ctx.active_page = page;

    for (uint32_t i = 0U; i < SENSORHUB_PAGE_COUNT; i++)
    {
        bool active = (i == (uint32_t)page);

        if (active)
        {
            lv_obj_clear_flag(s_ctx.pages[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(s_ctx.pages[i], LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_style_bg_color(s_ctx.tabs[i],
                                  lv_color_hex(active ? HUB_TAB_BTN_ACTIVE_BG_HEX : HUB_TAB_BTN_BG_HEX),
                                  LV_PART_MAIN);
        lv_obj_set_style_border_color(s_ctx.tabs[i],
                                      lv_color_hex(active ? HUB_TAB_BTN_ACTIVE_BORDER_HEX : HUB_TAB_BTN_BORDER_HEX),
                                      LV_PART_MAIN);
        lv_obj_set_style_text_color(s_ctx.tabs[i],
                                    lv_color_hex(active ? HUB_TAB_BTN_ACTIVE_TEXT_HEX : HUB_TAB_BTN_TEXT_HEX),
                                    LV_PART_MAIN);

        if (s_ctx.tab_labels[i] != NULL)
        {
            lv_obj_set_style_text_color(s_ctx.tab_labels[i],
                                        lv_color_hex(active ? HUB_TAB_BTN_ACTIVE_TEXT_HEX : HUB_TAB_BTN_TEXT_HEX),
                                        LV_PART_MAIN);
        }
    }
}

void sensorhub_view_set_footer_status(const char *text)
{
    if ((!s_ctx.created) || (s_ctx.footer_status == NULL) || (text == NULL))
    {
        return;
    }

    lv_label_set_text(s_ctx.footer_status, text);
}

void sensorhub_view_create(void)
{
    if (s_ctx.created)
    {
        return;
    }

    static const char *tab_text[SENSORHUB_PAGE_COUNT] =
    {
        LV_SYMBOL_HOME " Home",
        LV_SYMBOL_SETTINGS " Env",
        LV_SYMBOL_REFRESH " Motion",
        LV_SYMBOL_GPS " Compass",
        LV_SYMBOL_AUDIO " Audio"
    };

    static const char *motion_text[] =
    {
        "0", "45", "90", "135", "180", "225", "270", "315", "360", NULL
    };

    static const char *compass_text[] =
    {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW", "N", NULL
    };

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(HUB_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    /* Global shell: Header + Tabs + Content + Footer */
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 58);
    lv_obj_set_style_bg_color(header, lv_color_hex(HUB_HEADER_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(HUB_HEADER_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(header, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(header, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(header, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(header, 4, LV_PART_MAIN);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "SensorHub Final Dashboard");
    lv_obj_set_style_text_color(title, lv_color_hex(HUB_TEXT_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_flex_grow(title, 1);

    lv_obj_t *logo = lv_image_create(header);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, 30);
    lv_obj_clear_flag(logo, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *tabs = lv_obj_create(screen);
    lv_obj_set_size(tabs, LV_PCT(100), 48);
    lv_obj_set_style_bg_color(tabs, lv_color_hex(HUB_TAB_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabs, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(tabs, lv_color_hex(HUB_TAB_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(tabs, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(tabs, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_right(tabs, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_top(tabs, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(tabs, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(tabs, 8, LV_PART_MAIN);
    lv_obj_set_layout(tabs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabs, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(tabs, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0U; i < SENSORHUB_PAGE_COUNT; i++)
    {
        lv_obj_t *btn = lv_button_create(tabs);
        lv_obj_set_size(btn, 140, 34);
        lv_obj_set_style_bg_color(btn, lv_color_hex(HUB_TAB_BTN_BG_HEX), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(HUB_TAB_BTN_BORDER_HEX), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, tab_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, tab_text[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(HUB_TAB_BTN_TEXT_HEX), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(label);

        s_ctx.tabs[i] = btn;
        s_ctx.tab_labels[i] = label;
    }

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(HUB_CONTENT_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(content, lv_color_hex(HUB_CONTENT_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 8, LV_PART_MAIN);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    for (uint32_t i = 0U; i < SENSORHUB_PAGE_COUNT; i++)
    {
        lv_obj_t *page = lv_obj_create(content);
        lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
        lv_obj_set_pos(page, 0, 0);
        lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
        lv_obj_set_layout(page, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_gap(page, 8, LV_PART_MAIN);
        lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
        s_ctx.pages[i] = page;
    }

    /* Home page: compact overview cards for all sensor domains. */
    lv_obj_t *home_top = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_HOME]);
    lv_obj_set_width(home_top, LV_PCT(100));
    lv_obj_set_height(home_top, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(home_top, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(home_top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(home_top, 0, LV_PART_MAIN);
    lv_obj_set_layout(home_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(home_top, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(home_top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(home_top, 2, LV_PART_MAIN);

    lv_obj_t *home_title = lv_label_create(home_top);
    lv_label_set_text(home_title, "System Overview");
    lv_obj_set_style_text_color(home_title, lv_color_hex(HUB_TEXT_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(home_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *home_sub = lv_label_create(home_top);
    lv_label_set_text(home_sub, "Live data from onboard sensors (CM55 single-core)");
    lv_obj_set_style_text_color(home_sub, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(home_sub, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *home_grid = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_HOME]);
    lv_obj_set_size(home_grid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(home_grid, 1);
    lv_obj_set_layout(home_grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(home_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_bg_opa(home_grid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(home_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(home_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(home_grid, 8, LV_PART_MAIN);
    lv_obj_clear_flag(home_grid, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card_env = lv_obj_create(home_grid);
    lv_obj_set_size(card_env, LV_PCT(49), 120);
    style_card(card_env);
    lv_obj_set_layout(card_env, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card_env, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card_env, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *card_motion = lv_obj_create(home_grid);
    lv_obj_set_size(card_motion, LV_PCT(49), 120);
    style_card(card_motion);
    lv_obj_set_layout(card_motion, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card_motion, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card_motion, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *card_compass = lv_obj_create(home_grid);
    lv_obj_set_size(card_compass, LV_PCT(49), 120);
    style_card(card_compass);
    lv_obj_set_layout(card_compass, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card_compass, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card_compass, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *card_audio = lv_obj_create(home_grid);
    lv_obj_set_size(card_audio, LV_PCT(49), 120);
    style_card(card_audio);
    lv_obj_set_layout(card_audio, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card_audio, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card_audio, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *home_env_title = lv_label_create(card_env);
    lv_label_set_text(home_env_title, "Env");
    lv_obj_set_style_text_color(home_env_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.home_env_label = lv_label_create(card_env);
    lv_label_set_text(s_ctx.home_env_label, "Pressure -- hPa\nTemp -- C\nHumidity -- %RH");
    lv_obj_set_style_text_color(s_ctx.home_env_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    lv_obj_t *home_motion_title = lv_label_create(card_motion);
    lv_label_set_text(home_motion_title, "Motion");
    lv_obj_set_style_text_color(home_motion_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.home_motion_label = lv_label_create(card_motion);
    lv_label_set_text(s_ctx.home_motion_label, "AccD -- g\nGyZD -- dps");
    lv_obj_set_style_text_color(s_ctx.home_motion_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    lv_obj_t *home_compass_title = lv_label_create(card_compass);
    lv_label_set_text(home_compass_title, "Compass");
    lv_obj_set_style_text_color(home_compass_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.home_compass_label = lv_label_create(card_compass);
    lv_label_set_text(s_ctx.home_compass_label, "Heading -- deg\nDir --");
    lv_obj_set_style_text_color(s_ctx.home_compass_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    lv_obj_t *home_audio_title = lv_label_create(card_audio);
    lv_label_set_text(home_audio_title, "Audio");
    lv_obj_set_style_text_color(home_audio_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.home_audio_label = lv_label_create(card_audio);
    lv_label_set_text(s_ctx.home_audio_label, "L --%  R --%\nBalance --");
    lv_obj_set_style_text_color(s_ctx.home_audio_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    /* Environment page: pressure, temperature, humidity bars. */
    lv_obj_t *env_row = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_ENV]);
    lv_obj_set_width(env_row, LV_PCT(100));
    lv_obj_set_flex_grow(env_row, 1);
    lv_obj_set_layout(env_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(env_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(env_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(env_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(env_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(env_row, 8, LV_PART_MAIN);
    lv_obj_clear_flag(env_row, LV_OBJ_FLAG_SCROLLABLE);

    (void)create_metric_card(env_row,
                             "Pressure",
                             lv_color_hex(0x38BDF8),
                             &s_ctx.env_pressure_label,
                             &s_ctx.env_pressure_bar);

    (void)create_metric_card(env_row,
                             "Temperature",
                             lv_color_hex(0xFB7185),
                             &s_ctx.env_temp_label,
                             &s_ctx.env_temp_bar);

    (void)create_metric_card(env_row,
                             "Humidity",
                             lv_color_hex(0x22C55E),
                             &s_ctx.env_hum_label,
                             &s_ctx.env_hum_bar);

    lv_obj_t *env_hint = lv_label_create(s_ctx.pages[SENSORHUB_PAGE_ENV]);
    lv_label_set_text(env_hint, "DPS368 + SHT4x");
    lv_obj_set_style_text_color(env_hint, lv_color_hex(HUB_TEXT_MUTED_HEX), LV_PART_MAIN);

    /* Motion page: scope + intensity rings built from BMI270 deltas. */
    lv_obj_t *motion_content = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_MOTION]);
    lv_obj_set_width(motion_content, LV_PCT(100));
    lv_obj_set_flex_grow(motion_content, 1);
    lv_obj_set_layout(motion_content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(motion_content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(motion_content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(motion_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(motion_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(motion_content, 8, LV_PART_MAIN);

    lv_obj_t *motion_scope_card = lv_obj_create(motion_content);
    lv_obj_set_width(motion_scope_card, 336);
    lv_obj_set_height(motion_scope_card, LV_PCT(100));
    lv_obj_set_layout(motion_scope_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(motion_scope_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(motion_scope_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    style_card(motion_scope_card);

    lv_obj_t *motion_scope_title = lv_label_create(motion_scope_card);
    lv_label_set_text(motion_scope_title, "Motion Scope");
    lv_obj_set_style_text_color(motion_scope_title, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(motion_scope_title, &lv_font_montserrat_20, LV_PART_MAIN);

    s_ctx.motion_scope_scale = lv_scale_create(motion_scope_card);
    lv_obj_set_size(s_ctx.motion_scope_scale, 220, 220);
    lv_scale_set_mode(s_ctx.motion_scope_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(s_ctx.motion_scope_scale, 0, 360);
    lv_scale_set_total_tick_count(s_ctx.motion_scope_scale, 41);
    lv_scale_set_major_tick_every(s_ctx.motion_scope_scale, 5);
    lv_scale_set_text_src(s_ctx.motion_scope_scale, motion_text);
    lv_scale_set_label_show(s_ctx.motion_scope_scale, true);
    lv_scale_set_angle_range(s_ctx.motion_scope_scale, 360);
    lv_scale_set_rotation(s_ctx.motion_scope_scale, 0);
    lv_obj_set_style_bg_opa(s_ctx.motion_scope_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.motion_scope_scale, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.motion_scope_scale, lv_color_hex(0x2C4364), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ctx.motion_scope_scale, lv_color_hex(0xD0E7FF), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(s_ctx.motion_scope_scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_length(s_ctx.motion_scope_scale, 10, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(s_ctx.motion_scope_scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(s_ctx.motion_scope_scale, lv_color_hex(0x8DD4FF), LV_PART_INDICATOR);
    lv_obj_set_style_length(s_ctx.motion_scope_scale, 5, LV_PART_ITEMS);
    lv_obj_set_style_line_width(s_ctx.motion_scope_scale, 1, LV_PART_ITEMS);
    lv_obj_set_style_line_color(s_ctx.motion_scope_scale, lv_color_hex(0x4A658A), LV_PART_ITEMS);
    s_ctx.motion_scope_needle = lv_line_create(s_ctx.motion_scope_scale);
    lv_obj_set_style_line_width(s_ctx.motion_scope_needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(s_ctx.motion_scope_needle, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_ctx.motion_scope_needle, lv_color_hex(0x22D3EE), LV_PART_MAIN);
    lv_scale_set_line_needle_value(s_ctx.motion_scope_scale, s_ctx.motion_scope_needle, 18, 0);
    lv_obj_add_flag(s_ctx.motion_scope_needle, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *motion_center = lv_obj_create(s_ctx.motion_scope_scale);
    lv_obj_set_size(motion_center, 10, 10);
    lv_obj_set_style_radius(motion_center, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(motion_center, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(motion_center, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_center(motion_center);

    s_ctx.motion_angle_label = lv_label_create(motion_scope_card);
    lv_label_set_text(s_ctx.motion_angle_label, "Motion Angle: -- (still)");
    lv_obj_set_style_text_color(s_ctx.motion_angle_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_angle_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_ctx.motion_level_label = lv_label_create(motion_scope_card);
    lv_label_set_text(s_ctx.motion_level_label, "Level: STILL");
    lv_obj_set_style_text_color(s_ctx.motion_level_label, lv_color_hex(HUB_TEXT_MUTED_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_level_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *motion_intensity_card = lv_obj_create(motion_content);
    lv_obj_set_flex_grow(motion_intensity_card, 1);
    lv_obj_set_height(motion_intensity_card, LV_PCT(100));
    lv_obj_set_layout(motion_intensity_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(motion_intensity_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(motion_intensity_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    style_card(motion_intensity_card);

    lv_obj_t *motion_intensity_title = lv_label_create(motion_intensity_card);
    lv_label_set_text(motion_intensity_title, "Motion Intensity Rings");
    lv_obj_set_style_text_color(motion_intensity_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(motion_intensity_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *ring_wrap = lv_obj_create(motion_intensity_card);
    lv_obj_set_size(ring_wrap, 190, 190);
    lv_obj_set_style_bg_opa(ring_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ring_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ring_wrap, 0, LV_PART_MAIN);

    s_ctx.motion_intensity_acc_arc = lv_arc_create(ring_wrap);
    lv_obj_set_size(s_ctx.motion_intensity_acc_arc, 190, 190);
    lv_obj_center(s_ctx.motion_intensity_acc_arc);
    lv_arc_set_range(s_ctx.motion_intensity_acc_arc, 0, 100);
    lv_arc_set_value(s_ctx.motion_intensity_acc_arc, 0);
    lv_arc_set_rotation(s_ctx.motion_intensity_acc_arc, 270);
    lv_arc_set_bg_angles(s_ctx.motion_intensity_acc_arc, 0, 360);
    lv_obj_remove_style(s_ctx.motion_intensity_acc_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_ctx.motion_intensity_acc_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ctx.motion_intensity_acc_arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.motion_intensity_acc_arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ctx.motion_intensity_acc_arc, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.motion_intensity_acc_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

    s_ctx.motion_intensity_gyr_arc = lv_arc_create(ring_wrap);
    lv_obj_set_size(s_ctx.motion_intensity_gyr_arc, 142, 142);
    lv_obj_center(s_ctx.motion_intensity_gyr_arc);
    lv_arc_set_range(s_ctx.motion_intensity_gyr_arc, 0, 100);
    lv_arc_set_value(s_ctx.motion_intensity_gyr_arc, 0);
    lv_arc_set_rotation(s_ctx.motion_intensity_gyr_arc, 270);
    lv_arc_set_bg_angles(s_ctx.motion_intensity_gyr_arc, 0, 360);
    lv_obj_remove_style(s_ctx.motion_intensity_gyr_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_ctx.motion_intensity_gyr_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ctx.motion_intensity_gyr_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.motion_intensity_gyr_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ctx.motion_intensity_gyr_arc, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.motion_intensity_gyr_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

    s_ctx.motion_intensity_value_label = lv_label_create(ring_wrap);
    lv_label_set_text(s_ctx.motion_intensity_value_label, "A:0%\nG:0%");
    lv_obj_set_style_text_color(s_ctx.motion_intensity_value_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_ctx.motion_intensity_value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_intensity_value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(s_ctx.motion_intensity_value_label);

    s_ctx.motion_intensity_state_label = lv_label_create(motion_intensity_card);
    lv_label_set_text(s_ctx.motion_intensity_state_label, "State: STILL");
    lv_obj_set_style_text_color(s_ctx.motion_intensity_state_label, lv_color_hex(HUB_TEXT_MUTED_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_intensity_state_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_ctx.motion_delta_acc_label = lv_label_create(motion_intensity_card);
    lv_label_set_text(s_ctx.motion_delta_acc_label, "Delta Acc XY: 0.000 g");
    lv_obj_set_style_text_color(s_ctx.motion_delta_acc_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_delta_acc_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_ctx.motion_delta_gyr_label = lv_label_create(motion_intensity_card);
    lv_label_set_text(s_ctx.motion_delta_gyr_label, "|Delta Gyro Z|: 0.0 dps");
    lv_obj_set_style_text_color(s_ctx.motion_delta_gyr_label, lv_color_hex(0xFDBA74), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.motion_delta_gyr_label, &lv_font_montserrat_16, LV_PART_MAIN);

    /* Compass page: heading dial, XYZ axes, and calibration controls. */
    lv_obj_t *compass_content = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_COMPASS]);
    lv_obj_set_width(compass_content, LV_PCT(100));
    lv_obj_set_flex_grow(compass_content, 1);
    lv_obj_set_layout(compass_content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(compass_content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(compass_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compass_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(compass_content, 8, LV_PART_MAIN);

    lv_obj_t *compass_card = lv_obj_create(compass_content);
    lv_obj_set_width(compass_card, 248);
    lv_obj_set_height(compass_card, LV_PCT(100));
    lv_obj_set_layout(compass_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(compass_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    style_card(compass_card);

    lv_obj_t *compass_title = lv_label_create(compass_card);
    lv_label_set_text(compass_title, "Compass");
    lv_obj_set_style_text_color(compass_title, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(compass_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *compass_center_wrap = lv_obj_create(compass_card);
    lv_obj_set_width(compass_center_wrap, LV_PCT(100));
    lv_obj_set_flex_grow(compass_center_wrap, 1);
    lv_obj_set_layout(compass_center_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_center_wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(compass_center_wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(compass_center_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(compass_center_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compass_center_wrap, 0, LV_PART_MAIN);

    s_ctx.compass_scale = lv_scale_create(compass_center_wrap);
    lv_obj_set_size(s_ctx.compass_scale, 172, 172);
    lv_scale_set_mode(s_ctx.compass_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(s_ctx.compass_scale, 0, 360);
    lv_scale_set_total_tick_count(s_ctx.compass_scale, 41);
    lv_scale_set_major_tick_every(s_ctx.compass_scale, 5);
    lv_scale_set_label_show(s_ctx.compass_scale, true);
    lv_scale_set_text_src(s_ctx.compass_scale, compass_text);
    lv_scale_set_angle_range(s_ctx.compass_scale, 360);
    lv_scale_set_rotation(s_ctx.compass_scale, 270);
    lv_obj_set_style_bg_opa(s_ctx.compass_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.compass_scale, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.compass_scale, lv_color_hex(0x2C4364), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ctx.compass_scale, lv_color_hex(0xD0E7FF), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(s_ctx.compass_scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_length(s_ctx.compass_scale, 10, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(s_ctx.compass_scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(s_ctx.compass_scale, lv_color_hex(0x8DD4FF), LV_PART_INDICATOR);
    lv_obj_set_style_length(s_ctx.compass_scale, 5, LV_PART_ITEMS);
    lv_obj_set_style_line_width(s_ctx.compass_scale, 1, LV_PART_ITEMS);
    lv_obj_set_style_line_color(s_ctx.compass_scale, lv_color_hex(0x4A658A), LV_PART_ITEMS);

    s_ctx.compass_needle = lv_line_create(s_ctx.compass_scale);
    lv_obj_set_style_line_width(s_ctx.compass_needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(s_ctx.compass_needle, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_ctx.compass_needle, lv_color_hex(0xEF4444), LV_PART_MAIN);
    lv_scale_set_line_needle_value(s_ctx.compass_scale, s_ctx.compass_needle, 72, 0);

    lv_obj_t *compass_center_dot = lv_obj_create(s_ctx.compass_scale);
    lv_obj_set_size(compass_center_dot, 10, 10);
    lv_obj_set_style_radius(compass_center_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(compass_center_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(compass_center_dot, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_center(compass_center_dot);

    s_ctx.compass_heading_label = lv_label_create(compass_card);
    lv_label_set_text(s_ctx.compass_heading_label, "000 deg");
    lv_obj_set_style_text_color(s_ctx.compass_heading_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_heading_label, &lv_font_montserrat_22, LV_PART_MAIN);

    s_ctx.compass_dir_label = lv_label_create(compass_card);
    lv_label_set_text(s_ctx.compass_dir_label, "N");
    lv_obj_set_style_text_color(s_ctx.compass_dir_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_dir_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_t *axis_card = lv_obj_create(compass_content);
    lv_obj_set_flex_grow(axis_card, 1);
    lv_obj_set_height(axis_card, LV_PCT(100));
    lv_obj_set_layout(axis_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(axis_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(axis_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    style_card(axis_card);

    lv_obj_t *axis_title = lv_label_create(axis_card);
    lv_label_set_text(axis_title, "Magnetic Axis (uT)");
    lv_obj_set_style_text_color(axis_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(axis_title, &lv_font_montserrat_20, LV_PART_MAIN);

    (void)create_compass_axis_row(axis_card,
                                  "X Axis",
                                  lv_color_hex(0xF87171),
                                  &s_ctx.compass_x_value_label,
                                  &s_ctx.compass_x_bar);

    (void)create_compass_axis_row(axis_card,
                                  "Y Axis",
                                  lv_color_hex(0x4ADE80),
                                  &s_ctx.compass_y_value_label,
                                  &s_ctx.compass_y_bar);

    (void)create_compass_axis_row(axis_card,
                                  "Z Axis",
                                  lv_color_hex(0x60A5FA),
                                  &s_ctx.compass_z_value_label,
                                  &s_ctx.compass_z_bar);

    lv_obj_t *compass_summary = lv_obj_create(axis_card);
    lv_obj_set_width(compass_summary, LV_PCT(100));
    lv_obj_set_height(compass_summary, LV_SIZE_CONTENT);
    lv_obj_set_layout(compass_summary, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(compass_summary,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(compass_summary, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(compass_summary, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compass_summary, 0, LV_PART_MAIN);

    s_ctx.compass_field_label = lv_label_create(compass_summary);
    lv_label_set_text(s_ctx.compass_field_label, "Field: 0.0 uT");
    lv_obj_set_style_text_color(s_ctx.compass_field_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_field_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_ctx.compass_temp_label = lv_label_create(compass_summary);
    lv_label_set_text(s_ctx.compass_temp_label, "Temp: --.- C");
    lv_obj_set_style_text_color(s_ctx.compass_temp_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_temp_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_t *cal_row = lv_obj_create(axis_card);
    lv_obj_set_width(cal_row, LV_PCT(100));
    lv_obj_set_height(cal_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(cal_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cal_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cal_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cal_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cal_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cal_row, 0, LV_PART_MAIN);

    lv_obj_t *cal_hint = lv_label_create(cal_row);
    lv_label_set_text(cal_hint, "Need better heading? Re-run calibration");
    lv_obj_set_style_text_color(cal_hint, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(cal_hint, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_flex_grow(cal_hint, 1);

    lv_obj_t *cal_btn = lv_button_create(cal_row);
    lv_obj_set_size(cal_btn, 118, 34);
    lv_obj_set_style_bg_color(cal_btn, lv_color_hex(0x1D4ED8), LV_PART_MAIN);
    lv_obj_set_style_border_color(cal_btn, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_border_width(cal_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(cal_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(cal_btn, compass_cal_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cal_btn_label = lv_label_create(cal_btn);
    lv_label_set_text(cal_btn_label, LV_SYMBOL_REFRESH " Calibrate");
    lv_obj_set_style_text_color(cal_btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(cal_btn_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(cal_btn_label);

    /* Modal-like calibration overlay rendered inside Compass content area. */
    s_ctx.compass_cal_overlay = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_COMPASS]);
    lv_obj_set_size(s_ctx.compass_cal_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_ctx.compass_cal_overlay, lv_color_hex(0x020617), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_ctx.compass_cal_overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ctx.compass_cal_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ctx.compass_cal_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_ctx.compass_cal_overlay, 0, LV_PART_MAIN);
    lv_obj_set_layout(s_ctx.compass_cal_overlay, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_ctx.compass_cal_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ctx.compass_cal_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_ctx.compass_cal_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_ctx.compass_cal_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *overlay_card = lv_obj_create(s_ctx.compass_cal_overlay);
    lv_obj_set_size(overlay_card, LV_PCT(78), LV_PCT(74));
    lv_obj_set_layout(overlay_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(overlay_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(overlay_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(overlay_card, lv_color_hex(0x10213A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay_card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(overlay_card, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay_card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(overlay_card, 4, LV_PART_MAIN);
    lv_obj_clear_flag(overlay_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *overlay_top = lv_obj_create(overlay_card);
    lv_obj_set_width(overlay_top, LV_PCT(100));
    lv_obj_set_height(overlay_top, LV_SIZE_CONTENT);
    lv_obj_set_layout(overlay_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(overlay_top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(overlay_top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(overlay_top, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay_top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay_top, 0, LV_PART_MAIN);

    s_ctx.compass_cal_title_label = lv_label_create(overlay_top);
    lv_label_set_text(s_ctx.compass_cal_title_label, "Compass Calibration");
    lv_obj_set_style_text_color(s_ctx.compass_cal_title_label, lv_color_hex(HUB_TEXT_TITLE_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_cal_title_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *close_btn = lv_button_create(overlay_top);
    lv_obj_set_size(close_btn, 30, 24);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x1E3A5F), LV_PART_MAIN);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0x60A5FA), LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(close_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, compass_cal_close_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(close_label);

    lv_obj_t *overlay_ring_wrap = lv_obj_create(overlay_card);
    lv_obj_set_size(overlay_ring_wrap, 92, 92);
    lv_obj_set_style_bg_opa(overlay_ring_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay_ring_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay_ring_wrap, 0, LV_PART_MAIN);

    s_ctx.compass_cal_sample_arc = lv_arc_create(overlay_ring_wrap);
    lv_obj_set_size(s_ctx.compass_cal_sample_arc, 92, 92);
    lv_obj_center(s_ctx.compass_cal_sample_arc);
    lv_arc_set_range(s_ctx.compass_cal_sample_arc, 0, 100);
    lv_arc_set_value(s_ctx.compass_cal_sample_arc, 0);
    lv_arc_set_rotation(s_ctx.compass_cal_sample_arc, 270);
    lv_arc_set_bg_angles(s_ctx.compass_cal_sample_arc, 0, 360);
    lv_obj_remove_style(s_ctx.compass_cal_sample_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_ctx.compass_cal_sample_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ctx.compass_cal_sample_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.compass_cal_sample_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ctx.compass_cal_sample_arc, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.compass_cal_sample_arc, lv_color_hex(0x60A5FA), LV_PART_INDICATOR);

    s_ctx.compass_cal_coverage_arc = lv_arc_create(overlay_ring_wrap);
    lv_obj_set_size(s_ctx.compass_cal_coverage_arc, 66, 66);
    lv_obj_center(s_ctx.compass_cal_coverage_arc);
    lv_arc_set_range(s_ctx.compass_cal_coverage_arc, 0, 100);
    lv_arc_set_value(s_ctx.compass_cal_coverage_arc, 0);
    lv_arc_set_rotation(s_ctx.compass_cal_coverage_arc, 270);
    lv_arc_set_bg_angles(s_ctx.compass_cal_coverage_arc, 0, 360);
    lv_obj_remove_style(s_ctx.compass_cal_coverage_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_ctx.compass_cal_coverage_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ctx.compass_cal_coverage_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ctx.compass_cal_coverage_arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ctx.compass_cal_coverage_arc, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ctx.compass_cal_coverage_arc, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);

    s_ctx.compass_cal_value_label = lv_label_create(overlay_ring_wrap);
    lv_label_set_text(s_ctx.compass_cal_value_label, "0%\n0%");
    lv_obj_set_style_text_color(s_ctx.compass_cal_value_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_cal_value_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_ctx.compass_cal_value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(s_ctx.compass_cal_value_label);

    s_ctx.compass_cal_progress_label = lv_label_create(overlay_card);
    lv_label_set_text(s_ctx.compass_cal_progress_label, "Sample:0%  Coverage:0%\nRemaining:0");
    lv_obj_set_style_text_color(s_ctx.compass_cal_progress_label, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_cal_progress_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_width(s_ctx.compass_cal_progress_label, LV_PCT(100));
    lv_obj_set_style_text_align(s_ctx.compass_cal_progress_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    s_ctx.compass_cal_hint_label = lv_label_create(overlay_card);
    lv_label_set_text(s_ctx.compass_cal_hint_label, "Rotate in figure-8 and sweep all directions");
    lv_obj_set_style_text_color(s_ctx.compass_cal_hint_label, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.compass_cal_hint_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_width(s_ctx.compass_cal_hint_label, LV_PCT(100));
    lv_obj_set_style_text_align(s_ctx.compass_cal_hint_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    /* Audio page: stereo UI level bars + L/R balance bar. */
    lv_obj_t *audio_row = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_AUDIO]);
    lv_obj_set_width(audio_row, LV_PCT(100));
    lv_obj_set_flex_grow(audio_row, 1);
    lv_obj_set_layout(audio_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(audio_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(audio_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(audio_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(audio_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(audio_row, 8, LV_PART_MAIN);

    lv_obj_t *left_card = lv_obj_create(audio_row);
    lv_obj_set_size(left_card, LV_PCT(49), LV_PCT(100));
    lv_obj_set_layout(left_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_card, LV_FLEX_FLOW_COLUMN);
    style_card(left_card);

    lv_obj_t *right_card = lv_obj_create(audio_row);
    lv_obj_set_size(right_card, LV_PCT(49), LV_PCT(100));
    lv_obj_set_layout(right_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_card, LV_FLEX_FLOW_COLUMN);
    style_card(right_card);

    lv_obj_t *left_title = lv_label_create(left_card);
    lv_label_set_text(left_title, "Left Channel");
    lv_obj_set_style_text_color(left_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.audio_left_label = lv_label_create(left_card);
    lv_label_set_text(s_ctx.audio_left_label, "UI --% | Avg --");
    lv_obj_set_style_text_color(s_ctx.audio_left_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    s_ctx.audio_left_bar = lv_bar_create(left_card);
    lv_obj_set_size(s_ctx.audio_left_bar, LV_PCT(100), 14);
    lv_bar_set_range(s_ctx.audio_left_bar, 0, 100);
    lv_obj_set_style_bg_color(s_ctx.audio_left_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ctx.audio_left_bar, lv_color_hex(0x22C55E), LV_PART_INDICATOR);

    lv_obj_t *right_title = lv_label_create(right_card);
    lv_label_set_text(right_title, "Right Channel");
    lv_obj_set_style_text_color(right_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.audio_right_label = lv_label_create(right_card);
    lv_label_set_text(s_ctx.audio_right_label, "UI --% | Avg --");
    lv_obj_set_style_text_color(s_ctx.audio_right_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    s_ctx.audio_right_bar = lv_bar_create(right_card);
    lv_obj_set_size(s_ctx.audio_right_bar, LV_PCT(100), 14);
    lv_bar_set_range(s_ctx.audio_right_bar, 0, 100);
    lv_obj_set_style_bg_color(s_ctx.audio_right_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ctx.audio_right_bar, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);

    lv_obj_t *balance_card = lv_obj_create(s_ctx.pages[SENSORHUB_PAGE_AUDIO]);
    lv_obj_set_width(balance_card, LV_PCT(100));
    lv_obj_set_height(balance_card, 78);
    lv_obj_set_layout(balance_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(balance_card, LV_FLEX_FLOW_COLUMN);
    style_card(balance_card);

    lv_obj_t *balance_title = lv_label_create(balance_card);
    lv_label_set_text(balance_title, "Balance (L-R)");
    lv_obj_set_style_text_color(balance_title, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);

    s_ctx.audio_balance_bar = lv_bar_create(balance_card);
    lv_obj_set_size(s_ctx.audio_balance_bar, LV_PCT(100), 10);
    lv_bar_set_range(s_ctx.audio_balance_bar, -100, 100);
    lv_bar_set_value(s_ctx.audio_balance_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_ctx.audio_balance_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ctx.audio_balance_bar, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);

    s_ctx.audio_balance_label = lv_label_create(balance_card);
    lv_label_set_text(s_ctx.audio_balance_label, "Balance: 0");
    lv_obj_set_style_text_color(s_ctx.audio_balance_label, lv_color_hex(HUB_TEXT_MAIN_HEX), LV_PART_MAIN);

    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_set_size(footer, LV_PCT(100), 34);
    lv_obj_set_style_bg_color(footer, lv_color_hex(HUB_FOOTER_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(footer, lv_color_hex(HUB_FOOTER_BORDER_HEX), LV_PART_MAIN);
    lv_obj_set_style_radius(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(footer, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(footer, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(footer, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(footer, 4, LV_PART_MAIN);
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    s_ctx.footer_status = lv_label_create(footer);
    lv_label_set_text(s_ctx.footer_status, "Page: Home | Initializing sensors...");
    lv_obj_set_style_text_color(s_ctx.footer_status, lv_color_hex(HUB_TEXT_SUB_HEX), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.footer_status, &lv_font_montserrat_14, LV_PART_MAIN);

    s_ctx.created = true;
    sensorhub_view_update_compass_calibration(0U, 0U, 0U, false);
    sensorhub_view_set_active_page(SENSORHUB_PAGE_HOME);
}
/* Update Environment page and Home summary from DPS368/SHT4x samples. */
void sensorhub_view_update_env(const dps368_sample_t *dps, const sht4x_sample_t *sht)
{
    if (!s_ctx.created)
    {
        return;
    }

    if (dps != NULL)
    {
        float pressure_pct = clampf((dps->pressure_hpa / 1100.0f) * 100.0f, 0.0f, 100.0f);
        float temp_pct = clampf((dps->temperature_c / 100.0f) * 100.0f, 0.0f, 100.0f);

        lv_label_set_text_fmt(s_ctx.env_pressure_label, "%.1f hPa", (double)dps->pressure_hpa);
        lv_bar_set_value(s_ctx.env_pressure_bar, (int32_t)clamp_pct_from_float(pressure_pct), LV_ANIM_ON);

        lv_label_set_text_fmt(s_ctx.env_temp_label, "%.1f C", (double)dps->temperature_c);
        lv_bar_set_value(s_ctx.env_temp_bar, (int32_t)clamp_pct_from_float(temp_pct), LV_ANIM_ON);
    }

    if (sht != NULL)
    {
        float hum_pct = clampf(sht->humidity_rh, 0.0f, 100.0f);
        lv_label_set_text_fmt(s_ctx.env_hum_label, "%.1f %%RH", (double)sht->humidity_rh);
        lv_bar_set_value(s_ctx.env_hum_bar, (int32_t)clamp_pct_from_float(hum_pct), LV_ANIM_ON);
    }

    if ((dps != NULL) || (sht != NULL))
    {
        lv_label_set_text_fmt(s_ctx.home_env_label,
                              "Pressure %.1f hPa\nTemp %.1f C\nHumidity %.1f %%RH",
                              (double)(dps != NULL ? dps->pressure_hpa : 0.0f),
                              (double)(dps != NULL ? dps->temperature_c : 0.0f),
                              (double)(sht != NULL ? sht->humidity_rh : 0.0f));
    }
}

/* Update Motion page from BMI270 deltas with still-detection. */
void sensorhub_view_update_motion(const bmi270_sample_t *motion)
{
    if ((!s_ctx.created) || (motion == NULL))
    {
        return;
    }

    float acc_xy_delta_g = 0.0f;
    float gyr_z_delta_abs_dps = 0.0f;

    if (s_ctx.motion_has_prev)
    {
        float dx = motion->acc_g_x - s_ctx.motion_prev_sample.acc_g_x;
        float dy = motion->acc_g_y - s_ctx.motion_prev_sample.acc_g_y;
        acc_xy_delta_g = sqrtf((dx * dx) + (dy * dy));
        gyr_z_delta_abs_dps = fabsf(motion->gyr_dps_z - s_ctx.motion_prev_sample.gyr_dps_z);
    }

    s_ctx.motion_prev_sample = *motion;
    s_ctx.motion_has_prev = true;

    bool motion_active = ((acc_xy_delta_g >= 0.020f) || (gyr_z_delta_abs_dps >= 4.0f));
    hub_motion_level_t level = motion_level_from_delta(acc_xy_delta_g, gyr_z_delta_abs_dps);

    int32_t acc_pct = clamp_i32((int32_t)((acc_xy_delta_g / 1.20f) * 100.0f + 0.5f), 0, 100);
    int32_t gyr_pct = clamp_i32((int32_t)((gyr_z_delta_abs_dps / 320.0f) * 100.0f + 0.5f), 0, 100);

    if (motion_active)
    {
        float angle_deg = atan2f(motion->acc_g_y, motion->acc_g_x) * (180.0f / HUB_PI_F);
        if (angle_deg < 0.0f)
        {
            angle_deg += 360.0f;
        }

        int32_t angle_i = clamp_i32((int32_t)(angle_deg + 0.5f), 0, 359);
        int32_t needle_len = clamp_i32((int32_t)(18.0f + (acc_xy_delta_g * 120.0f)), 18, 108);

        lv_obj_remove_flag(s_ctx.motion_scope_needle, LV_OBJ_FLAG_HIDDEN);
        lv_scale_set_line_needle_value(s_ctx.motion_scope_scale, s_ctx.motion_scope_needle, needle_len, angle_i);

        lv_label_set_text_fmt(s_ctx.motion_angle_label, "Motion Angle: %03ld deg", (long)angle_i);
        lv_label_set_text(s_ctx.motion_level_label, motion_level_text(level));
        lv_obj_set_style_text_color(s_ctx.motion_level_label, motion_level_color(level), LV_PART_MAIN);

        lv_label_set_text_fmt(s_ctx.motion_intensity_state_label,
                              "State: %s",
                              (level == HUB_MOTION_HIGH) ? "HIGH" :
                              (level == HUB_MOTION_MEDIUM) ? "MEDIUM" : "LOW");
        lv_obj_set_style_text_color(s_ctx.motion_intensity_state_label, motion_level_color(level), LV_PART_MAIN);

        lv_obj_set_style_arc_color(s_ctx.motion_intensity_acc_arc, motion_level_color(level), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(s_ctx.motion_intensity_gyr_arc, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    }
    else
    {
        lv_obj_add_flag(s_ctx.motion_scope_needle, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_ctx.motion_angle_label, "Motion Angle: -- (still)");
        lv_label_set_text(s_ctx.motion_level_label, "Level: STILL");
        lv_obj_set_style_text_color(s_ctx.motion_level_label, lv_color_hex(HUB_TEXT_MUTED_HEX), LV_PART_MAIN);

        lv_label_set_text(s_ctx.motion_intensity_state_label, "State: STILL");
        lv_obj_set_style_text_color(s_ctx.motion_intensity_state_label, lv_color_hex(HUB_TEXT_MUTED_HEX), LV_PART_MAIN);

        lv_obj_set_style_arc_color(s_ctx.motion_intensity_acc_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(s_ctx.motion_intensity_gyr_arc, lv_color_hex(0x334155), LV_PART_INDICATOR);

        acc_pct = 0;
        gyr_pct = 0;
    }

    lv_arc_set_value(s_ctx.motion_intensity_acc_arc, acc_pct);
    lv_arc_set_value(s_ctx.motion_intensity_gyr_arc, gyr_pct);

    lv_label_set_text_fmt(s_ctx.motion_intensity_value_label, "A:%ld%%\nG:%ld%%", (long)acc_pct, (long)gyr_pct);
    lv_label_set_text_fmt(s_ctx.motion_delta_acc_label, "Delta Acc XY: %.3f g", (double)acc_xy_delta_g);
    lv_label_set_text_fmt(s_ctx.motion_delta_gyr_label, "|Delta Gyro Z|: %.1f dps", (double)gyr_z_delta_abs_dps);

    lv_label_set_text_fmt(s_ctx.home_motion_label,
                          "AccD %.3f g\nGyZD %.1f dps",
                          (double)acc_xy_delta_g,
                          (double)gyr_z_delta_abs_dps);
}

/* Update Compass page heading/axis widgets from BMM350 sample. */
void sensorhub_view_update_compass(const bmm350_sample_t *mag)
{
    if ((!s_ctx.created) || (mag == NULL))
    {
        return;
    }

    int32_t heading = (int32_t)(mag->heading_deg + 0.5f);
    if (heading >= 360)
    {
        heading = 0;
    }

    lv_scale_set_line_needle_value(s_ctx.compass_scale, s_ctx.compass_needle, 72, heading);
    lv_label_set_text_fmt(s_ctx.compass_heading_label, "%03ld deg", (long)heading);
    lv_label_set_text(s_ctx.compass_dir_label, heading_to_cardinal(mag->heading_deg));

    lv_label_set_text_fmt(s_ctx.compass_x_value_label, "%.1f uT", (double)mag->x_ut);
    lv_label_set_text_fmt(s_ctx.compass_y_value_label, "%.1f uT", (double)mag->y_ut);
    lv_label_set_text_fmt(s_ctx.compass_z_value_label, "%.1f uT", (double)mag->z_ut);

    lv_bar_set_start_value(s_ctx.compass_x_bar, 0, LV_ANIM_OFF);
    lv_bar_set_start_value(s_ctx.compass_y_bar, 0, LV_ANIM_OFF);
    lv_bar_set_start_value(s_ctx.compass_z_bar, 0, LV_ANIM_OFF);
    lv_bar_set_value(s_ctx.compass_x_bar, clamp_i32((int32_t)mag->x_ut, -HUB_COMPASS_AXIS_MAX_UT, HUB_COMPASS_AXIS_MAX_UT), LV_ANIM_OFF);
    lv_bar_set_value(s_ctx.compass_y_bar, clamp_i32((int32_t)mag->y_ut, -HUB_COMPASS_AXIS_MAX_UT, HUB_COMPASS_AXIS_MAX_UT), LV_ANIM_OFF);
    lv_bar_set_value(s_ctx.compass_z_bar, clamp_i32((int32_t)mag->z_ut, -HUB_COMPASS_AXIS_MAX_UT, HUB_COMPASS_AXIS_MAX_UT), LV_ANIM_OFF);

    lv_label_set_text_fmt(s_ctx.compass_field_label, "Field: %.1f uT", (double)mag->field_strength_ut);
    lv_label_set_text_fmt(s_ctx.compass_temp_label, "Temp: %.1f C", (double)mag->temperature_c);

    lv_label_set_text_fmt(s_ctx.home_compass_label,
                          "Heading %03ld deg\nDir %s",
                          (long)heading,
                          heading_to_cardinal(mag->heading_deg));
}

/* Update Audio page and Home audio summary from latest mic sample. */
void sensorhub_view_update_audio(const mic_presenter_sample_t *audio)
{
    if ((!s_ctx.created) || (audio == NULL))
    {
        return;
    }

    lv_label_set_text_fmt(s_ctx.audio_left_label,
                          "UI %lu%% | Avg %lu",
                          (unsigned long)audio->left_ui_pct,
                          (unsigned long)audio->left_avg_abs);

    lv_label_set_text_fmt(s_ctx.audio_right_label,
                          "UI %lu%% | Avg %lu",
                          (unsigned long)audio->right_ui_pct,
                          (unsigned long)audio->right_avg_abs);

    lv_bar_set_value(s_ctx.audio_left_bar, (int32_t)audio->left_ui_pct, LV_ANIM_ON);
    lv_bar_set_value(s_ctx.audio_right_bar, (int32_t)audio->right_ui_pct, LV_ANIM_ON);
    lv_bar_set_value(s_ctx.audio_balance_bar, audio->balance_lr, LV_ANIM_ON);

    lv_label_set_text_fmt(s_ctx.audio_balance_label,
                          "Balance: %ld (%s)",
                          (long)audio->balance_lr,
                          (audio->balance_lr > 3) ? "L" : ((audio->balance_lr < -3) ? "R" : "CENTER"));

    lv_label_set_text_fmt(s_ctx.home_audio_label,
                          "L %lu%%  R %lu%%\nBalance %ld",
                          (unsigned long)audio->left_ui_pct,
                          (unsigned long)audio->right_ui_pct,
                          (long)audio->balance_lr);
}







