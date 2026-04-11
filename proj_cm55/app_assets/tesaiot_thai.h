/*******************************************************************************
 * @file    tesaiot_thai.h
 * @brief   Thai language rendering helpers for TESAIoT Dev Kit episodes
 *
 *  The master template bundles Noto Sans Thai fonts (4 sizes: 14, 16, 20, 28)
 *  ใน `app_assets/` ทำให้ทุก episode สามารถแสดงข้อความภาษาไทยบน LVGL ได้ทันที
 *  โดยไม่ต้องเพิ่ม font file หรือแก้ lv_conf.h เพิ่มเติม
 *
 *  ไฟล์นี้เปิดเผย 2 ส่วน:
 *    1) extern const lv_font_t ของ 4 font sizes (จาก lv_fonts_thai.h)
 *    2) helper functions ให้ episode ใช้ได้สะดวก
 *
 *  Usage in episode:
 *  ```c
 *  #include "tesaiot_thai.h"
 *
 *  void example_main(lv_obj_t *parent) {
 *      tesaiot_add_thai_support_badge();   // persistent top-layer badge
 *      my_episode_create();                // ← existing episode entry
 *  }
 *  ```
 ******************************************************************************/
#ifndef TESAIOT_THAI_H
#define TESAIOT_THAI_H

#include "lvgl.h"
#include "lv_fonts_thai.h"   /* extern const lv_font_t lv_font_noto_thai_{14,16,20,28} */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a label that renders Thai text using Noto Sans Thai font.
 * Supports Thai + English + ASCII symbols in a single string.
 *
 * @param parent  LVGL parent object
 * @param text    UTF-8 encoded string (may contain Thai characters)
 * @param size    Font size: 14, 16, 20, or 28 px (others fall back to 14)
 * @param color   Text color
 * @return        Pointer to the created label object
 */
static inline lv_obj_t *tesaiot_thai_label(lv_obj_t *parent, const char *text,
                                           int size, lv_color_t color)
{
    const lv_font_t *font;
    switch (size) {
        case 28: font = &lv_font_noto_thai_28; break;
        case 20: font = &lv_font_noto_thai_20; break;
        case 16: font = &lv_font_noto_thai_16; break;
        default: font = &lv_font_noto_thai_14; break;
    }
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

/**
 * Add a persistent "รองรับภาษาไทย" badge to the LVGL top layer.
 *
 *  The badge is drawn above every active-screen widget so it remains visible
 *  regardless of what the episode renders on lv_scr_act(). Serves as a visible
 *  marker confirming that Thai font rendering is available in every TESAIoT
 *  Dev Kit episode — even those whose main UI is English-only.
 *
 *  Call this once from the episode's example_main() — typically as the first
 *  statement, before delegating to the episode's create function.
 */
static inline void tesaiot_add_thai_support_badge(void)
{
    lv_obj_t *badge = lv_label_create(lv_layer_top());
    lv_label_set_text(badge, "รองรับภาษาไทย");
    lv_obj_set_style_text_font(badge, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0xE0F7FA), 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_80, 0);
    lv_obj_set_style_pad_top(badge, 4, 0);
    lv_obj_set_style_pad_bottom(badge, 4, 0);
    lv_obj_set_style_pad_left(badge, 10, 0);
    lv_obj_set_style_pad_right(badge, 10, 0);
    lv_obj_set_style_radius(badge, 6, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, -8, 8);
}

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_THAI_H */
