/**
 * @file    main_example.c
 * @brief   Crypto Algorithm Visualization
 *
 * @description
 *   Educational display of cryptographic concepts: ECC, SHA-256, AES, RNG.
 *   Animated progress bars simulate processing steps. Uses lv_timer_create()
 *   for animation - no OPTIGA or IPC calls needed.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Colors
 * --------------------------------------------------------------------------- */
#define COLOR_BG                lv_color_hex(0x142240)
#define COLOR_CARD              lv_color_hex(0x0D1B2A)
#define COLOR_TEXT              lv_color_hex(0xE0E0E0)
#define COLOR_SUCCESS           lv_color_hex(0x4CAF50)
#define COLOR_ERROR             lv_color_hex(0xF44336)
#define COLOR_PENDING           lv_color_hex(0x616161)
#define COLOR_ACTIVE            lv_color_hex(0xFF9800)
#define COLOR_ACCENT            lv_color_hex(0x7C4DFF)
#define COLOR_BAR_BG            lv_color_hex(0x1A3050)

/* ---------------------------------------------------------------------------
 * Algorithm step definitions
 * --------------------------------------------------------------------------- */
#define NUM_ALGOS   4
#define ANIM_STEP   5       /* bar increment per timer tick */
#define ANIM_MS     60      /* timer period in ms */

typedef struct {
    const char *name;
    const char *icon;
    const char *description;
    const char *result_text;
    lv_color_t  color;
} algo_def_t;

static const algo_def_t ALGOS[NUM_ALGOS] = {
    {
        "SHA-256 Hash",
        LV_SYMBOL_REFRESH,
        "256-bit digest\nOne-way function",
        "E3B0C44298FC1C14...",
        {.blue = 0x50, .green = 0xAF, .red = 0x4C}  /* green */
    },
    {
        "ECC P-256 Sign",
        LV_SYMBOL_EDIT,
        "ECDSA signature\nKey slot 0xE0F1",
        "3045022100A1B2C3...",
        {.blue = 0xFF, .green = 0x4D, .red = 0x7C}  /* purple */
    },
    {
        "AES-128 CCM",
        LV_SYMBOL_EYE_OPEN,
        "Authenticated enc\n12-byte nonce",
        "Ciphertext + 8B tag",
        {.blue = 0xF3, .green = 0x96, .red = 0x21}  /* blue */
    },
    {
        "TRNG (256-bit)",
        LV_SYMBOL_SHUFFLE,
        "Hardware RNG\nNIST SP 800-90B",
        "7F2A91E4C83D05B6...",
        {.blue = 0x00, .green = 0x98, .red = 0xFF}  /* orange */
    },
};

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *bars[NUM_ALGOS];
    lv_obj_t    *result_labels[NUM_ALGOS];
    lv_obj_t    *status_dots[NUM_ALGOS];
    lv_obj_t    *btn_run;
    lv_obj_t    *status_label;
    lv_timer_t  *anim_timer;
    int          current_algo;
    int          bar_value;
    bool         running;
} crypto_viz_ctx_t;

static crypto_viz_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Animation timer: advances progress bars sequentially
 * --------------------------------------------------------------------------- */
static void anim_timer_cb(lv_timer_t *timer)
{
    crypto_viz_ctx_t *ctx = (crypto_viz_ctx_t *)lv_timer_get_user_data(timer);

    if (ctx->current_algo >= NUM_ALGOS) {
        /* All done */
        lv_label_set_text(ctx->status_label, "All algorithms completed");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
        lv_obj_remove_flag(ctx->btn_run, LV_OBJ_FLAG_HIDDEN);
        ctx->running = false;
        lv_timer_delete(timer);
        ctx->anim_timer = NULL;
        return;
    }

    int idx = ctx->current_algo;

    /* Show active dot */
    lv_obj_set_style_bg_color(ctx->status_dots[idx], COLOR_ACTIVE, 0);

    /* Update status */
    char buf[64];
    snprintf(buf, sizeof(buf), "Processing: %s (%d%%)",
             ALGOS[idx].name, ctx->bar_value);
    lv_label_set_text(ctx->status_label, buf);

    /* Advance bar */
    ctx->bar_value += ANIM_STEP;
    lv_bar_set_value(ctx->bars[idx], ctx->bar_value, LV_ANIM_ON);

    if (ctx->bar_value >= 100) {
        /* This algo is done */
        lv_obj_set_style_bg_color(ctx->status_dots[idx], COLOR_SUCCESS, 0);
        lv_label_set_text(ctx->result_labels[idx], ALGOS[idx].result_text);
        lv_obj_set_style_text_color(ctx->result_labels[idx], COLOR_SUCCESS, 0);

        /* Move to next */
        ctx->current_algo++;
        ctx->bar_value = 0;
    }
}

/* ---------------------------------------------------------------------------
 * Run button callback
 * --------------------------------------------------------------------------- */
static void btn_run_cb(lv_event_t *e)
{
    crypto_viz_ctx_t *ctx = (crypto_viz_ctx_t *)lv_event_get_user_data(e);
    if (ctx->running) return;

    /* Reset all */
    for (int i = 0; i < NUM_ALGOS; i++) {
        lv_bar_set_value(ctx->bars[i], 0, LV_ANIM_OFF);
        lv_label_set_text(ctx->result_labels[i], "---");
        lv_obj_set_style_text_color(ctx->result_labels[i], COLOR_TEXT, 0);
        lv_obj_set_style_bg_color(ctx->status_dots[i], COLOR_PENDING, 0);
    }

    ctx->current_algo = 0;
    ctx->bar_value = 0;
    ctx->running = true;
    lv_obj_add_flag(ctx->btn_run, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->status_label, "Starting...");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);

    ctx->anim_timer = lv_timer_create(anim_timer_cb, ANIM_MS, ctx);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Crypto Algorithm Visualization");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* เข้ารหัส OPTIGA */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เข้ารหัส OPTIGA");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 28);

    /* UI Reference banner */
    lv_obj_t *ref_banner = lv_label_create(parent);
    lv_label_set_text(ref_banner, LV_SYMBOL_WARNING " UI Reference - real benchmarks in a17_hsm_crypto");
    lv_obj_set_style_text_color(ref_banner, lv_color_hex(0xFF9800), 0);
    lv_obj_align(ref_banner, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* Status */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Run to simulate crypto operations");
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 30);

    /* Run button */
    s_ctx.btn_run = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_run, 120, 36);
    lv_obj_align(s_ctx.btn_run, LV_ALIGN_TOP_RIGHT, -16, 26);
    lv_obj_set_style_bg_color(s_ctx.btn_run, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(s_ctx.btn_run, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_run, btn_run_cb, LV_EVENT_CLICKED, &s_ctx);

    lv_obj_t *btn_lbl = lv_label_create(s_ctx.btn_run);
    lv_label_set_text(btn_lbl, LV_SYMBOL_PLAY " Run");
    lv_obj_center(btn_lbl);

    /* Algorithm cards */
    int card_w = 370;
    int card_h = 80;
    int start_y = 66;
    int gap = 6;

    for (int i = 0; i < NUM_ALGOS; i++) {
        int col = (i % 2);
        int row = (i / 2);
        int x = 8 + col * (card_w + 8);
        int y = start_y + row * (card_h + gap);

        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, x, y);
        lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
        lv_obj_set_style_border_color(card, ALGOS[i].color, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* Status dot */
        s_ctx.status_dots[i] = lv_obj_create(card);
        lv_obj_set_size(s_ctx.status_dots[i], 12, 12);
        lv_obj_set_style_radius(s_ctx.status_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_ctx.status_dots[i], COLOR_PENDING, 0);
        lv_obj_set_style_bg_opa(s_ctx.status_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_ctx.status_dots[i], 0, 0);
        lv_obj_align(s_ctx.status_dots[i], LV_ALIGN_TOP_LEFT, 0, 2);

        /* Name + icon */
        lv_obj_t *name = lv_label_create(card);
        char name_buf[64];
        snprintf(name_buf, sizeof(name_buf), "%s %s", ALGOS[i].icon, ALGOS[i].name);
        lv_label_set_text(name, name_buf);
        lv_obj_set_style_text_color(name, ALGOS[i].color, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 18, 0);

        /* Description */
        lv_obj_t *desc = lv_label_create(card);
        lv_label_set_text(desc, ALGOS[i].description);
        lv_obj_set_style_text_color(desc, COLOR_TEXT, 0);
        lv_obj_align(desc, LV_ALIGN_TOP_RIGHT, -4, 0);

        /* Progress bar */
        s_ctx.bars[i] = lv_bar_create(card);
        lv_obj_set_size(s_ctx.bars[i], 200, 10);
        lv_bar_set_range(s_ctx.bars[i], 0, 100);
        lv_bar_set_value(s_ctx.bars[i], 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_ctx.bars[i], COLOR_BAR_BG, 0);
        lv_obj_set_style_bg_color(s_ctx.bars[i], ALGOS[i].color, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_ctx.bars[i], 4, 0);
        lv_obj_set_style_radius(s_ctx.bars[i], 4, LV_PART_INDICATOR);
        lv_obj_align(s_ctx.bars[i], LV_ALIGN_BOTTOM_LEFT, 0, 0);

        /* Result label */
        s_ctx.result_labels[i] = lv_label_create(card);
        lv_label_set_text(s_ctx.result_labels[i], "---");
        lv_obj_set_style_text_color(s_ctx.result_labels[i], COLOR_TEXT, 0);
        lv_obj_align(s_ctx.result_labels[i], LV_ALIGN_BOTTOM_RIGHT, -4, 0);
    }

    /* Info note at bottom */
    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note, "Visualization only - actual OPTIGA Trust M operations require I2C bus access");
    lv_obj_set_style_text_color(note, COLOR_PENDING, 0);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -6);
}
