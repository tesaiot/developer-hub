#include "lvgl.h"

#include <stdio.h>

#include "app_logo.h"
#include "app_dps368_service.h"
#include "xensiv_dps3xx.h"
#include "dps368_monitor_legacy.h"

typedef struct
{
    lv_obj_t *status_label;
    lv_obj_t *pressure_label;
    lv_obj_t *temperature_label;
    lv_obj_t *sample_label;
    uint32_t sample_count;
} ui_ep01_ctx_t;

static ui_ep01_ctx_t s_ctx;

static void ui_ep01_update_sensor_cb(lv_timer_t *timer)
{
    (void)timer;

    static bool last_read_ok = false;
    static cy_rslt_t last_error = CY_RSLT_SUCCESS;

    float pressure_hpa = 0.0f;
    float temperature_c = 0.0f;

    cy_rslt_t rslt = app_dps368_service_read(&pressure_hpa, &temperature_c);
    if (CY_RSLT_SUCCESS == rslt)
    {
        char line[64];

        lv_snprintf(line, sizeof(line), "Pressure: %.2f hPa", (double)pressure_hpa);
        lv_label_set_text(s_ctx.pressure_label, line);

        lv_snprintf(line, sizeof(line), "Temperature: %.2f C", (double)temperature_c);
        lv_label_set_text(s_ctx.temperature_label, line);

        s_ctx.sample_count++;
        lv_snprintf(line, sizeof(line), "Samples: %lu", (unsigned long)s_ctx.sample_count);
        lv_label_set_text(s_ctx.sample_label, line);

        lv_label_set_text(s_ctx.status_label, "Sensor: ready");

        if ((!last_read_ok) || ((s_ctx.sample_count % 10U) == 0U))
        {
            printf("[EP01][DPS368] SAMPLE pressure=%.2f hPa temp=%.2f C\r\n",
                   (double)pressure_hpa,
                   (double)temperature_c);
        }

        last_read_ok = true;
        last_error = CY_RSLT_SUCCESS;
        return;
    }

    if (rslt == XENSIV_DPS3XX_RSLT_ERR_DATA_NOT_READY)
    {
        /* Ignore "not ready" between 1 Hz conversions to keep UI/log clean. */
        return;
    }

    lv_label_set_text(s_ctx.status_label, "Sensor: read error");

    if ((last_read_ok) || (last_error != rslt))
    {
        printf("[EP01][DPS368] READ_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
    }

    last_read_ok = false;
    last_error = rslt;
}

void ui_ep01_dps368_monitor_create(mtb_hal_i2c_t *i2c_bus)
{
    lv_obj_t *screen = lv_screen_active();

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
    lv_label_set_text(subtitle, "SensorHub EP01 (CM55)");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x93C5FD), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    s_ctx.status_label = lv_label_create(screen);
    lv_label_set_text(s_ctx.status_label, "Sensor: initializing...");
    lv_obj_set_style_text_color(s_ctx.status_label, lv_color_hex(0xFDE68A), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.status_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 20, 120);

    s_ctx.pressure_label = lv_label_create(screen);
    lv_label_set_text(s_ctx.pressure_label, "Pressure: --.-- hPa");
    lv_obj_set_style_text_color(s_ctx.pressure_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.pressure_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_ctx.pressure_label, LV_ALIGN_TOP_LEFT, 20, 176);

    s_ctx.temperature_label = lv_label_create(screen);
    lv_label_set_text(s_ctx.temperature_label, "Temperature: --.-- C");
    lv_obj_set_style_text_color(s_ctx.temperature_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.temperature_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_ctx.temperature_label, LV_ALIGN_TOP_LEFT, 20, 224);

    s_ctx.sample_label = lv_label_create(screen);
    lv_label_set_text(s_ctx.sample_label, "Samples: 0");
    lv_obj_set_style_text_color(s_ctx.sample_label, lv_color_hex(0x38BDF8), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_ctx.sample_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(s_ctx.sample_label, LV_ALIGN_TOP_LEFT, 20, 280);

    s_ctx.sample_count = 0U;

    cy_rslt_t init_rslt = app_dps368_service_init(i2c_bus);
    if (CY_RSLT_SUCCESS != init_rslt)
    {
        lv_label_set_text(s_ctx.status_label, "Sensor: init failed");
        return;
    }

    (void)lv_timer_create(ui_ep01_update_sensor_cb, 1000U, NULL);
}


