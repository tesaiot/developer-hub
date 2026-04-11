#include "dps368_view.h"

#include "lvgl.h"

#include "app_logo.h"

typedef struct
{
    lv_obj_t *status_label;
    lv_obj_t *pressure_label;
    lv_obj_t *temperature_label;
    lv_obj_t *sample_label;
} dps368_view_ctx_t;

static dps368_view_ctx_t s_view;

void dps368_view_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Screen layout and style are kept in this file only (UI-only layer). */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_image_set_scale(logo, 150);
    lv_obj_align(logo, LV_ALIGN_TOP_RIGHT, -20, 12);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "DPS368 Monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 24);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "SensorHub DPS368 (CM55)");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    s_view.status_label = lv_label_create(screen);
    lv_label_set_text(s_view.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_view.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.status_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(s_view.status_label, LV_ALIGN_TOP_LEFT, 20, 120);

    s_view.pressure_label = lv_label_create(screen);
    lv_label_set_text(s_view.pressure_label, "Pressure: --.-- hPa");
    lv_obj_set_style_text_color(s_view.pressure_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.pressure_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_view.pressure_label, LV_ALIGN_TOP_LEFT, 20, 176);

    s_view.temperature_label = lv_label_create(screen);
    lv_label_set_text(s_view.temperature_label, "Temperature: --.-- C");
    lv_obj_set_style_text_color(s_view.temperature_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.temperature_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_view.temperature_label, LV_ALIGN_TOP_LEFT, 20, 224);

    s_view.sample_label = lv_label_create(screen);
    lv_label_set_text(s_view.sample_label, "Samples: 0");
    lv_obj_set_style_text_color(s_view.sample_label, lv_color_hex(0x38BDF8), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_view.sample_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(s_view.sample_label, LV_ALIGN_TOP_LEFT, 20, 280);
}

void dps368_view_set_init_failed(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: init failed");
}

void dps368_view_set_ready(void)
{
    lv_label_set_text(s_view.status_label, "Sensor: ready");
}

void dps368_view_set_read_error(cy_rslt_t rslt)
{
    char line[48];

    lv_snprintf(line, sizeof(line), "Sensor: read error (0x%08lx)", (unsigned long)rslt);
    lv_label_set_text(s_view.status_label, line);
}

void dps368_view_update_sample(const dps368_sample_t *sample)
{
    char line[64];

    if (NULL == sample)
    {
        return;
    }

    lv_snprintf(line, sizeof(line), "Pressure: %.2f hPa", (double)sample->pressure_hpa);
    lv_label_set_text(s_view.pressure_label, line);

    lv_snprintf(line, sizeof(line), "Temperature: %.2f C", (double)sample->temperature_c);
    lv_label_set_text(s_view.temperature_label, line);

    lv_snprintf(line, sizeof(line), "Samples: %lu", (unsigned long)sample->sample_count);
    lv_label_set_text(s_view.sample_label, line);
}