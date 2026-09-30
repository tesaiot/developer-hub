/**
 * @file    main_example.c
 * @brief   Sensor Push via IPC - CM33 → CM55 sensor data transfer
 *
 * CM55 side: RegisterCallback receives packed sensor data from CM33,
 * stores in volatile buffer.  LVGL timer updates display from buffer.
 */

#include "pse84_common.h"
#include "ipc_communication.h"

#define IPC_CMD_SENSOR_DATA   (0xA0)

/* ── Shared volatile buffer (ISR → task) ─────────────────────────── */
static volatile struct {
    int16_t  accel_x;
    int16_t  accel_y;
    int16_t  accel_z;
    uint32_t update_count;
    bool     fresh;
} s_rx_buf;

static lv_obj_t *s_lbl_x, *s_lbl_y, *s_lbl_z;
static lv_obj_t *s_lbl_updates;

/* ── IPC callback - runs in ISR context ──────────────────────────── */
static void sensor_data_cb(uint32_t *msg_ptr)
{
    ipc_msg_t *msg = (ipc_msg_t *)msg_ptr;

    if (msg->cmd != IPC_CMD_SENSOR_DATA) return;

    /* Unpack little-endian 16-bit signed values from data[] */
    s_rx_buf.accel_x = (int16_t)(msg->data[0] | (msg->data[1] << 8));
    s_rx_buf.accel_y = (int16_t)(msg->data[2] | (msg->data[3] << 8));
    s_rx_buf.accel_z = (int16_t)(msg->data[4] | (msg->data[5] << 8));
    s_rx_buf.update_count++;
    s_rx_buf.fresh = true;
}

/* ── LVGL timer - display update at 10 Hz ────────────────────────── */
static void display_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!s_rx_buf.fresh) return;
    s_rx_buf.fresh = false;

    lv_label_set_text_fmt(s_lbl_x, "Accel X: %d mg", (int)s_rx_buf.accel_x);
    lv_label_set_text_fmt(s_lbl_y, "Accel Y: %d mg", (int)s_rx_buf.accel_y);
    lv_label_set_text_fmt(s_lbl_z, "Accel Z: %d mg", (int)s_rx_buf.accel_z);
    lv_label_set_text_fmt(s_lbl_updates, "Updates received: %u",
                          (unsigned)s_rx_buf.update_count);
}

/* ── Entry point (CM55 side) ─────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I18 - Sensor Push via IPC");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* ส่งข้อมูลเซ็นเซอร์ */
    lv_obj_t *th_sub = example_label_create(parent,
        "ส่งข้อมูลเซ็นเซอร์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Explanation */
    lv_obj_t *desc = lv_label_create(parent);
    lv_label_set_text(desc, "CM33 reads BMI270 -> packs into IPC msg -> CM55 displays");
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 32);

    /* Card container */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 500, 200);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x142240), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2a4060), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Value labels */
    s_lbl_x = lv_label_create(card);
    lv_label_set_text(s_lbl_x, "Accel X: -- mg");
    lv_obj_set_style_text_font(s_lbl_x, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_x, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(s_lbl_x, LV_ALIGN_TOP_LEFT, 20, 20);

    s_lbl_y = lv_label_create(card);
    lv_label_set_text(s_lbl_y, "Accel Y: -- mg");
    lv_obj_set_style_text_font(s_lbl_y, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_y, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(s_lbl_y, LV_ALIGN_TOP_LEFT, 20, 60);

    s_lbl_z = lv_label_create(card);
    lv_label_set_text(s_lbl_z, "Accel Z: -- mg");
    lv_obj_set_style_text_font(s_lbl_z, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lbl_z, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(s_lbl_z, LV_ALIGN_TOP_LEFT, 20, 100);

    s_lbl_updates = lv_label_create(card);
    lv_label_set_text(s_lbl_updates, "Updates received: 0");
    lv_obj_set_style_text_font(s_lbl_updates, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_updates, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_updates, LV_ALIGN_BOTTOM_LEFT, 20, -10);

    /* Register IPC callback + start display timer */
    memset((void *)&s_rx_buf, 0, sizeof(s_rx_buf));
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                  sensor_data_cb, IPC_CMD_SENSOR_DATA);
    lv_timer_create(display_timer_cb, 100, NULL);
}
