#include "sht4x_view.h"

#include "lvgl.h"

#include "app_logo.h"
#include "sht4x/sht4x_config.h"

typedef struct
{
    lv_obj_t *status_label;

    lv_obj_t *temp_value_label;
    lv_obj_t *temp_bar;

    lv_obj_t *hum_value_label;
    lv_obj_t *hum_bar;

    lv_obj_t *comfort_value_label;
    lv_obj_t *comfort_chip;
} sht4x_view_ctx_t;

static sht4x_view_ctx_t s_view;

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

/* Map temperature to level color for bar indicator. */
static lv_color_t temp_level_color(float temp_c)
{
    if (temp_c < 20.0f)
    {
        return lv_color_hex(0x0EA5E9);
    }

    if (temp_c <= 32.0f)
    {
        return lv_color_hex(0x22C55E);
    }

    return lv_color_hex(0xEF4444);
}

/* Map humidity to level color and comfort-zone chip. */
static lv_color_t hum_level_color(float humidity_rh)
{
    if (humidity_rh < 40.0f)
    {
        return lv_color_hex(0xD97706);
    }

    if (humidity_rh <= 60.0f)
    {
        return lv_color_hex(0x16A34A);
    }

    return lv_color_hex(0x2563EB);
}

static void set_comfort_chip(float humidity_rh)
{
    lv_color_t chip_color = hum_level_color(humidity_rh);

    if (humidity_rh < 40.0f)
    {
        lv_label_set_text(s_view.comfort_value_label, "Dry");
    }
    else if (humidity_rh <= 60.0f)
    {
        lv_label_set_text(s_view.comfort_value_label, "Comfort");
    }
    else
    {
        lv_label_set_text(s_view.comfort_value_label, "Humid");
    }

    lv_obj_set_style_bg_color(s_view.comfort_chip, chip_color, LV_PART_MAIN);
}

/* Reusable flex card with title, big value label, and one LVGL bar. */
static lv_obj_t *create_metric_card(lv_obj_t *parent,
                                    const char *title,
                                    lv_color_t title_color,
                                    lv_obj_t **out_value_label,
                                    lv_obj_t **out_bar,
                                    int32_t bar_min,
                                    int32_t bar_max)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_PCT(100));
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(card, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x122034), LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, title_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, LV_PART_MAIN);

    *out_value_label = lv_label_create(card);
    lv_label_set_text(*out_value_label, "--.-");
    lv_obj_set_style_text_color(*out_value_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(*out_value_label, &lv_font_montserrat_30, LV_PART_MAIN);

    *out_bar = lv_bar_create(card);
    lv_obj_set_width(*out_bar, LV_PCT(100));
    lv_obj_set_height(*out_bar, 22);
    lv_bar_set_range(*out_bar, bar_min, bar_max);
    lv_bar_set_value(*out_bar, bar_min, LV_ANIM_OFF);
    lv_obj_set_style_radius(*out_bar, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(*out_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x1E3A5F), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_bar, lv_color_hex(0x60A5FA), LV_PART_INDICATOR);

    return card;
}

void sht4x_view_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Root page uses flex column: header -> metrics -> comfort -> footer. */

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(screen, 8, LV_PART_MAIN);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Header row: title/subtitle (logo is floating on screen). */
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);

    lv_obj_t *header_text = lv_obj_create(header);
    lv_obj_set_flex_grow(header_text, 1);
    lv_obj_set_width(header_text, LV_PCT(80));
    lv_obj_set_height(header_text, LV_SIZE_CONTENT);
    lv_obj_set_layout(header_text, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_text, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_text, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(header_text, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_text, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_text, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(header_text, 2, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(header_text);
    lv_label_set_text(title, "SHT4x Humidity Indicator");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(header_text);
    lv_label_set_text(subtitle, "Relative humidity + temperature with indicator bars");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, LV_PART_MAIN);

    /* Keep logo out of header layout so it does not stretch header height. */
    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, 40);
    lv_obj_add_flag(logo, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(logo, LV_ALIGN_TOP_RIGHT, -8, 2);
    lv_obj_clear_flag(logo, LV_OBJ_FLAG_CLICKABLE);

    /* Top content row with two equal cards. */
    lv_obj_t *metrics_row = lv_obj_create(screen);
    lv_obj_set_width(metrics_row, LV_PCT(100));
    lv_obj_set_height(metrics_row, 162);
    lv_obj_set_layout(metrics_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(metrics_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(metrics_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(metrics_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(metrics_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(metrics_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(metrics_row, 8, LV_PART_MAIN);

    (void)create_metric_card(metrics_row,
                             "Temperature",
                             lv_color_hex(0xFCA5A5),
                             &s_view.temp_value_label,
                             &s_view.temp_bar,
                             0,
                             1000);

    (void)create_metric_card(metrics_row,
                             "Humidity",
                             lv_color_hex(0x67E8F9),
                             &s_view.hum_value_label,
                             &s_view.hum_bar,
                             0,
                             1000);

    /* Comfort card fills the next area and avoids overlap by flex column. */
    lv_obj_t *comfort_card = lv_obj_create(screen);
    lv_obj_set_width(comfort_card, LV_PCT(100));
    lv_obj_set_height(comfort_card, 156);
    lv_obj_set_layout(comfort_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(comfort_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(comfort_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(comfort_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(comfort_card, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(comfort_card, lv_color_hex(0x101B2D), LV_PART_MAIN);
    lv_obj_set_style_border_color(comfort_card, lv_color_hex(0x274766), LV_PART_MAIN);
    lv_obj_set_style_radius(comfort_card, 10, LV_PART_MAIN);

    lv_obj_t *comfort_title = lv_label_create(comfort_card);
    lv_label_set_text(comfort_title, "Humidity Comfort Zone");
    lv_obj_set_style_text_color(comfort_title, lv_color_hex(0xBFDBFE), LV_PART_MAIN);
    lv_obj_set_style_text_font(comfort_title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *range_hint = lv_label_create(comfort_card);
    lv_label_set_text(range_hint, "Dry < 40%RH   |   Comfort 40-60%RH   |   Humid > 60%RH");
    lv_label_set_long_mode(range_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(range_hint, LV_PCT(100));
    lv_obj_set_style_text_color(range_hint, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_text_font(range_hint, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *chip_row = lv_obj_create(comfort_card);
    lv_obj_set_width(chip_row, LV_PCT(100));
    lv_obj_set_flex_grow(chip_row, 1);
    lv_obj_set_layout(chip_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chip_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(chip_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(chip_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chip_row, 0, LV_PART_MAIN);

    s_view.comfort_chip = lv_obj_create(chip_row);
    lv_obj_set_size(s_view.comfort_chip, 220, 56);
    lv_obj_set_style_border_width(s_view.comfort_chip, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_view.comfort_chip, 10, LV_PART_MAIN);

    s_view.comfort_value_label = lv_label_create(s_view.comfort_chip);
    lv_obj_set_style_text_color(s_view.comfort_value_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.comfort_value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(s_view.comfort_value_label);

    /* Footer holds runtime status text (ready/error). */
    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_set_width(footer, LV_PCT(100));
    lv_obj_set_height(footer, LV_SIZE_CONTENT);
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(footer, 0, LV_PART_MAIN);

    s_view.status_label = lv_label_create(footer);
    lv_label_set_text(s_view.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_view.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.status_label, &lv_font_montserrat_16, LV_PART_MAIN);

    set_comfort_chip(50.0f);
}

void sht4x_view_set_init_failed(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: init failed");
}

void sht4x_view_set_ready(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: ready");
}

void sht4x_view_set_read_error(cy_rslt_t rslt)
{
    char line[64];

    lv_snprintf(line, sizeof(line), "Sensor: read error (0x%08lx)", (unsigned long)rslt);
    lv_label_set_text(s_view.status_label, line);
}

void sht4x_view_update_sample(const sht4x_sample_t *sample)
{
    char line[64];
    int32_t temp_scaled;
    int32_t hum_scaled;
    lv_color_t temp_color;
    lv_color_t hum_color;

    if (NULL == sample)
    {
        return;
    }

    lv_snprintf(line, sizeof(line), "%.1f C", (double)sample->temperature_c);
    lv_label_set_text(s_view.temp_value_label, line);

    lv_snprintf(line, sizeof(line), "%.1f %%RH", (double)sample->humidity_rh);
    lv_label_set_text(s_view.hum_value_label, line);

    temp_scaled = clamp_i32((int32_t)(sample->temperature_c * 10.0f), 0, 1000);
    hum_scaled = clamp_i32((int32_t)(sample->humidity_rh * 10.0f), 0, 1000);

    temp_color = temp_level_color(sample->temperature_c);
    hum_color = hum_level_color(sample->humidity_rh);

#if SHT4X_BAR_USE_ANIM
    lv_bar_set_value(s_view.temp_bar, temp_scaled, LV_ANIM_ON);
    lv_bar_set_value(s_view.hum_bar, hum_scaled, LV_ANIM_ON);
#else
    lv_bar_set_value(s_view.temp_bar, temp_scaled, LV_ANIM_OFF);
    lv_bar_set_value(s_view.hum_bar, hum_scaled, LV_ANIM_OFF);
#endif

    /* Update indicator colors by current levels. */
    lv_obj_set_style_bg_color(s_view.temp_bar, temp_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_view.hum_bar, hum_color, LV_PART_INDICATOR);
    set_comfort_chip(sample->humidity_rh);
}

