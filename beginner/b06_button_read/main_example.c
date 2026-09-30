/**
 * @file    main_example.c
 * @brief   Button Read - Poll real hardware button SW1 with press counter
 *
 * Uses Cy_GPIO_Read() on a 100ms LVGL timer to poll the physical
 * button state. Displays PRESSED/Released with a press counter.
 *
 * Hardware:
 *   SW1 (CYBSP_USER_BTN1) - P7.0 (active LOW, internal pull-up)
 *   Note: AI Kit has 1 button (SW1). Eva Kit has SW1 + SW2.
 *
 * @board  AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 */

#include "pse84_common.h"

typedef struct {
    lv_obj_t *lbl_sw1;
    lv_obj_t *lbl_count1;
    int32_t   count_sw1;
    bool      prev_pressed;
} btn_ctx_t;

static btn_ctx_t ctx;

/* Poll real button every 100ms */
static void btn_poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    bool pressed = (Cy_GPIO_Read(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN) == 0);

    if (pressed) {
        lv_label_set_text(ctx.lbl_sw1, "PRESSED");
        lv_obj_set_style_text_color(ctx.lbl_sw1, UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(ctx.lbl_sw1, "Released");
        lv_obj_set_style_text_color(ctx.lbl_sw1, UI_COLOR_TEXT_DIM, 0);
    }

    /* Count rising edge (release → press transition) */
    if (pressed && !ctx.prev_pressed) {
        ctx.count_sw1++;
        lv_label_set_text_fmt(ctx.lbl_count1, "Presses: %"PRId32, ctx.count_sw1);
    }
    ctx.prev_pressed = pressed;
}

void example_main(lv_obj_t *parent)
{
    ctx.count_sw1 = 0;
    ctx.prev_pressed = false;

    /* Title */
    lv_obj_t *title = example_label_create(parent, "Hardware Button Input",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* อ่านค่าปุ่มกด */
    thai_label(parent, "อ่านค่าปุ่มกด (ฮาร์ดแวร์จริง)", 14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *info = example_label_create(parent,
        "Press the physical SW1 button on the board",
        &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 50);

    /* SW1 display */
    lv_obj_t *card = example_card_create(parent, 300, 180, UI_COLOR_CARD_BG);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *sw1_title = example_label_create(card, "SW1 (P7.0)",
                                                &lv_font_montserrat_16, UI_COLOR_PRIMARY);
    lv_obj_align(sw1_title, LV_ALIGN_TOP_MID, 0, 4);

    ctx.lbl_sw1 = example_label_create(card, "Released",
                                        &lv_font_montserrat_24, UI_COLOR_TEXT_DIM);
    lv_obj_align(ctx.lbl_sw1, LV_ALIGN_CENTER, 0, -10);

    ctx.lbl_count1 = example_label_create(card, "Presses: 0",
                                           &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_count1, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Start polling timer (100ms) */
    lv_timer_create(btn_poll_timer_cb, 100, NULL);
}
