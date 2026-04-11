#include "lvgl.h"
#include "ui_ep01_basic_label.h"
#include "app_logo.h"

void ui_ep01_basic_label_create(void)
{
    /* Use the currently active screen as the root container. */
    lv_obj_t *screen = lv_screen_active();

    /* Set a solid dark background for this episode. */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* Create logo in the upper section to reserve space for labels below. */
    lv_obj_t *logo = lv_image_create(screen);
    lv_image_set_src(logo, &APP_LOGO);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 24);

    /* Place main title under logo to avoid overlap when logo size changes. */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "EP01 - Basic Label");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_align_to(title, logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 48);

    /* Place subtitle below title with a smaller spacing. */
    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "HMI Menu & Setting");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 24);
}
