/**
 * @file main_example.c
 * @brief B04 - LED Toggle: Three buttons toggle Red, Green, Blue LEDs.
 *
 * Demonstrates real GPIO control from CM55 using Cy_GPIO_Inv() to
 * toggle physical LEDs, with LVGL LED widgets as visual indicators.
 *
 * Hardware:
 *   LED1 (CYBSP_USER_LED1)       - P10.7 (active LOW)
 *   RGB Red (CYBSP_LED_RGB_RED)  - P20.6 (active LOW)
 *   RGB Blue (CYBSP_LED_RGB_BLUE)- P20.5 (active LOW)
 *
 * @board  AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 */

#include "pse84_common.h"

/* LED pin compatibility: AI Kit uses CYBSP_LED_RGB_*, Eva Kit uses CYBSP_LED_* */
#ifndef CYBSP_LED_RGB_RED_PORT
#define CYBSP_LED_RGB_RED_PORT    CYBSP_LED_RED_PORT
#define CYBSP_LED_RGB_RED_PIN     CYBSP_LED_RED_PIN
#endif
#ifndef CYBSP_LED_RGB_GREEN_PORT
#define CYBSP_LED_RGB_GREEN_PORT  CYBSP_LED_GREEN_PORT
#define CYBSP_LED_RGB_GREEN_PIN   CYBSP_LED_GREEN_PIN
#endif
#ifndef CYBSP_LED_RGB_BLUE_PORT
#define CYBSP_LED_RGB_BLUE_PORT   CYBSP_LED_BLUE_PORT
#define CYBSP_LED_RGB_BLUE_PIN    CYBSP_LED_BLUE_PIN
#endif

/* Map 3 on-screen LEDs to real GPIO pins */
static GPIO_PRT_Type *const s_led_ports[3] = {
    CYBSP_LED_RGB_RED_PORT,
    CYBSP_USER_LED1_PORT,
    CYBSP_LED_RGB_BLUE_PORT,
};
static const uint32_t s_led_pins[3] = {
    CYBSP_LED_RGB_RED_PIN,
    CYBSP_USER_LED1_PIN,
    CYBSP_LED_RGB_BLUE_PIN,
};

static lv_obj_t *s_leds[3];
static bool s_led_on[3] = {false, false, false};

static const char *led_names[] = {"Red", "Green", "Blue"};

static void led_btn_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_led_on[idx] = !s_led_on[idx];

    /* Toggle real hardware LED */
    Cy_GPIO_Inv(s_led_ports[idx], s_led_pins[idx]);

    /* Update LVGL indicator to match */
    if (s_led_on[idx]) {
        lv_led_on(s_leds[idx]);
    } else {
        lv_led_off(s_leds[idx]);
    }
}

void example_main(lv_obj_t *parent)
{
    lv_color_t colors[3] = {
        lv_color_hex(0xFF0000),
        lv_color_hex(0x00FF00),
        lv_color_hex(0x0000FF),
    };

    /* Ensure LEDs start OFF (driven HIGH = off for active-LOW) */
    for (int i = 0; i < 3; i++) {
        Cy_GPIO_Set(s_led_ports[i], s_led_pins[i]);
    }

    /* Title */
    lv_obj_t *title = example_label_create(parent, "LED Toggle",
                                            &lv_font_montserrat_24, UI_COLOR_TEXT);
    /* สลับไฟ LED */
    thai_label(parent, "สลับไฟ LED", 14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *sub = example_label_create(parent, "Tap a button to toggle the LED",
                                          &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 55);

    /* Create 3 LED + button pairs */
    int start_x = -200;
    for (int i = 0; i < 3; i++) {
        int cx = start_x + i * 200;

        /* LED indicator */
        s_leds[i] = lv_led_create(parent);
        lv_obj_set_size(s_leds[i], 60, 60);
        lv_obj_align(s_leds[i], LV_ALIGN_CENTER, cx, -40);
        lv_led_set_color(s_leds[i], colors[i]);
        lv_led_set_brightness(s_leds[i], 80);
        lv_led_off(s_leds[i]);

        /* Button */
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 140, 60);
        lv_obj_align(btn, LV_ALIGN_CENTER, cx, 50);
        lv_obj_set_style_bg_color(btn, colors[i], 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_color(btn, colors[i], 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_50, 0);
        lv_obj_add_event_cb(btn, led_btn_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, led_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }
}
