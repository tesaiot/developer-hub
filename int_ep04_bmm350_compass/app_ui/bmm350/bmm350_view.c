#include "bmm350_view.h"

#include "lvgl.h"

#include "bmm350/bmm350_config.h"

typedef struct
{
    lv_obj_t *status_label;
    lv_obj_t *calib_label;
    lv_obj_t *calibrate_btn;

    lv_obj_t *heading_scale;
    lv_obj_t *heading_needle;
    lv_obj_t *heading_value_label;
    lv_obj_t *heading_dir_label;

    lv_obj_t *x_value_label;
    lv_obj_t *y_value_label;
    lv_obj_t *z_value_label;

    lv_obj_t *x_bar;
    lv_obj_t *y_bar;
    lv_obj_t *z_bar;

    lv_obj_t *field_value_label;
    lv_obj_t *temp_value_label;

    lv_obj_t *calib_overlay;
    lv_obj_t *calib_sample_bar;
    lv_obj_t *calib_coverage_bar;
    lv_obj_t *calib_sample_label;
    lv_obj_t *calib_coverage_label;
    lv_obj_t *calib_hint_label;

    bmm350_view_calibrate_cb_t calibrate_cb;
    void *calibrate_user_data;
} bmm350_view_ctx_t;

static bmm350_view_ctx_t s_view;

/* Keep value widgets in one context so presenter can update only data. */

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

static void calibrate_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (NULL != s_view.calibrate_cb)
    {
        s_view.calibrate_cb(s_view.calibrate_user_data);
    }
}

static lv_obj_t *create_axis_row(lv_obj_t *parent,
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
    lv_obj_set_style_border_color(row, lv_color_hex(0x2B4668), LV_PART_MAIN);
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
    lv_obj_set_style_text_color(*out_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(*out_value_label, &lv_font_montserrat_16, LV_PART_MAIN);

    *out_bar = lv_bar_create(row);
    lv_obj_set_width(*out_bar, LV_PCT(100));
    lv_obj_set_height(*out_bar, 14);
    lv_bar_set_range(*out_bar, -BMM350_AXIS_BAR_MAX_UT, BMM350_AXIS_BAR_MAX_UT);
    lv_bar_set_mode(*out_bar, LV_BAR_MODE_RANGE);
    lv_bar_set_start_value(*out_bar, 0, LV_ANIM_OFF);
    lv_bar_set_value(*out_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(*out_bar, 7, LV_PART_MAIN);
    lv_obj_set_style_border_width(*out_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, color, LV_PART_INDICATOR);

    return row;
}

static lv_obj_t *create_progress_row(lv_obj_t *parent,
                                     const char *title_text,
                                     lv_color_t indicator_color,
                                     lv_obj_t **out_value_label,
                                     lv_obj_t **out_bar)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(row, 4, LV_PART_MAIN);

    lv_obj_t *top = lv_obj_create(row);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_set_height(top, LV_SIZE_CONTENT);
    lv_obj_set_layout(top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_hex(0xD0E7FF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);

    *out_value_label = lv_label_create(top);
    lv_label_set_text(*out_value_label, "0%");
    lv_obj_set_style_text_color(*out_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(*out_value_label, &lv_font_montserrat_14, LV_PART_MAIN);

    *out_bar = lv_bar_create(row);
    lv_obj_set_width(*out_bar, LV_PCT(100));
    lv_obj_set_height(*out_bar, 10);
    lv_bar_set_range(*out_bar, 0, 100);
    lv_bar_set_value(*out_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(*out_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(*out_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1D3555), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, indicator_color, LV_PART_INDICATOR);

    return row;
}

void bmm350_view_create(void)
{
    static const char *compass_labels[] =
    {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW", "N", NULL
    };

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
    lv_label_set_text(title, "BMM350 Compass (I3C)");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(header);
    lv_label_set_text(subtitle, "Compass needle with heading and XYZ axis");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(content, 8, LV_PART_MAIN);

    lv_obj_t *compass_card = lv_obj_create(content);
    lv_obj_set_width(compass_card, 248);
    lv_obj_set_height(compass_card, LV_PCT(100));
    lv_obj_set_layout(compass_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(compass_card,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(compass_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(compass_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(compass_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compass_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(compass_card, 6, LV_PART_MAIN);

    lv_obj_t *compass_title = lv_label_create(compass_card);
    lv_label_set_text(compass_title, "Compass");
    lv_obj_set_style_text_color(compass_title, lv_color_hex(0x67E8F9), LV_PART_MAIN);
    lv_obj_set_style_text_font(compass_title, &lv_font_montserrat_20, LV_PART_MAIN);

    /* Compass center zone uses flex_grow=1 so the round scale stays centered
     * in both X/Y regardless of title/value label heights.
     */
    lv_obj_t *compass_center = lv_obj_create(compass_card);
    lv_obj_set_width(compass_center, LV_PCT(100));
    lv_obj_set_flex_grow(compass_center, 1);
    lv_obj_set_layout(compass_center, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(compass_center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(compass_center,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(compass_center, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(compass_center, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compass_center, 0, LV_PART_MAIN);

    s_view.heading_scale = lv_scale_create(compass_center);
    lv_obj_set_size(s_view.heading_scale, 172, 172);
    lv_scale_set_mode(s_view.heading_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(s_view.heading_scale, 0, 360);
    lv_scale_set_total_tick_count(s_view.heading_scale, 41);
    lv_scale_set_major_tick_every(s_view.heading_scale, 5);
    lv_scale_set_label_show(s_view.heading_scale, true);
    lv_scale_set_text_src(s_view.heading_scale, compass_labels);
    lv_scale_set_angle_range(s_view.heading_scale, 360);
    lv_scale_set_rotation(s_view.heading_scale, 270);
    lv_obj_set_style_bg_opa(s_view.heading_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_view.heading_scale, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_view.heading_scale, lv_color_hex(0x2C4364), LV_PART_MAIN);

    lv_obj_set_style_text_color(s_view.heading_scale, lv_color_hex(0xD0E7FF), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(s_view.heading_scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_length(s_view.heading_scale, 10, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(s_view.heading_scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(s_view.heading_scale, lv_color_hex(0x8DD4FF), LV_PART_INDICATOR);

    lv_obj_set_style_length(s_view.heading_scale, 5, LV_PART_ITEMS);
    lv_obj_set_style_line_width(s_view.heading_scale, 1, LV_PART_ITEMS);
    lv_obj_set_style_line_color(s_view.heading_scale, lv_color_hex(0x4A658A), LV_PART_ITEMS);

    s_view.heading_needle = lv_line_create(s_view.heading_scale);
    lv_obj_set_style_line_width(s_view.heading_needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(s_view.heading_needle, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_view.heading_needle, lv_color_hex(0xEF4444), LV_PART_MAIN);
    lv_scale_set_line_needle_value(s_view.heading_scale, s_view.heading_needle, 72, 0);

    lv_obj_t *needle_center = lv_obj_create(s_view.heading_scale);
    lv_obj_set_size(needle_center, 10, 10);
    lv_obj_set_style_radius(needle_center, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(needle_center, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(needle_center, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_center(needle_center);
    lv_obj_clear_flag(needle_center, LV_OBJ_FLAG_CLICKABLE);

    s_view.heading_value_label = lv_label_create(compass_card);
    lv_label_set_text(s_view.heading_value_label, "000 deg");
    lv_obj_set_style_text_color(s_view.heading_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.heading_value_label, &lv_font_montserrat_22, LV_PART_MAIN);

    s_view.heading_dir_label = lv_label_create(compass_card);
    lv_label_set_text(s_view.heading_dir_label, "N");
    lv_obj_set_style_text_color(s_view.heading_dir_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.heading_dir_label, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *axis_card = lv_obj_create(content);
    lv_obj_set_flex_grow(axis_card, 1);
    lv_obj_set_height(axis_card, LV_PCT(100));
    lv_obj_set_layout(axis_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(axis_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(axis_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(axis_card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(axis_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(axis_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(axis_card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(axis_card, 8, LV_PART_MAIN);

    lv_obj_t *axis_title = lv_label_create(axis_card);
    lv_label_set_text(axis_title, "Magnetic Axis (uT)");
    lv_obj_set_style_text_color(axis_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(axis_title, &lv_font_montserrat_20, LV_PART_MAIN);

    (void)create_axis_row(axis_card,
                          "X Axis",
                          lv_color_hex(0xF87171),
                          &s_view.x_value_label,
                          &s_view.x_bar);

    (void)create_axis_row(axis_card,
                          "Y Axis",
                          lv_color_hex(0x4ADE80),
                          &s_view.y_value_label,
                          &s_view.y_bar);

    (void)create_axis_row(axis_card,
                          "Z Axis",
                          lv_color_hex(0x60A5FA),
                          &s_view.z_value_label,
                          &s_view.z_bar);

    lv_obj_t *summary = lv_obj_create(axis_card);
    lv_obj_set_width(summary, LV_PCT(100));
    lv_obj_set_height(summary, LV_SIZE_CONTENT);
    lv_obj_set_layout(summary, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(summary,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(summary, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(summary, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(summary, 0, LV_PART_MAIN);

    s_view.field_value_label = lv_label_create(summary);
    lv_label_set_text(s_view.field_value_label, "Field: 0.0 uT");
    lv_obj_set_style_text_color(s_view.field_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.field_value_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_view.temp_value_label = lv_label_create(summary);
    lv_label_set_text(s_view.temp_value_label, "Temp: --.- C");
    lv_obj_set_style_text_color(s_view.temp_value_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.temp_value_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_set_width(footer, LV_PCT(100));
    lv_obj_set_height(footer, LV_SIZE_CONTENT);
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(footer, 10, LV_PART_MAIN);

    s_view.status_label = lv_label_create(footer);
    lv_label_set_text(s_view.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_view.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_flex_grow(s_view.status_label, 1);

    s_view.calib_label = lv_label_create(footer);
    lv_label_set_text(s_view.calib_label, "Cal: pending");
    lv_obj_set_style_text_color(s_view.calib_label, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.calib_label, &lv_font_montserrat_16, LV_PART_MAIN);

    s_view.calibrate_btn = lv_button_create(footer);
    lv_obj_set_height(s_view.calibrate_btn, 34);
    lv_obj_set_style_bg_color(s_view.calibrate_btn, lv_color_hex(0x1D4ED8), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_view.calibrate_btn, lv_color_hex(0x1E40AF), LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_view.calibrate_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_view.calibrate_btn, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_view.calibrate_btn, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(s_view.calibrate_btn, calibrate_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(s_view.calibrate_btn);
    lv_label_set_text(btn_label, "Calibrate");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(btn_label);

    s_view.calib_overlay = lv_obj_create(screen);
    lv_obj_set_size(s_view.calib_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_view.calib_overlay, lv_color_hex(0x020617), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_view.calib_overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_view.calib_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_view.calib_overlay, 0, LV_PART_MAIN);
    lv_obj_set_layout(s_view.calib_overlay, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_view.calib_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_view.calib_overlay,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(s_view.calib_overlay, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *overlay_card = lv_obj_create(s_view.calib_overlay);
    lv_obj_set_width(overlay_card, LV_PCT(80));
    lv_obj_set_height(overlay_card, LV_SIZE_CONTENT);
    lv_obj_set_layout(overlay_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(overlay_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(overlay_card, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_border_color(overlay_card, lv_color_hex(0x2563EB), LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay_card, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(overlay_card, 8, LV_PART_MAIN);

    lv_obj_t *overlay_title = lv_label_create(overlay_card);
    lv_label_set_text(overlay_title, "Compass Calibration");
    lv_obj_set_style_text_color(overlay_title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(overlay_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *overlay_desc = lv_label_create(overlay_card);
    lv_label_set_text(overlay_desc,
                      "Rotate board slowly in many directions\n"
                      "(figure-8 / full circle) until both progress bars reach 100%.");
    lv_obj_set_style_text_color(overlay_desc, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(overlay_desc, &lv_font_montserrat_14, LV_PART_MAIN);

    (void)create_progress_row(overlay_card,
                              "Sample Progress",
                              lv_color_hex(0x22C55E),
                              &s_view.calib_sample_label,
                              &s_view.calib_sample_bar);

    (void)create_progress_row(overlay_card,
                              "Coverage Progress",
                              lv_color_hex(0xF59E0B),
                              &s_view.calib_coverage_label,
                              &s_view.calib_coverage_bar);

    s_view.calib_hint_label = lv_label_create(overlay_card);
    lv_label_set_text(s_view.calib_hint_label, "Press Calibrate to start.");
    lv_obj_set_style_text_color(s_view.calib_hint_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.calib_hint_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_add_flag(s_view.calib_overlay, LV_OBJ_FLAG_HIDDEN);
}

void bmm350_view_set_calibrate_handler(bmm350_view_calibrate_cb_t cb, void *user_data)
{
    s_view.calibrate_cb = cb;
    s_view.calibrate_user_data = user_data;
}

void bmm350_view_set_init_failed(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: init failed");
}

void bmm350_view_set_ready(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: ready");
}

void bmm350_view_set_read_error(cy_rslt_t rslt)
{
    char line[64];

    lv_snprintf(line, sizeof(line), "Sensor: read error (0x%08lx)", (unsigned long)rslt);
    lv_label_set_text(s_view.status_label, line);
}

void bmm350_view_set_calibration_status(bool active,
                                        bool done,
                                        uint32_t sample_progress_pct,
                                        uint32_t coverage_progress_pct,
                                        uint32_t remaining_sec)
{
    char line[64];

    if (sample_progress_pct > 100U)
    {
        sample_progress_pct = 100U;
    }

    if (coverage_progress_pct > 100U)
    {
        coverage_progress_pct = 100U;
    }

    lv_bar_set_value(s_view.calib_sample_bar, (int32_t)sample_progress_pct, LV_ANIM_OFF);
    lv_bar_set_value(s_view.calib_coverage_bar, (int32_t)coverage_progress_pct, LV_ANIM_OFF);

    lv_snprintf(line, sizeof(line), "%lu%%", (unsigned long)sample_progress_pct);
    lv_label_set_text(s_view.calib_sample_label, line);

    lv_snprintf(line, sizeof(line), "%lu%%", (unsigned long)coverage_progress_pct);
    lv_label_set_text(s_view.calib_coverage_label, line);

    if (done)
    {
        lv_label_set_text(s_view.calib_label, "Cal: done");
        lv_label_set_text(s_view.calib_hint_label, "Calibration complete.");
        return;
    }

    if (active)
    {
        lv_label_set_text(s_view.calib_label, "Cal: running");
        lv_snprintf(line, sizeof(line), "Keep rotating. Remaining ~%lu s", (unsigned long)remaining_sec);
        lv_label_set_text(s_view.calib_hint_label, line);
        return;
    }

    lv_label_set_text(s_view.calib_label, "Cal: pending");
    lv_label_set_text(s_view.calib_hint_label, "Press Calibrate to start.");
}

void bmm350_view_set_calibration_overlay_visible(bool visible)
{
    if (visible)
    {
        lv_obj_clear_flag(s_view.calib_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_view.calib_overlay);
        return;
    }

    lv_obj_add_flag(s_view.calib_overlay, LV_OBJ_FLAG_HIDDEN);
}

void bmm350_view_update_sample(const bmm350_sample_t *sample)
{
    char line[64];

    if (NULL == sample)
    {
        return;
    }

    /* Round to nearest degree for stable text + needle position. */
    int32_t heading = (int32_t)(sample->heading_deg + 0.5f);
    if (heading >= 360)
    {
        heading = 0;
    }

    lv_scale_set_line_needle_value(s_view.heading_scale, s_view.heading_needle, 72, heading);

    lv_snprintf(line, sizeof(line), "%03ld deg", (long)heading);
    lv_label_set_text(s_view.heading_value_label, line);
    lv_label_set_text(s_view.heading_dir_label, heading_to_cardinal(sample->heading_deg));

    lv_snprintf(line, sizeof(line), "%.1f uT", (double)sample->x_ut);
    lv_label_set_text(s_view.x_value_label, line);

    lv_snprintf(line, sizeof(line), "%.1f uT", (double)sample->y_ut);
    lv_label_set_text(s_view.y_value_label, line);

    lv_snprintf(line, sizeof(line), "%.1f uT", (double)sample->z_ut);
    lv_label_set_text(s_view.z_value_label, line);

    int32_t x_val = clamp_i32((int32_t)sample->x_ut, -BMM350_AXIS_BAR_MAX_UT, BMM350_AXIS_BAR_MAX_UT);
    int32_t y_val = clamp_i32((int32_t)sample->y_ut, -BMM350_AXIS_BAR_MAX_UT, BMM350_AXIS_BAR_MAX_UT);
    int32_t z_val = clamp_i32((int32_t)sample->z_ut, -BMM350_AXIS_BAR_MAX_UT, BMM350_AXIS_BAR_MAX_UT);

    lv_bar_set_start_value(s_view.x_bar, 0, LV_ANIM_OFF);
    lv_bar_set_start_value(s_view.y_bar, 0, LV_ANIM_OFF);
    lv_bar_set_start_value(s_view.z_bar, 0, LV_ANIM_OFF);
    lv_bar_set_value(s_view.x_bar, x_val, LV_ANIM_OFF);
    lv_bar_set_value(s_view.y_bar, y_val, LV_ANIM_OFF);
    lv_bar_set_value(s_view.z_bar, z_val, LV_ANIM_OFF);

    lv_snprintf(line, sizeof(line), "Field: %.1f uT", (double)sample->field_strength_ut);
    lv_label_set_text(s_view.field_value_label, line);

    lv_snprintf(line, sizeof(line), "Temp: %.1f C", (double)sample->temperature_c);
    lv_label_set_text(s_view.temp_value_label, line);
}