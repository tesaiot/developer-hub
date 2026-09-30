/**
 * @file    page_custom_style.c
 * @brief   Custom Style — 3 buttons with different lv_style_t configurations
 *
 * Demonstrates lv_style_t, lv_style_init, lv_obj_add_style with
 * radius, shadow, gradient, border, and font properties.
 */

#include "pages.h"

/* Styles must be static to persist after function returns */
static lv_style_t s_style_rounded;
static lv_style_t s_style_shadow;
static lv_style_t s_style_gradient;

void page_custom_style_create(lv_obj_t *parent)
{
    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Same button, 3 different styles.\n"
                                "Each uses a separate lv_style_t.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Style 1: Large radius (pill shape), bold border */
    lv_style_init(&s_style_rounded);
    lv_style_set_radius(&s_style_rounded, 25);
    lv_style_set_bg_color(&s_style_rounded, lv_color_hex(0x00BCD4));
    lv_style_set_bg_opa(&s_style_rounded, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_rounded, 3);
    lv_style_set_border_color(&s_style_rounded, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&s_style_rounded, &lv_font_montserrat_16);
    lv_style_set_text_color(&s_style_rounded, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&s_style_rounded, 12);

    /* Style 2: Deep shadow, dark background */
    lv_style_init(&s_style_shadow);
    lv_style_set_radius(&s_style_shadow, 10);
    lv_style_set_bg_color(&s_style_shadow, lv_color_hex(0x263B5A));
    lv_style_set_bg_opa(&s_style_shadow, LV_OPA_COVER);
    lv_style_set_shadow_width(&s_style_shadow, 20);
    lv_style_set_shadow_color(&s_style_shadow, lv_color_hex(0x00BCD4));
    lv_style_set_shadow_opa(&s_style_shadow, LV_OPA_60);
    lv_style_set_border_width(&s_style_shadow, 0);
    lv_style_set_text_font(&s_style_shadow, &lv_font_montserrat_16);
    lv_style_set_text_color(&s_style_shadow, UI_COLOR_PRIMARY);
    lv_style_set_pad_all(&s_style_shadow, 12);

    /* Style 3: Vertical gradient, no border */
    lv_style_init(&s_style_gradient);
    lv_style_set_radius(&s_style_gradient, 12);
    lv_style_set_bg_color(&s_style_gradient, lv_color_hex(0x4CAF50));
    lv_style_set_bg_grad_color(&s_style_gradient, lv_color_hex(0x1B5E20));
    lv_style_set_bg_grad_dir(&s_style_gradient, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&s_style_gradient, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_gradient, 0);
    lv_style_set_shadow_width(&s_style_gradient, 8);
    lv_style_set_shadow_color(&s_style_gradient, lv_color_hex(0x4CAF50));
    lv_style_set_shadow_opa(&s_style_gradient, LV_OPA_40);
    lv_style_set_text_font(&s_style_gradient, &lv_font_montserrat_16);
    lv_style_set_text_color(&s_style_gradient, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&s_style_gradient, 12);

    /* Button 1: Rounded */
    lv_obj_t *btn1 = lv_btn_create(parent);
    lv_obj_set_size(btn1, 130, 50);
    lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, 5, 60);
    lv_obj_remove_style_all(btn1);
    lv_obj_add_style(btn1, &s_style_rounded, 0);

    lv_obj_t *lbl1 = lv_label_create(btn1);
    lv_label_set_text(lbl1, "Rounded");
    lv_obj_center(lbl1);

    /* Label for Style 1 */
    lv_obj_t *desc1 = lv_label_create(parent);
    lv_label_set_text(desc1, "radius=25\nborder=3px");
    lv_obj_set_style_text_font(desc1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc1, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(desc1, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Button 2: Shadow glow */
    lv_obj_t *btn2 = lv_btn_create(parent);
    lv_obj_set_size(btn2, 130, 50);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_remove_style_all(btn2);
    lv_obj_add_style(btn2, &s_style_shadow, 0);

    lv_obj_t *lbl2 = lv_label_create(btn2);
    lv_label_set_text(lbl2, "Shadow");
    lv_obj_center(lbl2);

    lv_obj_t *desc2 = lv_label_create(parent);
    lv_label_set_text(desc2, "shadow=20px\nglow=cyan");
    lv_obj_set_style_text_font(desc2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc2, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(desc2, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Button 3: Gradient */
    lv_obj_t *btn3 = lv_btn_create(parent);
    lv_obj_set_size(btn3, 130, 50);
    lv_obj_align(btn3, LV_ALIGN_TOP_RIGHT, -5, 60);
    lv_obj_remove_style_all(btn3);
    lv_obj_add_style(btn3, &s_style_gradient, 0);

    lv_obj_t *lbl3 = lv_label_create(btn3);
    lv_label_set_text(lbl3, "Gradient");
    lv_obj_center(lbl3);

    lv_obj_t *desc3 = lv_label_create(parent);
    lv_label_set_text(desc3, "grad=vertical\ngreen tones");
    lv_obj_set_style_text_font(desc3, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc3, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(desc3, btn3, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "static lv_style_t style;\n"
                                "lv_style_init(&style);\n"
                                "lv_style_set_radius(&style, 25);\n"
                                "lv_obj_add_style(btn, &style, 0);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 190);
}
