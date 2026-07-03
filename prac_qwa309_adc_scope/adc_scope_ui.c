/*******************************************************************************
* File Name        : adc_scope_ui.c
*
* Description      : 4-channel "oscilloscope" for the QWA309 potentiometers.
*                    Samples the four SAR pots (P15.4-P15.7) and plots each as a
*                    scrolling LVGL line series (0-100 %). Reuses the HW-verified
*                    pot_monitor AUTANALOG setup; a pure analog-input teaching
*                    example that reads real 12-bit conversions.
*******************************************************************************/
#include "adc_scope_ui.h"

#include "cy_autanalog.h"
#include "cy_autanalog_sar.h"
#include "cy_gpio.h"
#include "cycfg_peripherals.h"
#include "cybsp.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

#define SCOPE_ADC_INDEX   (0U)
#define SCOPE_FULL_SCALE  (4095U)
#define SCOPE_POINTS      (100U)
#define SCOPE_PERIOD_MS   (60U)
#define SCOPE_CH          (4U)

static const uint8_t  s_ch[SCOPE_CH]     = { 0U, 1U, 2U, 3U };            /* result idx */
static const char    *s_name[SCOPE_CH]   = { "VR1", "VR2", "VR3", "VR4" };
static const uint32_t s_accent[SCOPE_CH] = { 0x14B8A6, 0x22C55E, 0xF59E0B, 0xF43F5E };

static lv_obj_t      *s_chart;
static lv_chart_series_t *s_series[SCOPE_CH];
static lv_obj_t      *s_val[SCOPE_CH];
static bool           s_adc_ok;

static void scope_pin(GPIO_PRT_Type *port, uint32_t pin)
{
    Cy_GPIO_Pin_FastInit(port, pin, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
}

static bool scope_adc_init(void)
{
    scope_pin(P15_4_PORT, P15_4_PIN);
    scope_pin(P15_5_PORT, P15_5_PIN);
    scope_pin(P15_6_PORT, P15_6_PIN);
    scope_pin(P15_7_PORT, P15_7_PIN);
    if (CY_AUTANALOG_SUCCESS != Cy_AutAnalog_Init(&autonomous_analog_init)) {
        return false;
    }
    Cy_AutAnalog_Enable();
    Cy_AutAnalog_StartAutonomousControl();
    return true;
}

static void scope_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_adc_ok) { return; }
    for (uint8_t i = 0U; i < SCOPE_CH; i++) {
        uint16_t raw = (uint16_t)Cy_AutAnalog_SAR_ReadResult(SCOPE_ADC_INDEX,
                          CY_AUTANALOG_SAR_INPUT_GPIO, s_ch[i]) & 0x0FFFU;
        uint32_t pct = ((uint32_t)raw * 100U) / SCOPE_FULL_SCALE;
        lv_chart_set_next_value(s_chart, s_series[i], (int32_t)pct);
        lv_label_set_text_fmt(s_val[i], "%s %lu%%", s_name[i], (unsigned long)pct);
    }
}

void adc_scope_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    s_adc_ok = scope_adc_init();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 14, 0);
    lv_obj_set_style_pad_gap(screen, 8, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, s_adc_ok ? "4-Channel ADC Scope" : "ADC init failed");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(s_adc_ok ? 0xF8FAFC : 0xFCA5A5), 0);

    /* value row */
    lv_obj_t *vrow = lv_obj_create(screen);
    lv_obj_remove_style_all(vrow);
    lv_obj_set_width(vrow, lv_pct(100));
    lv_obj_set_flex_flow(vrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(vrow, 16, 0);
    for (uint8_t i = 0U; i < SCOPE_CH; i++) {
        s_val[i] = lv_label_create(vrow);
        lv_label_set_text(s_val[i], s_name[i]);
        lv_obj_set_style_text_font(s_val[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(s_val[i], lv_color_hex(s_accent[i]), 0);
    }

    s_chart = lv_chart_create(screen);
    lv_obj_set_width(s_chart, lv_pct(100));
    lv_obj_set_flex_grow(s_chart, 1);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, SCOPE_POINTS);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_color(s_chart, lv_color_hex(0x263241), 0);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);   /* no point dots */
    for (uint8_t i = 0U; i < SCOPE_CH; i++) {
        s_series[i] = lv_chart_add_series(s_chart, lv_color_hex(s_accent[i]),
                                          LV_CHART_AXIS_PRIMARY_Y);
    }

    if (s_adc_ok) {
        lv_timer_t *t = lv_timer_create(scope_timer_cb, SCOPE_PERIOD_MS, NULL);
        lv_timer_ready(t);
    }
}

/* [] END OF FILE */
