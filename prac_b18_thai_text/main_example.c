/**
 * @file    main_example.c
 * @brief   Thai Text Display - Noto Sans Thai font rendering demo
 *
 * Shows all 4 Thai font sizes (14, 16, 20, 28) with complex Thai text.
 * Uses thai_label() helper for clean, readable Thai strings.
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 20, 0);
    lv_obj_set_style_pad_row(parent, 10, 0);

    /* ── Size 28: Title ──────────────────────────────────────────── */
    example_label_create(parent, "28px:", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    thai_label(parent, "สวัสดีครับ TESAIoT : : Make Anything.", 28, UI_COLOR_PRIMARY);

    /* ── Divider ─────────────────────────────────────────────────── */
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_set_size(div, 760, 2);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);

    /* ── Size 20: Stacked vowels + Sara Am ───────────────────────── */
    example_label_create(parent, "20px:", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    thai_label(parent,
        "ประเทศไทยมีภูมิอากาศร้อนชื้น  ความสำคัญของความรู้ทำให้เราก้าวไกล",
        20, UI_COLOR_SUCCESS);

    /* ── Size 16: Technical + sensor data ────────────────────────── */
    example_label_create(parent, "16px:", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    thai_label(parent,
        "ระบบสมองกลฝังตัว PSoC Edge E84 รันบน Cortex-M55",
        16, UI_COLOR_WARNING);
    thai_label(parent,
        "อุณหภูมิ 28.5°C  ความชื้น 65%  ความดัน 1013.2 hPa",
        16, lv_color_hex(0xFFD740));

    /* ── Size 14: Rare clusters + brand names ────────────────────── */
    example_label_create(parent, "14px:", &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    thai_label(parent,
        "ทรัพย์สินทางปัญญา  อินฟินิออน  เซมิคอนดักเตอร์",
        14, UI_COLOR_BMM350);
    thai_label(parent,
        "เรียนรู้แล้วโตใหญ่ไม่ได้  สระหน้า: เ แ โ ใ ไ  สระอำ: ทำ คำ สำ",
        14, UI_COLOR_INFO);
}
