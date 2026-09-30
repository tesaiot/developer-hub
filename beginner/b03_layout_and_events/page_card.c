/**
 * @file    page_card.c
 * @brief   Card Layout — 2 styled cards with title, subtitle, description
 *
 * Demonstrates the example_card_create() helper with shadow, border, radius.
 */

#include "pages.h"

void page_card_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Two styled card containers side by side.\n"
                                "Uses example_card_create() helper.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Card 1: Sensor Info */
    lv_obj_t *card1 = example_card_create(parent, 200, 220, UI_COLOR_CARD_BG);
    lv_obj_align(card1, LV_ALIGN_TOP_LEFT, 5, 60);

    lv_obj_t *title1 = lv_label_create(card1);
    lv_label_set_text(title1, LV_SYMBOL_SETTINGS " Sensors");
    lv_obj_set_style_text_font(title1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title1, UI_COLOR_PRIMARY, 0);
    lv_obj_align(title1, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sub1 = lv_label_create(card1);
    lv_label_set_text(sub1, "BMI270 + DPS368");
    lv_obj_set_style_text_font(sub1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub1, UI_COLOR_WARNING, 0);
    lv_obj_align(sub1, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *desc1 = lv_label_create(card1);
    lv_label_set_text(desc1, "6-axis IMU with\nbarometric pressure\nsensor for altitude\nmeasurement.");
    lv_obj_set_style_text_font(desc1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc1, UI_COLOR_TEXT, 0);
    lv_obj_set_width(desc1, 180);
    lv_label_set_long_mode(desc1, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc1, LV_ALIGN_TOP_LEFT, 0, 55);

    /* Card 2: WiFi Info */
    lv_obj_t *card2 = example_card_create(parent, 200, 220, UI_COLOR_CARD_BG);
    lv_obj_align(card2, LV_ALIGN_TOP_RIGHT, -5, 60);

    lv_obj_t *title2 = lv_label_create(card2);
    lv_label_set_text(title2, LV_SYMBOL_WIFI " WiFi");
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title2, UI_COLOR_INFO, 0);
    lv_obj_align(title2, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sub2 = lv_label_create(card2);
    lv_label_set_text(sub2, "CYW55513 SoC");
    lv_obj_set_style_text_font(sub2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub2, UI_COLOR_SUCCESS, 0);
    lv_obj_align(sub2, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *desc2 = lv_label_create(card2);
    lv_label_set_text(desc2, "Dual-band WiFi 6\nwith SoftAP mode.\nSecure connections\nvia WPA3.");
    lv_obj_set_style_text_font(desc2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc2, UI_COLOR_TEXT, 0);
    lv_obj_set_width(desc2, 180);
    lv_label_set_long_mode(desc2, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc2, LV_ALIGN_TOP_LEFT, 0, 55);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "example_card_create(parent, 200, 220,\n"
                                "                    UI_COLOR_CARD_BG);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 300);
}
