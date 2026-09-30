/**
 * @file    main_example.c
 * @brief   Touch Pause/Resume IPC - Shared I2C bus access pattern
 *
 * IPC_CMD_TOUCH_PAUSE (0xD6): CM33 requests CM55 to stop touch polling
 * IPC_CMD_TOUCH_RESUME (0xD7): CM33 signals CM55 to reinit touch driver
 *
 * Every PAUSE must have a matching RESUME - missing RESUME kills touch.
 */

#include "pse84_common.h"
#include "ipc_communication.h"

#define IPC_CMD_TOUCH_PAUSE    (0xD6)
#define IPC_CMD_TOUCH_RESUME   (0xD7)

/* CRITICAL: Must be in shared SRAM for cross-core IPC */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;
static lv_obj_t *s_lbl_state;
static lv_obj_t *s_lbl_count;
static lv_obj_t *s_lbl_log;
static uint32_t  s_pause_count;
static bool      s_paused;

/* ── Send touch pause command ────────────────────────────────────── */
static void send_touch_pause(void)
{
    s_ipc_tx.cmd = IPC_CMD_TOUCH_PAUSE;
    Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                            CM55_IPC_PIPE_EP_ADDR,
                            (uint32_t *)&s_ipc_tx, NULL);
}

/* ── Send touch resume command ───────────────────────────────────── */
static void send_touch_resume(void)
{
    s_ipc_tx.cmd = IPC_CMD_TOUCH_RESUME;
    Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                            CM55_IPC_PIPE_EP_ADDR,
                            (uint32_t *)&s_ipc_tx, NULL);
}

/* ── Simulate shared I2C access sequence ─────────────────────────── */
static void btn_access_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    /* Step 1: Pause touch */
    s_paused = true;
    send_touch_pause();
    lv_label_set_text(s_lbl_state, "Touch: PAUSED");
    lv_obj_set_style_text_color(s_lbl_state,
                                lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(s_lbl_log,
        "Sequence:\n"
        "  1. TOUCH_PAUSE (0xD6) sent\n"
        "  2. I2C access in progress...\n"
        "  3. (waiting)");

    /*
     * Step 2: Perform I2C access
     * In real code this would be the OPTIGA transaction.
     * Here we just delay to simulate the bus access time.
     */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Step 3: Resume touch */
    send_touch_resume();
    s_paused = false;
    s_pause_count++;

    lv_label_set_text(s_lbl_state, "Touch: ACTIVE");
    lv_obj_set_style_text_color(s_lbl_state,
                                lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_label_set_text_fmt(s_lbl_count, "Pause/Resume cycles: %u",
                          (unsigned)s_pause_count);
    lv_label_set_text(s_lbl_log,
        "Sequence:\n"
        "  1. TOUCH_PAUSE (0xD6) sent\n"
        "  2. I2C access complete (200 ms)\n"
        "  3. TOUCH_RESUME (0xD7) sent  [OK]");
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_pause_count = 0;
    s_paused = false;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I22 - Touch Pause/Resume (I2C Sharing)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    /* หยุดชั่วคราว/เล่นต่อ */
    lv_obj_t *th_sub = example_label_create(parent,
        "หยุดชั่วคราว/เล่นต่อ",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    /* State label */
    s_lbl_state = lv_label_create(parent);
    lv_label_set_text(s_lbl_state, "Touch: ACTIVE");
    lv_obj_set_style_text_font(s_lbl_state, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_lbl_state,
                                lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(s_lbl_state, LV_ALIGN_CENTER, 0, -60);

    /* Count label */
    s_lbl_count = lv_label_create(parent);
    lv_label_set_text(s_lbl_count, "Pause/Resume cycles: 0");
    lv_obj_set_style_text_font(s_lbl_count, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_count, lv_color_white(), 0);
    lv_obj_align(s_lbl_count, LV_ALIGN_CENTER, 0, -30);

    /* Log area */
    s_lbl_log = lv_label_create(parent);
    lv_label_set_text(s_lbl_log, "Tap button to simulate shared I2C access");
    lv_obj_set_style_text_font(s_lbl_log, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_log, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_lbl_log, LV_ALIGN_CENTER, 0, 30);

    /* Access button */
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 240, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, btn_access_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Simulate I2C Access");
    lv_obj_center(btn_lbl);
}
