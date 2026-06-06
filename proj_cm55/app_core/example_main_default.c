/*******************************************************************************
 * @file    example_main_default.c
 * @brief   Default weak-symbol stub for the episode entry point.
 *
 *  This file is linked whenever proj_cm55/apps/ contains no episode that
 *  provides a strong definition of example_main(). It displays a splash
 *  screen instructing the student to download and install an episode.
 *
 *  DO NOT EDIT — this is part of the master template.
 *  To run an episode, copy its files into proj_cm55/apps/ and rebuild.
 *  The student's episode file will override this weak symbol at link time.
 ******************************************************************************/

#include "app_interface.h"
#include "lvgl.h"
#include <stdio.h>

/* Color palette (kept in sync with the course common theme) */
#define UI_COLOR_BG        lv_color_hex(0x0A1628)
#define UI_COLOR_CARD      lv_color_hex(0x142240)
#define UI_COLOR_PRIMARY   lv_color_hex(0x00BCD4)
#define UI_COLOR_TEXT      lv_color_hex(0xE0E0E0)
#define UI_COLOR_TEXT_DIM  lv_color_hex(0x808080)
#define UI_COLOR_ACCENT    lv_color_hex(0xFF9800)

__attribute__((weak)) void example_main(lv_obj_t *parent)
{
    printf("[MASTER][STUB] No episode in proj_cm55/apps/ — showing default splash.\r\n");
    printf("[MASTER][STUB] Download an episode from the TESAIoT Developer Hub:\r\n");
    printf("[MASTER][STUB]   https://dev.tesaiot.dev\r\n");
    printf("[MASTER][STUB] Then copy its files into proj_cm55/apps/ and rebuild.\r\n");

    /* Full-screen dark background */
    lv_obj_set_style_bg_color(parent, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Card container in the center */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, lv_pct(80), lv_pct(70));
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 24, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title — use fonts known-enabled in lv_conf.h (24/20/16/14 are all :=1) */
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "TESAIoT Dev Kit");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_bottom(title, 8, 0);

    /* Subtitle */
    lv_obj_t *subtitle = lv_label_create(card);
    lv_label_set_text(subtitle, "Master Template Ready");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(subtitle, UI_COLOR_TEXT, 0);
    lv_obj_set_style_pad_bottom(subtitle, 24, 0);

    /* Instructions */
    lv_obj_t *inst = lv_label_create(card);
    lv_label_set_text(inst, "No episode is currently installed.\n\n"
                            "Step 1  Visit " LV_SYMBOL_HOME " dev.tesaiot.dev\n"
                            "Step 2  Download an episode .zip\n"
                            "Step 3  Extract into proj_cm55/apps/\n"
                            "Step 4  make program");
    lv_obj_set_style_text_font(inst, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(inst, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_line_space(inst, 6, 0);
    lv_label_set_long_mode(inst, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(inst, lv_pct(90));

    /* Footer hint */
    lv_obj_t *footer = lv_label_create(card);
    lv_label_set_text(footer, LV_SYMBOL_WARNING " This is the default stub — replace by dropping an episode in apps/");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(footer, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_pad_top(footer, 24, 0);
    lv_label_set_long_mode(footer, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(footer, lv_pct(90));
}
