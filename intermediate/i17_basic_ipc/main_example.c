/**
 * @file    main_example.c
 * @brief   Basic IPC - CM33↔CM55 ping-pong with SendMessage/RegisterCallback
 *
 * CRITICAL: Never call Cy_IPC_Pipe_SendMessage from inside a
 * RegisterCallback handler - use the deferred flag pattern instead.
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/* ── IPC command ID for this example ─────────────────────────────── */
#define IPC_CMD_PING     (0xE0)
#define IPC_CMD_PONG     (0xE1)

/* ── Shared state ────────────────────────────────────────────────── */
static volatile uint32_t s_counter;
static volatile bool     s_need_pong;     /* deferred reply flag */

static lv_obj_t *s_lbl_count;
static lv_obj_t *s_lbl_dir;

/* ── IPC message structure ───────────────────────────────────────── */
/* CRITICAL: IPC TX buffer MUST be in shared SRAM - the other core
 * reads this memory via the pointer passed to SendMessage.
 * Without CY_SECTION_SHAREDMEM the buffer sits in core-private
 * SRAM and the receiver gets garbage or a BusFault. */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;

/* ── Callback - runs in IPC ISR context, DO NOT call SendMessage ── */
static void ipc_rx_cb(uint32_t *msg_ptr)
{
    ipc_msg_t *msg = (ipc_msg_t *)msg_ptr;

    if (msg->cmd == IPC_CMD_PING) {
        /* Received ping from CM33 - schedule pong from task loop */
        s_counter++;
        s_need_pong = true;   /* DEFERRED - will send in LVGL timer */
    }
}

/* ── Send a ping message to CM33 ─────────────────────────────────── */
static void send_ping(void)
{
    s_ipc_tx.cmd     = IPC_CMD_PING;
    s_ipc_tx.data[0] = (uint8_t)(s_counter & 0xFF);

    Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                            CM55_IPC_PIPE_EP_ADDR,
                            (uint32_t *)&s_ipc_tx, NULL);
}

/* ── Button event - initiate first ping ──────────────────────────── */
static void btn_ping_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        s_counter++;
        send_ping();
        lv_label_set_text(s_lbl_dir, "Direction: CM55 -> CM33 (PING)");
    }
}

/* ── LVGL timer - process deferred pong ──────────────────────────── */
static void poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_need_pong) {
        s_need_pong = false;

        /* Safe to call SendMessage here - we are in task context */
        s_ipc_tx.cmd     = IPC_CMD_PONG;
        s_ipc_tx.data[0] = (uint8_t)(s_counter & 0xFF);

        Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                                CM55_IPC_PIPE_EP_ADDR,
                                (uint32_t *)&s_ipc_tx, NULL);

        lv_label_set_text(s_lbl_dir, "Direction: CM55 -> CM33 (PONG)");
    }

    lv_label_set_text_fmt(s_lbl_count, "Count: %u", (unsigned)s_counter);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I17 - Basic IPC Ping-Pong");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* การสื่อสารระหว่างแกน IPC */
    lv_obj_t *th_sub = example_label_create(parent,
        "การสื่อสารระหว่างแกน IPC",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Counter label */
    s_lbl_count = lv_label_create(parent);
    lv_label_set_text(s_lbl_count, "Count: 0");
    lv_obj_set_style_text_font(s_lbl_count, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lbl_count, lv_color_white(), 0);
    lv_obj_align(s_lbl_count, LV_ALIGN_CENTER, 0, -40);

    /* Direction label */
    s_lbl_dir = lv_label_create(parent);
    lv_label_set_text(s_lbl_dir, "Direction: --");
    lv_obj_set_style_text_font(s_lbl_dir, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_dir, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_dir, LV_ALIGN_CENTER, 0, 0);

    /* Ping button */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_event_cb(btn, btn_ping_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_TEAL), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Send Ping");
    lv_obj_center(btn_lbl);

    /* Warning label */
    lv_obj_t *warn = lv_label_create(parent);
    lv_label_set_text(warn, "NOTE: Never call SendMessage inside RegisterCallback handler!");
    lv_obj_set_style_text_font(warn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(warn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_align(warn, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* Register IPC callback + start poll timer */
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                  ipc_rx_cb, IPC_CMD_PING);
    s_counter   = 0;
    s_need_pong = false;
    lv_timer_create(poll_timer_cb, 50, NULL);
}
