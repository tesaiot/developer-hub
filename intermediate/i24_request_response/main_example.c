/**
 * @file    main_example.c
 * @brief   IPC Request-Response - Synchronous query with timeout
 *
 * Sends request via IPC, polls volatile flag for response, times out
 * after configurable duration.  Shows round-trip time on screen.
 */

#include "pse84_common.h"
#include "ipc_communication.h"

#define IPC_CMD_QUERY_REQ   (0xB0)
#define IPC_CMD_QUERY_RSP   (0xB1)
#define TIMEOUT_MS          (2000)

/* ── Response state ──────────────────────────────────────────────── */
static volatile bool     s_rsp_received;
static volatile uint8_t  s_rsp_value;
/* CRITICAL: Must be in shared SRAM for cross-core IPC */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;

static lv_obj_t *s_lbl_result;
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_status;

/* ── Response callback - ISR context ─────────────────────────────── */
static void response_cb(uint32_t *msg_ptr)
{
    ipc_msg_t *msg = (ipc_msg_t *)msg_ptr;
    if (msg->cmd != IPC_CMD_QUERY_RSP) return;

    s_rsp_value    = msg->data[0];
    s_rsp_received = true;   /* signal to polling loop */
}

/* ── Blocking request with timeout ───────────────────────────────── */
static bool send_request_blocking(uint8_t query_id, uint8_t *out_value)
{
    s_rsp_received = false;

    s_ipc_tx.cmd     = IPC_CMD_QUERY_REQ;
    s_ipc_tx.data[0] = query_id;

    cy_en_ipc_pipe_status_t st =
        Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                                CM55_IPC_PIPE_EP_ADDR,
                                (uint32_t *)&s_ipc_tx, NULL);
    if (st != CY_IPC_PIPE_SUCCESS) return false;

    /* Poll with timeout */
    uint32_t start = xTaskGetTickCount();
    while (!s_rsp_received) {
        uint32_t elapsed = (xTaskGetTickCount() - start) * portTICK_PERIOD_MS;
        if (elapsed >= TIMEOUT_MS) return false;   /* timed out */
        vTaskDelay(pdMS_TO_TICKS(1));               /* yield CPU */
    }

    *out_value = s_rsp_value;
    return true;
}

/* ── Button event - trigger request ──────────────────────────────── */
static void btn_query_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_label_set_text(s_lbl_status, "Status: Waiting...");
    lv_obj_set_style_text_color(s_lbl_status,
                                lv_palette_main(LV_PALETTE_YELLOW), 0);

    uint32_t t0 = xTaskGetTickCount();
    uint8_t  value = 0;
    bool ok = send_request_blocking(0x01, &value);
    uint32_t dt = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;

    if (ok) {
        lv_label_set_text_fmt(s_lbl_result, "Response: 0x%02X", value);
        lv_label_set_text(s_lbl_status, "Status: OK");
        lv_obj_set_style_text_color(s_lbl_status,
                                    lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        lv_label_set_text(s_lbl_result, "Response: TIMEOUT");
        lv_label_set_text(s_lbl_status, "Status: Timed out");
        lv_obj_set_style_text_color(s_lbl_status,
                                    lv_palette_main(LV_PALETTE_RED), 0);
    }

    lv_label_set_text_fmt(s_lbl_time, "Round-trip: %u ms", (unsigned)dt);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I20 - IPC Request-Response");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* ร้องขอ-ตอบกลับ IPC */
    lv_obj_t *th_sub = example_label_create(parent,
        "ร้องขอ-ตอบกลับ IPC",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* Result label */
    s_lbl_result = lv_label_create(parent);
    lv_label_set_text(s_lbl_result, "Response: --");
    lv_obj_set_style_text_font(s_lbl_result, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_result, lv_color_white(), 0);
    lv_obj_align(s_lbl_result, LV_ALIGN_CENTER, 0, -50);

    /* Time label */
    s_lbl_time = lv_label_create(parent);
    lv_label_set_text(s_lbl_time, "Round-trip: -- ms");
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_time, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, -15);

    /* Status label */
    s_lbl_status = lv_label_create(parent);
    lv_label_set_text(s_lbl_status, "Status: Idle");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_status, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_CENTER, 0, 15);

    /* Query button */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 70);
    lv_obj_add_event_cb(btn, btn_query_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_TEAL), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Send Query");
    lv_obj_center(btn_lbl);

    /* Register response callback */
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                  response_cb, IPC_CMD_QUERY_RSP);
}
