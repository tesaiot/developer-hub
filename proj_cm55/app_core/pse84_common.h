/*******************************************************************************
 * @file    pse84_common.h
 * @brief   Practise-collection compatibility shim for the TESAIoT Dev Kit host.
 *
 *  The practise_examples/ codes were authored against the tesaiot_bento_kit_master
 *  host, which exposes UI colour macros and small LVGL helpers here. This shim
 *  provides the same surface on the tesaiot_dev_kit_master framework so those
 *  practise codes drop into proj_cm55/apps/ and build unchanged.
 *
 *  Difference from the bento original: thai_label() routes text through the
 *  thai_shaping layer (thai_label_set_text) so Thai diacritic clusters stack
 *  correctly — see tesaiot_thai.h / thai_lvgl_adapter.h.
 ******************************************************************************/
#ifndef PSE84_COMMON_H
#define PSE84_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "tesaiot_thai.h"          /* fonts + tesaiot_thai_label + thai_label_set_text */

/* ── Display geometry (4.3" panel) ───────────────────────────────────── */
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

/* ── Board capability flags (TESAIoT Dev Kit = AI Kit SoM + QWA309 base) ─ */
#define BSP_HAS_BMI270        1
#define BSP_HAS_BMM350        1
#define BSP_HAS_DPS368        1
#define BSP_HAS_SHT40         1
#define BSP_HAS_CAPSENSE      1   /* PSoC 4000T companion (I2C 0x08)        */
#define BSP_HAS_POTENTIOMETER 1   /* 4x SAR pots on P15.4-P15.7            */

/* ── UI colour palette ───────────────────────────────────────────────── */
#define UI_COLOR_PRIMARY    lv_color_hex(0x00D4FF)
#define UI_COLOR_SUCCESS    lv_color_hex(0x4ADE80)
#define UI_COLOR_WARNING    lv_color_hex(0xFBBF24)
#define UI_COLOR_ERROR      lv_color_hex(0xEF4444)
#define UI_COLOR_INFO       lv_color_hex(0x3B82F6)
#define UI_COLOR_TEXT       lv_color_hex(0xFFFFFF)
#define UI_COLOR_TEXT_DIM   lv_color_hex(0x94A3B8)
#define UI_COLOR_BG_DARK    lv_color_hex(0x16213E)
#define UI_COLOR_BG_CARD    lv_color_hex(0x1F4068)
#define UI_COLOR_BMI270     lv_color_hex(0x00D4FF)
#define UI_COLOR_DPS368     lv_color_hex(0xFBBF24)
#define UI_COLOR_SHT40      lv_color_hex(0x4ADE80)
#define UI_COLOR_BMM350     lv_color_hex(0xA78BFA)
#define UI_COLOR_CARD_BG    lv_color_hex(0x1F4068)

#define COLOR_PRIMARY       lv_color_hex(0x00D4FF)
#define COLOR_BG_DARK       lv_color_hex(0x16213E)
#define COLOR_BG_CARD       lv_color_hex(0x1F4068)
#define COLOR_SUCCESS       lv_color_hex(0x4ADE80)
#define COLOR_WARNING       lv_color_hex(0xFBBF24)
#define COLOR_ERROR         lv_color_hex(0xEF4444)
#define COLOR_INFO          lv_color_hex(0x3B82F6)
#define COLOR_TEXT          lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_DIM      lv_color_hex(0x94A3B8)
#define COLOR_ACCENT_CYAN   lv_color_hex(0x00D4FF)
#define COLOR_ACCENT_GREEN  lv_color_hex(0x4ADE80)
#define COLOR_ACCENT_ORANGE lv_color_hex(0xFBBF24)
#define COLOR_ACCENT_RED    lv_color_hex(0xEF4444)
#define COLOR_ACCENT_BLUE   lv_color_hex(0x3B82F6)

#define PHI_MAJOR  0.618f
#define PHI_MINOR  0.382f
#define UI_CARD_RADIUS      12
#define UI_CARD_BORDER      1
#define UI_CARD_SHADOW      8
#define UI_CARD_PAD         12

static inline const lv_font_t *_thai_font_by_size(int sz)
{
    if (sz >= 28) return &lv_font_noto_thai_28;
    if (sz >= 24) return &lv_font_noto_thai_24;
    if (sz >= 20) return &lv_font_noto_thai_20;
    if (sz >= 16) return &lv_font_noto_thai_16;
    return &lv_font_noto_thai_14;
}

/* Styled card container (fixed 4-arg). */
static inline lv_obj_t *example_card_create(lv_obj_t *parent, int w, int h, lv_color_t bg_color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, UI_CARD_BORDER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_shadow_width(card, UI_CARD_SHADOW, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(card, UI_CARD_PAD, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* Styled label (fixed 4-arg). */
static inline lv_obj_t *example_label_create(lv_obj_t *parent, const char *text,
                                             const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

/* Thai label — auto-selects Noto Thai by size AND runs cluster shaping so
 * stacked vowels/tone marks render correctly (dev-kit host difference). */
static inline lv_obj_t *thai_label(lv_obj_t *parent, const char *text,
                                   int font_size, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, _thai_font_by_size(font_size), 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    thai_label_set_text(lbl, text);
    return lbl;
}

#endif /* PSE84_COMMON_H */
