/**
 * @file    main_example.c
 * @brief   Deferred Flag Pattern - CRITICAL: avoid IPC pipe deadlock
 *
 * RULE: NEVER call Cy_IPC_Pipe_SendMessage() inside RegisterCallback.
 *
 * Pattern:
 *   1. ISR callback sets volatile flag + stores data
 *   2. Task/timer loop checks flag, calls SendMessage from task context
 *
 * Violating this rule → permanent display freeze (2026-03-10 incident).
 */

#include "pse84_common.h"
#include "ipc_communication.h"

#define IPC_CMD_DEFERRED_REQ   (0xC5)
#define IPC_CMD_DEFERRED_RSP   (0xC6)

/* ── Deferred reply state ────────────────────────────────────────── */
static volatile bool     s_need_reply;
static volatile uint8_t  s_reply_data;
static volatile uint32_t s_rx_count;
static volatile uint32_t s_tx_count;

/* CRITICAL: Must be in shared SRAM for cross-core IPC */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;
static lv_obj_t *s_lbl_rx;
static lv_obj_t *s_lbl_tx;
static lv_obj_t *s_lbl_log;

/*
 * ── ISR CALLBACK - runs in interrupt context ────────────────────
 *
 * DO NOT call Cy_IPC_Pipe_SendMessage here!
 * Only set flags and store data.
 */
static void ipc_isr_handler(uint32_t *msg_ptr)
{
    ipc_msg_t *msg = (ipc_msg_t *)msg_ptr;
    if (msg->cmd != IPC_CMD_DEFERRED_REQ) return;

    /* Store what we need for the reply */
    s_reply_data = msg->data[0] + 1;    /* echo back incremented */
    s_rx_count++;

    /* DEFERRED: set flag, do NOT send reply here */
    s_need_reply = true;
}

/*
 * ── LVGL TIMER - runs in GFX task context ──────────────────────
 *
 * Safe to call Cy_IPC_Pipe_SendMessage here.
 */
static void deferred_poll_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_need_reply) {
        s_need_reply = false;

        /* Now safe to send - we are in task context, pipe is free */
        s_ipc_tx.cmd     = IPC_CMD_DEFERRED_RSP;
        s_ipc_tx.data[0] = s_reply_data;

        cy_en_ipc_pipe_status_t st =
            Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                                    CM55_IPC_PIPE_EP_ADDR,
                                    (uint32_t *)&s_ipc_tx, NULL);

        if (st == CY_IPC_PIPE_SUCCESS) {
            s_tx_count++;
        }
    }

    /* Update UI */
    lv_label_set_text_fmt(s_lbl_rx, "Received (ISR): %u", (unsigned)s_rx_count);
    lv_label_set_text_fmt(s_lbl_tx, "Replied (Task): %u", (unsigned)s_tx_count);

    if (s_tx_count > 0) {
        lv_label_set_text_fmt(s_lbl_log,
            "Sequence:\n"
            "  1. IPC ISR fires -> sets s_need_reply = true\n"
            "  2. Timer poll -> sees flag -> calls SendMessage\n"
            "  3. Reply sent safely from task context\n\n"
            "Last reply data: 0x%02X", (unsigned)s_reply_data);
    }
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I21 - Deferred Flag Pattern (CRITICAL)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* แฟล็กเลื่อนเวลา */
    lv_obj_t *th_sub = example_label_create(parent,
        "แฟล็กเลื่อนเวลา",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Warning */
    lv_obj_t *warn = lv_label_create(parent);
    lv_label_set_text(warn, "NEVER call SendMessage inside RegisterCallback!");
    lv_obj_set_style_text_font(warn, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(warn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_align(warn, LV_ALIGN_TOP_MID, 0, 32);

    /* Counters */
    s_lbl_rx = lv_label_create(parent);
    lv_label_set_text(s_lbl_rx, "Received (ISR): 0");
    lv_obj_set_style_text_font(s_lbl_rx, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_rx, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(s_lbl_rx, LV_ALIGN_TOP_LEFT, 30, 70);

    s_lbl_tx = lv_label_create(parent);
    lv_label_set_text(s_lbl_tx, "Replied (Task): 0");
    lv_obj_set_style_text_font(s_lbl_tx, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_tx, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(s_lbl_tx, LV_ALIGN_TOP_RIGHT, -30, 70);

    /* Log area */
    s_lbl_log = lv_label_create(parent);
    lv_label_set_text(s_lbl_log, "Waiting for IPC messages...");
    lv_obj_set_style_text_font(s_lbl_log, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_log, lv_color_white(), 0);
    lv_obj_align(s_lbl_log, LV_ALIGN_CENTER, 0, 40);

    /* Init state */
    s_need_reply = false;
    s_rx_count   = 0;
    s_tx_count   = 0;

    /* Register callback + start poll timer */
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                  ipc_isr_handler, IPC_CMD_DEFERRED_REQ);
    lv_timer_create(deferred_poll_cb, 20, NULL);
}
