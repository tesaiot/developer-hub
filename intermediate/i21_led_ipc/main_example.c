/**
 * @file    main_example.c
 * @brief   LED Control via IPC - CM55 button → IPC_CMD_GPIO_LED → CM33 toggles LED
 *
 * CM55 sends 0x80 command with data[0]=1 (ON) or data[0]=0 (OFF).
 * On-screen LED widget mirrors the physical LED state.
 */

#include "pse84_common.h"
#include "ipc_communication.h"

#define IPC_CMD_GPIO_LED   (0x80)

static bool      s_led_on;
static lv_obj_t *s_led_widget;
static lv_obj_t *s_lbl_state;
/* CRITICAL: Must be in shared SRAM for cross-core IPC */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;

/* ── Send LED command to CM33 ────────────────────────────────────── */
static void send_led_cmd(bool on)
{
    s_ipc_tx.cmd     = IPC_CMD_GPIO_LED;
    s_ipc_tx.data[0] = on ? 0x01 : 0x00;

    Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                            CM55_IPC_PIPE_EP_ADDR,
                            (uint32_t *)&s_ipc_tx, NULL);
}

/* ── Button toggle event ─────────────────────────────────────────── */
static void btn_toggle_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    s_led_on = !s_led_on;
    send_led_cmd(s_led_on);

    /* Update on-screen LED mirror */
    if (s_led_on) {
        lv_led_on(s_led_widget);
        lv_obj_set_style_bg_color(s_led_widget,
                                  lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(s_lbl_state, "LED: ON");
        lv_obj_set_style_text_color(s_lbl_state,
                                    lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        lv_led_off(s_led_widget);
        lv_obj_set_style_bg_color(s_led_widget,
                                  lv_palette_main(LV_PALETTE_GREY), 0);
        lv_label_set_text(s_lbl_state, "LED: OFF");
        lv_obj_set_style_text_color(s_lbl_state,
                                    lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_led_on = false;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I19 - LED Control via IPC");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* ควบคุม LED ผ่าน IPC */
    lv_obj_t *th_sub = example_label_create(parent,
        "ควบคุม LED ผ่าน IPC",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Description */
    lv_obj_t *desc = lv_label_create(parent);
    lv_label_set_text(desc, "Tap button to send IPC_CMD_GPIO_LED (0x80) to CM33");
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 32);

    /* LED indicator */
    s_led_widget = lv_led_create(parent);
    lv_obj_set_size(s_led_widget, 80, 80);
    lv_obj_align(s_led_widget, LV_ALIGN_CENTER, 0, -30);
    lv_led_off(s_led_widget);

    /* State label */
    s_lbl_state = lv_label_create(parent);
    lv_label_set_text(s_lbl_state, "LED: OFF");
    lv_obj_set_style_text_font(s_lbl_state, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_state, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_state, LV_ALIGN_CENTER, 0, 30);

    /* Toggle button */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_event_cb(btn, btn_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_TEAL), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Toggle LED");
    lv_obj_center(btn_lbl);
}
