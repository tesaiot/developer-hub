/**
 * @file    main_example.c
 * @brief   HSM Crypto Benchmark - Real OPTIGA via IPC_CMD_HSM_BENCHMARK
 *
 * @description
 *   Runs 5 real crypto benchmarks on OPTIGA Trust M via IPC_CMD_HSM_BENCHMARK
 *   (0xB6) from CM55 → CM33_NS. CM33_NS executes each operation and returns
 *   actual timing in milliseconds (5 × uint16_t LE).
 *
 *   Operations: Random(32B), SHA-256(256B), ECC P-256 KeyGen,
 *   ECDSA Sign, ECDSA Verify.
 *
 *   Falls back to datasheet-typical simulated timings if IPC times out.
 *
 * @board   AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/*******************************************************************************
 * HSM Color Palette (from production page_hsm.c)
 *******************************************************************************/
#define HSM_COLOR_OK         0x50D890
#define HSM_COLOR_WARN       0xF2B84B
#define HSM_COLOR_CRIT       0xE85B5B
#define HSM_COLOR_BENCH      0x50D890
#define HSM_COLOR_TITLE      0xBB86FC

/* ---------------------------------------------------------------------------
 * IPC shared-memory buffers
 * --------------------------------------------------------------------------- */
CY_SECTION_SHAREDMEM static ipc_msg_t      s_bench_msg;
CY_SECTION_SHAREDMEM static ipc_response_t s_bench_resp;

/* Benchmark response offsets (from ipc_hsm_handler.h) */
#define HSM_BENCH_RANDOM_OFF   0
#define HSM_BENCH_HASH_OFF     2
#define HSM_BENCH_KEYGEN_OFF   4
#define HSM_BENCH_SIGN_OFF     6
#define HSM_BENCH_VERIFY_OFF   8
static const uint8_t BENCH_OFFSETS[5] = {
    HSM_BENCH_RANDOM_OFF, HSM_BENCH_HASH_OFF, HSM_BENCH_KEYGEN_OFF,
    HSM_BENCH_SIGN_OFF, HSM_BENCH_VERIFY_OFF
};

/* Real benchmark results (filled by IPC) */
static uint16_t s_bench_ms[5];
static bool     s_bench_ipc_ok = false;

/* Send IPC_CMD_HSM_BENCHMARK (0xB6) and parse 5 timings */
static bool run_real_benchmark(void)
{
    memset(&s_bench_msg, 0, sizeof(s_bench_msg));
    memset(&s_bench_resp, 0, sizeof(s_bench_resp));

    s_bench_msg.client_id = CM33_IPC_HSM_CLIENT_ID;
    s_bench_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
    s_bench_msg.cmd       = IPC_CMD_HSM_BENCHMARK;
    s_bench_msg.value     = (uint32_t)(uintptr_t)&s_bench_resp;
    s_bench_resp.ready    = 0;

    cy_en_ipc_pipe_status_t st = CY_IPC_PIPE_ERROR_NO_IPC;
    for (int i = 0; i < 20; i++) {
        st = Cy_IPC_Pipe_SendMessage(
            CM33_IPC_PIPE_EP_ADDR, CM55_IPC_PIPE_EP_ADDR,
            (void *)&s_bench_msg, NULL);
        if (st == CY_IPC_PIPE_SUCCESS) break;
        Cy_SysLib_DelayUs(200);
    }
    if (st != CY_IPC_PIPE_SUCCESS) return false;

    /* Benchmark takes longer - wait up to 30s */
    uint32_t elapsed = 0;
    while (s_bench_resp.ready != 1 && elapsed < 30000) {
        Cy_SysLib_DelayUs(1000);
        elapsed++;
    }
    if (s_bench_resp.ready != 1 || s_bench_resp.status != 0) return false;

    /* Parse 5 × uint16_t LE timings */
    for (int i = 0; i < 5; i++) {
        uint8_t off = BENCH_OFFSETS[i];
        s_bench_ms[i] = (uint16_t)s_bench_resp.data[off] |
                         ((uint16_t)s_bench_resp.data[off + 1] << 8);
    }
    return true;
}

/*******************************************************************************
 * Benchmark Operation Definitions
 *******************************************************************************/
#define NUM_BENCH_OPS  5

typedef struct {
    const char *name;
    const char *ipc_cmd;
    uint16_t typical_ms;
} bench_op_def_t;

static const bench_op_def_t BENCH_OPS[NUM_BENCH_OPS] = {
    { "Random (32 B)",    "IPC_CMD_HSM_BENCHMARK (0xB6)", 15  },
    { "SHA-256 (256 B)",  "IPC_CMD_HSM_BENCHMARK (0xB6)", 30  },
    { "ECC P-256 KeyGen", "IPC_CMD_HSM_BENCHMARK (0xB6)", 80  },
    { "ECDSA Sign",       "IPC_CMD_HSM_BENCHMARK (0xB6)", 70  },
    { "ECDSA Verify",     "IPC_CMD_HSM_BENCHMARK (0xB6)", 80  },
};

/*******************************************************************************
 * UI State
 *******************************************************************************/
typedef struct {
    lv_obj_t *result_labels[NUM_BENCH_OPS];
    lv_obj_t *time_labels[NUM_BENCH_OPS];
    lv_obj_t *bars[NUM_BENCH_OPS];
    lv_obj_t *run_btns[NUM_BENCH_OPS];
    lv_obj_t *run_all_btn;
    lv_obj_t *status_label;
    bool running;
} bench_state_t;

static bench_state_t s_bench;

/*******************************************************************************
 * Display result for a single operation
 *******************************************************************************/
static void bench_show_result(int idx, uint16_t actual_ms)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u ms", actual_ms);
    lv_label_set_text(s_bench.time_labels[idx], buf);

    uint32_t time_color = (actual_ms <= BENCH_OPS[idx].typical_ms * 2)
                          ? HSM_COLOR_OK : HSM_COLOR_WARN;
    lv_obj_set_style_text_color(s_bench.time_labels[idx],
                                 lv_color_hex(time_color), 0);

    lv_label_set_text(s_bench.result_labels[idx],
                      s_bench_ipc_ok ? "PASS (real)" : "Est. (typical)");
    lv_obj_set_style_text_color(s_bench.result_labels[idx],
                                 lv_color_hex(HSM_COLOR_OK), 0);

    lv_bar_set_value(s_bench.bars[idx], 100, LV_ANIM_ON);
}

/*******************************************************************************
 * "Run All" - sends single IPC, shows all 5 real timings
 *******************************************************************************/
static void run_all_cb(lv_event_t *e)
{
    (void)e;
    if (s_bench.running) return;
    s_bench.running = true;

    /* Reset all */
    for (int i = 0; i < NUM_BENCH_OPS; i++) {
        lv_bar_set_value(s_bench.bars[i], 0, LV_ANIM_OFF);
        lv_label_set_text(s_bench.result_labels[i], "Running...");
        lv_obj_set_style_text_color(s_bench.result_labels[i],
                                     lv_color_hex(0xFF9800), 0);
        lv_label_set_text(s_bench.time_labels[i], "---");
    }

    lv_label_set_text(s_bench.status_label,
        "Running IPC_CMD_HSM_BENCHMARK (0xB6)...");
    lv_obj_set_style_text_color(s_bench.status_label,
                                 lv_color_hex(0xFF9800), 0);

    /* Force LVGL to redraw before blocking IPC call */
    lv_refr_now(NULL);

    /* Run real benchmark via IPC */
    s_bench_ipc_ok = run_real_benchmark();

    if (!s_bench_ipc_ok) {
        /* Fallback: use datasheet typical timings */
        for (int i = 0; i < NUM_BENCH_OPS; i++) {
            s_bench_ms[i] = BENCH_OPS[i].typical_ms;
        }
    }

    /* Display results */
    for (int i = 0; i < NUM_BENCH_OPS; i++) {
        bench_show_result(i, s_bench_ms[i]);
    }

    lv_label_set_text(s_bench.status_label,
        s_bench_ipc_ok ? LV_SYMBOL_OK " Benchmark complete (real OPTIGA timings)"
                       : LV_SYMBOL_WARNING " IPC timeout - showing datasheet estimates");
    lv_obj_set_style_text_color(s_bench.status_label,
        lv_color_hex(s_bench_ipc_ok ? HSM_COLOR_OK : HSM_COLOR_WARN), 0);

    s_bench.running = false;
}

/*******************************************************************************
 * Single "Run" button - re-runs full benchmark, shows one result
 *******************************************************************************/
static void single_run_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_bench.running) return;

    lv_bar_set_value(s_bench.bars[idx], 0, LV_ANIM_OFF);
    lv_label_set_text(s_bench.result_labels[idx], "Running...");
    lv_obj_set_style_text_color(s_bench.result_labels[idx],
                                 lv_color_hex(0xFF9800), 0);
    lv_label_set_text(s_bench.time_labels[idx], "---");
    lv_refr_now(NULL);

    s_bench_ipc_ok = run_real_benchmark();
    if (!s_bench_ipc_ok) {
        s_bench_ms[idx] = BENCH_OPS[idx].typical_ms;
    }
    bench_show_result(idx, s_bench_ms[idx]);
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    memset(&s_bench, 0, sizeof(s_bench));

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title,
        LV_SYMBOL_EYE_OPEN " OPTIGA Trust M - Crypto Benchmark");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(HSM_COLOR_BENCH), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* ทดสอบเข้ารหัส HSM */
    thai_label(parent, "ทดสอบเข้ารหัส HSM", 14, UI_COLOR_TEXT_DIM);

    /* Status line */
    s_bench.status_label = lv_label_create(parent);
    lv_label_set_text(s_bench.status_label,
        "Tap Run All or individual Run buttons");
    lv_obj_set_style_text_color(s_bench.status_label,
                                 lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_pos(s_bench.status_label, 16, 30);

    /* Run All button */
    s_bench.run_all_btn = lv_btn_create(parent);
    lv_obj_set_size(s_bench.run_all_btn, 120, 32);
    lv_obj_align(s_bench.run_all_btn, LV_ALIGN_TOP_RIGHT, -16, 26);
    lv_obj_set_style_bg_color(s_bench.run_all_btn,
                               lv_color_hex(HSM_COLOR_BENCH), 0);
    lv_obj_set_style_radius(s_bench.run_all_btn, 8, 0);
    lv_obj_set_style_shadow_width(s_bench.run_all_btn, 0, 0);
    lv_obj_add_event_cb(s_bench.run_all_btn, run_all_cb,
                         LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(s_bench.run_all_btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_PLAY " Run All");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(btn_lbl);

    /* Column header labels */
    int hdr_y = 60;
    lv_obj_t *h_op = lv_label_create(parent);
    lv_label_set_text(h_op, "Operation");
    lv_obj_set_style_text_color(h_op, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_op, 16, hdr_y);

    lv_obj_t *h_typ = lv_label_create(parent);
    lv_label_set_text(h_typ, "Typical");
    lv_obj_set_style_text_color(h_typ, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_typ, 180, hdr_y);

    lv_obj_t *h_act = lv_label_create(parent);
    lv_label_set_text(h_act, "Actual");
    lv_obj_set_style_text_color(h_act, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_act, 240, hdr_y);

    lv_obj_t *h_res = lv_label_create(parent);
    lv_label_set_text(h_res, "Result");
    lv_obj_set_style_text_color(h_res, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(h_res, 310, hdr_y);

    /* Benchmark operation cards */
    int card_y_start = 80;
    int card_h = 58;
    int card_gap = 4;

    for (int i = 0; i < NUM_BENCH_OPS; i++) {
        int y = card_y_start + i * (card_h + card_gap);
        char buf[32];

        /* Card background */
        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_set_size(card, DISPLAY_WIDTH - 16, card_h);
        lv_obj_set_pos(card, 8, y);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1B2A), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x2A3A5C), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* Operation name */
        lv_obj_t *op_lbl = lv_label_create(card);
        lv_label_set_text(op_lbl, BENCH_OPS[i].name);
        lv_obj_set_style_text_color(op_lbl, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_font(op_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(op_lbl, 0, 0);

        /* IPC command reference (small, dimmed) */
        lv_obj_t *ipc_lbl = lv_label_create(card);
        lv_label_set_text(ipc_lbl, BENCH_OPS[i].ipc_cmd);
        lv_obj_set_style_text_color(ipc_lbl, lv_color_hex(0x606060), 0);
        lv_obj_set_pos(ipc_lbl, 0, 18);

        /* Typical time */
        snprintf(buf, sizeof(buf), "%u ms", BENCH_OPS[i].typical_ms);
        lv_obj_t *typ_lbl = lv_label_create(card);
        lv_label_set_text(typ_lbl, buf);
        lv_obj_set_style_text_color(typ_lbl, lv_color_hex(0x808080), 0);
        lv_obj_set_pos(typ_lbl, 172, 0);

        /* Actual time (filled after run) */
        s_bench.time_labels[i] = lv_label_create(card);
        lv_label_set_text(s_bench.time_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.time_labels[i],
                                     lv_color_hex(0x808080), 0);
        lv_obj_set_pos(s_bench.time_labels[i], 232, 0);

        /* Result label */
        s_bench.result_labels[i] = lv_label_create(card);
        lv_label_set_text(s_bench.result_labels[i], "---");
        lv_obj_set_style_text_color(s_bench.result_labels[i],
                                     lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_pos(s_bench.result_labels[i], 302, 0);

        /* Progress bar */
        s_bench.bars[i] = lv_bar_create(card);
        lv_obj_set_size(s_bench.bars[i], 280, 6);
        lv_bar_set_range(s_bench.bars[i], 0, 100);
        lv_bar_set_value(s_bench.bars[i], 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bench.bars[i],
                                   lv_color_hex(0x1A3050), 0);
        lv_obj_set_style_bg_color(s_bench.bars[i],
                                   lv_color_hex(HSM_COLOR_BENCH),
                                   LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_bench.bars[i], 3, 0);
        lv_obj_set_style_radius(s_bench.bars[i], 3, LV_PART_INDICATOR);
        lv_obj_set_pos(s_bench.bars[i], 0, 36);

        /* Individual Run button */
        s_bench.run_btns[i] = lv_btn_create(card);
        lv_obj_set_size(s_bench.run_btns[i], 60, 24);
        lv_obj_set_style_bg_color(s_bench.run_btns[i],
                                   lv_color_hex(0x2A3A5C), 0);
        lv_obj_set_style_radius(s_bench.run_btns[i], 6, 0);
        lv_obj_set_style_shadow_width(s_bench.run_btns[i], 0, 0);
        lv_obj_align(s_bench.run_btns[i], LV_ALIGN_TOP_RIGHT, 0, -4);
        lv_obj_add_event_cb(s_bench.run_btns[i], single_run_cb,
                             LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *rb = lv_label_create(s_bench.run_btns[i]);
        lv_label_set_text(rb, "Run");
        lv_obj_set_style_text_color(rb, lv_color_hex(0xE0E0E0), 0);
        lv_obj_center(rb);
    }

    /* Summary note at bottom */
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note,
        "Typical = Infineon SLS 32AIA datasheet. "
        "Actual = real OPTIGA measurement via IPC_CMD_HSM_BENCHMARK (0xB6) on CM33_NS.");
    lv_obj_set_style_text_color(note, lv_color_hex(0x606060), 0);
    lv_obj_set_pos(note, 16, card_y_start + NUM_BENCH_OPS * (card_h + card_gap) + 4);
    lv_obj_set_width(note, DISPLAY_WIDTH - 32);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
}
