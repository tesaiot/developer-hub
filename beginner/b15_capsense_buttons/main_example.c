/**
 * @file    main_example.c
 * @brief   CapSense Buttons - Touch buttons and slider via IPC sensorhub
 *
 * Reads CapSense touch buttons and slider position every 50ms
 * using ipc_sensorhub_snapshot(). Eva Kit only (BSP_HAS_CAPSENSE).
 */

#include "pse84_common.h"

#if BSP_HAS_CAPSENSE

#define NUM_CS_BTNS 2

typedef struct {
    lv_obj_t *btn_leds[NUM_CS_BTNS];
    lv_obj_t *btn_labels[NUM_CS_BTNS];
    lv_obj_t *slider_widget;
    lv_obj_t *lbl_slider_val;
} capsense_ctx_t;

static capsense_ctx_t ctx;

static void capsense_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_capsense) return;

    /* Button 0 */
    bool btn0 = snap.capsense.btn0_pressed;
    if (btn0) {
        lv_led_on(ctx.btn_leds[0]);
        lv_obj_set_style_text_color(ctx.btn_labels[0], UI_COLOR_SUCCESS, 0);
    } else {
        lv_led_off(ctx.btn_leds[0]);
        lv_obj_set_style_text_color(ctx.btn_labels[0], UI_COLOR_TEXT_DIM, 0);
    }

    /* Button 1 */
    bool btn1 = snap.capsense.btn1_pressed;
    if (btn1) {
        lv_led_on(ctx.btn_leds[1]);
        lv_obj_set_style_text_color(ctx.btn_labels[1], UI_COLOR_SUCCESS, 0);
    } else {
        lv_led_off(ctx.btn_leds[1]);
        lv_obj_set_style_text_color(ctx.btn_labels[1], UI_COLOR_TEXT_DIM, 0);
    }

    /* Slider (0-100%) */
    uint8_t slider_pos = snap.capsense.slider;
    lv_slider_set_value(ctx.slider_widget, (int32_t)slider_pos, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx.lbl_slider_val, "Slider: %u%%", slider_pos);
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent,
        "CapSense Touch (Eva Kit)", &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    /* ปุ่มสัมผัส CapSense */
    lv_obj_t *th_sub = example_label_create(parent,
        "ปุ่มสัมผัส CapSense",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Button row */
    lv_obj_t *btn_row = lv_obj_create(parent);
    lv_obj_set_size(btn_row, 400, 120);
    lv_obj_align(btn_row, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);

    const char *btn_names[] = { "BTN 0", "BTN 1" };
    for (int i = 0; i < NUM_CS_BTNS; i++) {
        lv_obj_t *col = lv_obj_create(btn_row);
        lv_obj_set_size(col, 130, 100);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);

        ctx.btn_leds[i] = lv_led_create(col);
        lv_obj_set_size(ctx.btn_leds[i], 50, 50);
        lv_led_set_color(ctx.btn_leds[i], lv_palette_main(LV_PALETTE_CYAN));
        lv_led_off(ctx.btn_leds[i]);

        ctx.btn_labels[i] = lv_label_create(col);
        lv_label_set_text(ctx.btn_labels[i], btn_names[i]);
        lv_obj_set_style_text_font(ctx.btn_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(ctx.btn_labels[i], UI_COLOR_TEXT_DIM, 0);
    }

    /* Slider display */
    ctx.slider_widget = lv_slider_create(parent);
    lv_obj_set_width(ctx.slider_widget, 400);
    lv_slider_set_range(ctx.slider_widget, 0, 100);
    lv_slider_set_value(ctx.slider_widget, 0, LV_ANIM_OFF);
    lv_obj_remove_flag(ctx.slider_widget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(ctx.slider_widget, LV_ALIGN_CENTER, 0, 50);

    ctx.lbl_slider_val = example_label_create(parent, "Slider: 0%",
                                               &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(ctx.lbl_slider_val, LV_ALIGN_CENTER, 0, 80);

    /* Timer: 50ms */
    lv_timer_create(capsense_timer_cb, 50, NULL);
}

#else

void example_main(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "CapSense not available on this board.\nThis example requires Eva Kit.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
