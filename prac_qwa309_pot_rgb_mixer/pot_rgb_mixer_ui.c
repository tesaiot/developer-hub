/*******************************************************************************
* File Name        : pot_rgb_mixer_ui.c
*
* Description      : Analog -> actuator demo. Three potentiometers act as R/G/B
*                    channels; each above 50% turns its colour bit on. The 3
*                    bits select one of the DFR0522's 8 colours, filled onto the
*                    matrix over the shared 3.3 V I2C bus (0x10). The LVGL screen
*                    mirrors the mix with live bars + the resulting colour name.
*
*                    Combines two HW-verified QWA309 peripherals:
*                      - SAR pots  (P15.4-P15.7, AUTANALOG, from pot_monitor)
*                      - DFR0522   (I2C 0x10, from dfr0522_rgb_matrix)
*******************************************************************************/
#include "pot_rgb_mixer_ui.h"

#include "cy_autanalog.h"
#include "cy_autanalog_sar.h"
#include "cy_gpio.h"
#include "cycfg_peripherals.h"
#include "cybsp.h"
#include "display_i2c_config.h"
#include "rgb_panel.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

/* Framework-owned display/touch I2C context (also carries the DFR0522). */
extern cy_stc_scb_i2c_context_t disp_touch_i2c_controller_context;

#define MIX_ADC_INDEX      (0U)
#define MIX_FULL_SCALE     (4095U)
#define MIX_THRESHOLD      (2048U)   /* ~50 % */
#define MIX_REFRESH_MS     (120U)

/* Result indices 0,1,2 correspond to SAR GPIO channels 0-2 (pots on the
 * P15.4-P15.6 pins per the pot_monitor mapping). Channel 3 is shown but unused
 * for the colour mix. */
typedef struct { const char *name; uint8_t ch; uint32_t accent; lv_obj_t *bar; lv_obj_t *val; } mix_ch_t;
static mix_ch_t s_ch[3] = {
    { "R", 0U, 0xEF4444, NULL, NULL },
    { "G", 1U, 0x22C55E, NULL, NULL },
    { "B", 2U, 0x3B82F6, NULL, NULL },
};

static lv_obj_t *s_color_label;
static lv_obj_t *s_swatch;
static bool      s_adc_ok;
static rgb_panel_color_t s_last_color = RGB_PANEL_COLOR_WHITE; /* force first write */

static const char *color_names[8] = {
    "OFF", "RED", "GREEN", "YELLOW", "BLUE", "PURPLE", "CYAN", "WHITE"
};
static const uint32_t color_hex[8] = {
    0x1F2937, 0xEF4444, 0x22C55E, 0xEAB308, 0x3B82F6, 0xA855F7, 0x06B6D4, 0xF8FAFC
};

static void mix_pin(GPIO_PRT_Type *port, uint32_t pin)
{
    Cy_GPIO_Pin_FastInit(port, pin, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
}

static bool mix_adc_init(void)
{
    mix_pin(P15_4_PORT, P15_4_PIN);
    mix_pin(P15_5_PORT, P15_5_PIN);
    mix_pin(P15_6_PORT, P15_6_PIN);
    mix_pin(P15_7_PORT, P15_7_PIN);
    if (CY_AUTANALOG_SUCCESS != Cy_AutAnalog_Init(&autonomous_analog_init)) {
        return false;
    }
    Cy_AutAnalog_Enable();
    Cy_AutAnalog_StartAutonomousControl();
    return true;
}

static uint16_t mix_read(uint8_t ch)
{
    return (uint16_t)Cy_AutAnalog_SAR_ReadResult(MIX_ADC_INDEX,
             CY_AUTANALOG_SAR_INPUT_GPIO, ch) & 0x0FFFU;
}

static void mix_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_adc_ok) { return; }

    uint8_t bits = 0U;
    for (uint8_t i = 0U; i < 3U; i++) {
        uint16_t raw = mix_read(s_ch[i].ch);
        uint32_t pct = ((uint32_t)raw * 1000U) / MIX_FULL_SCALE;
        lv_bar_set_value(s_ch[i].bar, (int32_t)pct, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_ch[i].val, "%s %lu.%lu%%", s_ch[i].name,
                              (unsigned long)(pct / 10U), (unsigned long)(pct % 10U));
        if (raw >= MIX_THRESHOLD) { bits |= (uint8_t)(1U << i); }
    }

    /* bits: b0=R b1=G b2=B  ->  DFR0522 colour enum is the same RGB packing. */
    rgb_panel_color_t color = (rgb_panel_color_t)bits;
    lv_label_set_text_fmt(s_color_label, "Colour: %s", color_names[bits]);
    lv_obj_set_style_bg_color(s_swatch, lv_color_hex(color_hex[bits]), 0);

    if (color != s_last_color) {
        s_last_color = color;
        (void)rgb_panel_fill(DISPLAY_I2C_CONTROLLER_HW,
                             &disp_touch_i2c_controller_context, color);
    }
}

static void style_label(lv_obj_t *l, uint32_t c, const lv_font_t *f)
{
    lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
    lv_obj_set_style_text_font(l, f, 0);
}

void pot_rgb_mixer_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    s_adc_ok = mix_adc_init();
    s_last_color = RGB_PANEL_COLOR_WHITE;

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 16, 0);
    lv_obj_set_style_pad_gap(screen, 12, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Pot -> RGB Matrix Mixer");
    style_label(title, 0xF8FAFC, &lv_font_montserrat_24);
    lv_obj_t *sub = lv_label_create(screen);
    lv_label_set_text(sub, s_adc_ok ? "3 pots (R/G/B) -> DFR0522 (I2C 0x10)"
                                    : "ADC init failed");
    style_label(sub, s_adc_ok ? 0x5EEAD4 : 0xFCA5A5, &lv_font_montserrat_16);

    for (uint8_t i = 0U; i < 3U; i++) {
        lv_obj_t *rowc = lv_obj_create(screen);
        lv_obj_remove_style_all(rowc);
        lv_obj_set_width(rowc, lv_pct(100));
        lv_obj_set_flex_flow(rowc, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_gap(rowc, 4, 0);
        s_ch[i].val = lv_label_create(rowc);
        lv_label_set_text(s_ch[i].val, s_ch[i].name);
        style_label(s_ch[i].val, s_ch[i].accent, &lv_font_montserrat_16);
        s_ch[i].bar = lv_bar_create(rowc);
        lv_obj_set_width(s_ch[i].bar, lv_pct(100));
        lv_obj_set_height(s_ch[i].bar, 16);
        lv_bar_set_range(s_ch[i].bar, 0, 1000);
        lv_obj_set_style_bg_color(s_ch[i].bar, lv_color_hex(0x1F2937), LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_ch[i].bar, lv_color_hex(s_ch[i].accent), LV_PART_INDICATOR);
    }

    lv_obj_t *bottom = lv_obj_create(screen);
    lv_obj_remove_style_all(bottom);
    lv_obj_set_width(bottom, lv_pct(100));
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(bottom, 14, 0);
    s_swatch = lv_obj_create(bottom);
    lv_obj_set_size(s_swatch, 60, 60);
    lv_obj_set_style_radius(s_swatch, 8, 0);
    lv_obj_set_style_border_width(s_swatch, 2, 0);
    lv_obj_set_style_border_color(s_swatch, lv_color_hex(0x334155), 0);
    s_color_label = lv_label_create(bottom);
    lv_label_set_text(s_color_label, "Colour: OFF");
    style_label(s_color_label, 0xF8FAFC, &lv_font_montserrat_20);

    if (s_adc_ok) {
        lv_timer_t *t = lv_timer_create(mix_timer_cb, MIX_REFRESH_MS, NULL);
        lv_timer_ready(t);
    }
}

/* [] END OF FILE */
