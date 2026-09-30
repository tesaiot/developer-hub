/**
 * @file    main_example.c
 * @brief   Button-LED Link - Physical button press toggles real LED
 *
 * Polls CYBSP_USER_BTN1 (SW1) at 100ms. Each press toggles
 * CYBSP_USER_LED1 on/off via Cy_GPIO_Inv(). LVGL indicators
 * reflect real hardware state.
 *
 * Hardware:
 *   SW1  (CYBSP_USER_BTN1)  - P7.0  (active LOW)
 *   LED1 (CYBSP_USER_LED1)  - P10.7 (active LOW)
 *
 * @board  AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 */

#include "pse84_common.h"

typedef struct {
    lv_obj_t *led_widget;
    lv_obj_t *lbl_state;
    bool      prev_pressed;
} link_ctx_t;

static link_ctx_t ctx;

/* Poll button and toggle LED on press edge */
static void poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    bool pressed = (Cy_GPIO_Read(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN) == 0);

    if (pressed && !ctx.prev_pressed) {
        /* Toggle real LED on button press edge */
        Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
    }
    ctx.prev_pressed = pressed;

    /* Read actual LED state (active LOW: 0 = ON) */
    bool led_on = (Cy_GPIO_Read(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN) == 0);

    if (led_on) {
        lv_led_on(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, LV_SYMBOL_OK " LED ON (SW1 toggles)");
        lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_SUCCESS, 0);
    } else {
        lv_led_off(ctx.led_widget);
        lv_label_set_text(ctx.lbl_state, "LED OFF - press SW1");
        lv_obj_set_style_text_color(ctx.lbl_state, UI_COLOR_TEXT_DIM, 0);
    }
}

void example_main(lv_obj_t *parent)
{
    ctx.prev_pressed = false;

    /* Ensure LED starts OFF */
    Cy_GPIO_Set(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Button-LED Link",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* ปุ่มกดเชื่อมต่อ LED */
    thai_label(parent, "ปุ่มกดเชื่อมต่อ LED (ฮาร์ดแวร์)", 14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Instruction */
    lv_obj_t *info = example_label_create(parent,
        "Press SW1 on the board to toggle LED1",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 40);

    /* Virtual LED */
    ctx.led_widget = lv_led_create(parent);
    lv_obj_set_size(ctx.led_widget, 100, 100);
    lv_led_set_color(ctx.led_widget, lv_palette_main(LV_PALETTE_YELLOW));
    lv_led_off(ctx.led_widget);
    lv_obj_align(ctx.led_widget, LV_ALIGN_CENTER, 0, -30);

    /* State label */
    ctx.lbl_state = example_label_create(parent, "LED OFF - press SW1",
                                          &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_state, LV_ALIGN_CENTER, 0, 60);

    /* Start polling timer (100ms) */
    lv_timer_create(poll_timer_cb, 100, NULL);
}
