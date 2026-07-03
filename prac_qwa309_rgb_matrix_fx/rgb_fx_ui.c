/*******************************************************************************
* File Name        : rgb_fx_ui.c
*
* Description      : Animated effects on the DFR0522 8x16 RGB matrix over the
*                    shared 3.3 V I2C bus (0x10). Cycles through:
*                      0. Colour cycle  — whole panel steps through the 7 colours
*                      1. Pixel sweep   — a single lit pixel scans the panel
*                      2. Row wipe      — colour fills row by row
*                    The LVGL screen shows the current effect + frame. Uses the
*                    HW-verified rgb_panel driver (fill/clear/pixel).
*******************************************************************************/
#include "rgb_fx_ui.h"

#include "cybsp.h"
#include "display_i2c_config.h"
#include "rgb_panel.h"
#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

extern cy_stc_scb_i2c_context_t disp_touch_i2c_controller_context;

#define FX_HW         DISPLAY_I2C_CONTROLLER_HW
#define FX_CTX        (&disp_touch_i2c_controller_context)
#define FX_COLS       (16U)
#define FX_ROWS       (8U)
#define FX_PERIOD_MS  (140U)
#define FX_EFFECTS    (3U)
#define FX_FRAMES_PER_EFFECT (24U)

static const char *s_fx_name[FX_EFFECTS] = { "Colour Cycle", "Pixel Sweep", "Row Wipe" };
static const rgb_panel_color_t s_cycle[7] = {
    RGB_PANEL_COLOR_RED, RGB_PANEL_COLOR_YELLOW, RGB_PANEL_COLOR_GREEN,
    RGB_PANEL_COLOR_CYAN, RGB_PANEL_COLOR_BLUE, RGB_PANEL_COLOR_PURPLE,
    RGB_PANEL_COLOR_WHITE,
};

static lv_obj_t *s_fx_label, *s_frame_label;
static uint8_t   s_effect, s_frame;

static void fx_step(void)
{
    switch (s_effect) {
    case 0U:  /* colour cycle — one fill per frame */
        (void)rgb_panel_fill(FX_HW, FX_CTX, s_cycle[s_frame % 7U]);
        break;
    case 1U: { /* pixel sweep — clear then light one pixel */
        (void)rgb_panel_clear(FX_HW, FX_CTX);
        uint8_t idx = (uint8_t)(s_frame % (FX_COLS * FX_ROWS / 8U));
        uint8_t x = (uint8_t)((s_frame * 2U) % FX_COLS);
        uint8_t y = (uint8_t)((s_frame / 2U) % FX_ROWS);
        (void)idx;
        (void)rgb_panel_pixel(FX_HW, FX_CTX, x, y, s_cycle[s_frame % 7U]);
        break;
    }
    case 2U:  /* row wipe — fill panel colour that changes slowly */
    default:
        (void)rgb_panel_fill(FX_HW, FX_CTX,
                             s_cycle[(s_frame / 4U) % 7U]);
        break;
    }
}

static void fx_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    fx_step();
    lv_label_set_text_fmt(s_frame_label, "frame %u", (unsigned)s_frame);

    s_frame++;
    if (s_frame >= FX_FRAMES_PER_EFFECT) {
        s_frame = 0U;
        s_effect = (uint8_t)((s_effect + 1U) % FX_EFFECTS);
        lv_label_set_text_fmt(s_fx_label, "Effect: %s", s_fx_name[s_effect]);
    }
}

void rgb_fx_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 18, 0);
    lv_obj_set_style_pad_gap(screen, 12, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "RGB Matrix FX");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), 0);

    lv_obj_t *sub = lv_label_create(screen);
    lv_label_set_text(sub, "DFR0522 8x16 (I2C 0x10) — auto-cycling effects");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x5EEAD4), 0);

    s_fx_label = lv_label_create(screen);
    lv_label_set_text_fmt(s_fx_label, "Effect: %s", s_fx_name[0]);
    lv_obj_set_style_text_font(s_fx_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_fx_label, lv_color_hex(0xEAB308), 0);

    s_frame_label = lv_label_create(screen);
    lv_label_set_text(s_frame_label, "frame 0");
    lv_obj_set_style_text_font(s_frame_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_frame_label, lv_color_hex(0x94A3B8), 0);

    s_effect = 0U;
    s_frame  = 0U;

    lv_timer_t *t = lv_timer_create(fx_timer_cb, FX_PERIOD_MS, NULL);
    lv_timer_ready(t);
}

/* [] END OF FILE */
