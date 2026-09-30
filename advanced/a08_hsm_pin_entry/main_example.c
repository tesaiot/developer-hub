/**
 * @file    main_example.c
 * @brief   HSM PIN Entry Overlay - production-quality numpad with OPTIGA reference
 *
 * @description
 *   Secure PIN entry overlay with full-screen numpad, shake animation on wrong
 *   PIN, green flash on success, 3-attempt lockout with countdown, and reference
 *   to OPTIGA Trust M hardware verification via IPC.
 *
 *   Features:
 *     - 4-digit PIN display with asterisk masking
 *     - 3x4 numpad grid: 1-9, Backspace, 0, Enter
 *     - Styled dark buttons with press highlight
 *     - Lateral shake animation on wrong PIN (lv_anim)
 *     - Green flash + "Access Granted" card on correct PIN
 *     - 3-attempt lockout with 30s countdown
 *     - IPC_CMD_OPTIGA_VERIFY_PIN (0xE5) reference for HW verification
 *
 *   In production, PIN hash is verified via OPTIGA Trust M SHA-256 on CM33_NS.
 *   This example uses local FNV-1a hash for standalone demonstration.
 *
 * @board   AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/* ── IPC Reference ─────────────────────────────────────────────────── */
#define IPC_CMD_OPTIGA_VERIFY_PIN   (0xE5)

/* ── Constants ─────────────────────────────────────────────────────── */
#define PIN_LEN             4
#define MAX_ATTEMPTS        3
#define LOCKOUT_SECONDS     30
#define SHAKE_DURATION_MS   300
#define SHAKE_AMPLITUDE     20

/* ── Colors ────────────────────────────────────────────────────────── */
#define COLOR_BG            lv_color_hex(0x0D1B2A)
#define COLOR_CARD          lv_color_hex(0x142240)
#define COLOR_OVERLAY       lv_color_hex(0x0A1628)
#define COLOR_TEXT          lv_color_hex(0xE0E0E0)
#define COLOR_KEY_BG        lv_color_hex(0x1A3050)
#define COLOR_KEY_PRESS     lv_color_hex(0x2196F3)
#define COLOR_ACCENT        lv_color_hex(0x7C4DFF)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_ERROR         lv_color_hex(0xF44336)
#define COLOR_WARNING       lv_color_hex(0xFF9800)
#define COLOR_LOCKED        lv_color_hex(0xB71C1C)

/* ── FNV-1a hash (demo only - production uses OPTIGA SHA-256) ──────── */
static uint32_t fnv1a_hash(const char *data, size_t len)
{
    uint32_t hash = 0x811C9DC5u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 0x01000193u;
    }
    return hash;
}

/* Pre-computed FNV-1a("1234") */
#define STORED_PIN_HASH  0x6222E842u

/* ── IPC TX buffer for OPTIGA verification (shared SRAM) ───────────── */
CY_SECTION_SHAREDMEM static ipc_msg_t s_ipc_tx;

/* ── Context ───────────────────────────────────────────────────────── */
typedef struct {
    lv_obj_t    *overlay;
    lv_obj_t    *pin_card;
    lv_obj_t    *pin_dots[PIN_LEN];
    lv_obj_t    *status_label;
    lv_obj_t    *attempts_label;
    lv_obj_t    *btnmatrix;
    lv_obj_t    *lockout_card;
    lv_obj_t    *lockout_label;
    lv_obj_t    *lockout_timer_label;
    lv_obj_t    *success_card;
    lv_obj_t    *ipc_note;
    lv_timer_t  *lockout_timer;
    lv_anim_t    shake_anim;
    char         pin[PIN_LEN + 1];
    uint8_t      pin_pos;
    uint8_t      attempts;
    bool         locked;
    int          lockout_remaining;
    int32_t      pin_card_orig_x;
} pin_ctx_t;

static pin_ctx_t s_ctx;

/* Button matrix map */
static const char *s_btnm_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
};

/* ── Update PIN dot display ────────────────────────────────────────── */
static void update_dots(pin_ctx_t *ctx)
{
    for (uint8_t i = 0; i < PIN_LEN; i++) {
        if (i < ctx->pin_pos) {
            /* Filled dot */
            lv_obj_set_style_bg_color(ctx->pin_dots[i], COLOR_ACCENT, 0);
            lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
        } else {
            /* Empty ring */
            lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_TRANSP, 0);
        }
    }
}

/* ── Shake animation callback ──────────────────────────────────────── */
static void shake_anim_cb(void *var, int32_t val)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    pin_ctx_t *ctx = &s_ctx;
    lv_obj_set_x(obj, ctx->pin_card_orig_x + val);
}

static void shake_anim_ready_cb(lv_anim_t *a)
{
    (void)a;
    /* Restore original position */
    lv_obj_set_x(s_ctx.pin_card, s_ctx.pin_card_orig_x);
}

static void trigger_shake(pin_ctx_t *ctx)
{
    ctx->pin_card_orig_x = lv_obj_get_x(ctx->pin_card);

    lv_anim_init(&ctx->shake_anim);
    lv_anim_set_var(&ctx->shake_anim, ctx->pin_card);
    lv_anim_set_values(&ctx->shake_anim, -SHAKE_AMPLITUDE, SHAKE_AMPLITUDE);
    lv_anim_set_duration(&ctx->shake_anim, SHAKE_DURATION_MS);
    lv_anim_set_repeat_count(&ctx->shake_anim, 2);
    lv_anim_set_path_cb(&ctx->shake_anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&ctx->shake_anim, shake_anim_cb);
    lv_anim_set_completed_cb(&ctx->shake_anim, shake_anim_ready_cb);
    lv_anim_start(&ctx->shake_anim);
}

/* ── Show success state ────────────────────────────────────────────── */
static void show_success(pin_ctx_t *ctx)
{
    ctx->attempts = 0;
    lv_label_set_text(ctx->status_label, "ACCESS GRANTED");
    lv_obj_set_style_text_color(ctx->status_label, COLOR_SUCCESS, 0);
    lv_label_set_text(ctx->attempts_label, "");
    lv_obj_remove_flag(ctx->success_card, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->btnmatrix, LV_OBJ_FLAG_HIDDEN);

    /* Flash dots green */
    for (uint8_t i = 0; i < PIN_LEN; i++) {
        lv_obj_set_style_bg_color(ctx->pin_dots[i], COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
    }
}

/* ── Lockout timer ─────────────────────────────────────────────────── */
static void lockout_tick_cb(lv_timer_t *timer)
{
    pin_ctx_t *ctx = (pin_ctx_t *)lv_timer_get_user_data(timer);

    ctx->lockout_remaining--;
    if (ctx->lockout_remaining <= 0) {
        /* Unlock */
        ctx->locked = false;
        ctx->attempts = 0;
        lv_obj_add_flag(ctx->lockout_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ctx->btnmatrix, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctx->status_label, "Enter PIN");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
        lv_label_set_text(ctx->attempts_label, "Attempts: 0/3");
        lv_timer_delete(timer);
        ctx->lockout_timer = NULL;

        /* Reset dots */
        update_dots(ctx);
    } else {
        lv_label_set_text_fmt(ctx->lockout_timer_label, "%d", ctx->lockout_remaining);
    }
}

/* ── Verify PIN ────────────────────────────────────────────────────── */
static void verify_pin(pin_ctx_t *ctx)
{
    /*
     * Production path (commented - requires CM33_NS OPTIGA handler):
     *   memset(&s_ipc_tx, 0, sizeof(s_ipc_tx));
     *   s_ipc_tx.cmd = IPC_CMD_OPTIGA_VERIFY_PIN;
     *   memcpy(s_ipc_tx.data, ctx->pin, PIN_LEN);
     *   Cy_IPC_Pipe_SendMessage(CY_IPC_EP_CYPIPE_CM33_ADDR,
     *                           CY_IPC_EP_CYPIPE_CM55_ADDR,
     *                           (uint32_t *)&s_ipc_tx, NULL);
     *
     * Then wait for IPC response with result (deferred flag pattern).
     */

    /* Demo: local FNV-1a verification */
    uint32_t hash = fnv1a_hash(ctx->pin, ctx->pin_pos);

    /* Clear PIN from memory immediately */
    memset(ctx->pin, 0, sizeof(ctx->pin));
    ctx->pin_pos = 0;

    if (hash == STORED_PIN_HASH) {
        show_success(ctx);
        return;
    }

    /* Wrong PIN */
    ctx->attempts++;
    trigger_shake(ctx);
    update_dots(ctx);

    char buf[32];
    snprintf(buf, sizeof(buf), "Attempts: %u/%u", ctx->attempts, MAX_ATTEMPTS);
    lv_label_set_text(ctx->attempts_label, buf);

    if (ctx->attempts >= MAX_ATTEMPTS) {
        /* Lock out */
        ctx->locked = true;
        ctx->lockout_remaining = LOCKOUT_SECONDS;
        lv_obj_add_flag(ctx->btnmatrix, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ctx->lockout_card, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(ctx->lockout_timer_label, "%d", LOCKOUT_SECONDS);
        lv_label_set_text(ctx->status_label, "LOCKED");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_LOCKED, 0);

        /* Red dots */
        for (uint8_t i = 0; i < PIN_LEN; i++) {
            lv_obj_set_style_bg_color(ctx->pin_dots[i], COLOR_ERROR, 0);
            lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
        }

        ctx->lockout_timer = lv_timer_create(lockout_tick_cb, 1000, ctx);
    } else {
        lv_label_set_text(ctx->status_label, "Wrong PIN");
        lv_obj_set_style_text_color(ctx->status_label, COLOR_WARNING, 0);
    }
}

/* ── Numpad callback ───────────────────────────────────────────────── */
static void numpad_cb(lv_event_t *e)
{
    pin_ctx_t *ctx = &s_ctx;
    if (ctx->locked) return;

    lv_obj_t *obj = lv_event_get_target(e);
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    const char *txt = lv_btnmatrix_get_btn_text(obj, id);
    if (!txt) return;

    /* Hide success card on any interaction */
    lv_obj_add_flag(ctx->success_card, LV_OBJ_FLAG_HIDDEN);

    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        if (ctx->pin_pos > 0) {
            ctx->pin_pos--;
            ctx->pin[ctx->pin_pos] = '\0';
            update_dots(ctx);
            lv_label_set_text(ctx->status_label, "Enter PIN");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);
        }
    } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        if (ctx->pin_pos < PIN_LEN) {
            lv_label_set_text(ctx->status_label, "Enter all 4 digits");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_WARNING, 0);
            return;
        }
        verify_pin(ctx);
    } else {
        /* Digit key */
        if (ctx->pin_pos < PIN_LEN) {
            ctx->pin[ctx->pin_pos] = txt[0];
            ctx->pin_pos++;
            ctx->pin[ctx->pin_pos] = '\0';
            update_dots(ctx);
            lv_label_set_text(ctx->status_label, "Enter PIN");
            lv_obj_set_style_text_color(ctx->status_label, COLOR_TEXT, 0);

            /* Auto-submit when 4 digits entered */
            if (ctx->pin_pos == PIN_LEN) {
                verify_pin(ctx);
            }
        }
    }
}

/* ── Entry point ───────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));

    lv_obj_set_style_bg_color(parent, COLOR_BG, 0);

    /* ── Title with lock icon ──────────────────────────────────────── */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " HSM PIN Entry");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* ── Subtitle ──────────────────────────────────────────────────── */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "OPTIGA Trust M Secure Authentication");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x808080), 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 36);

    /* ป้อนรหัส PIN */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ป้อนรหัส PIN");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 52);

    /* ── PIN dots card ─────────────────────────────────────────────── */
    s_ctx.pin_card = example_card_create(parent, 240, 60, COLOR_CARD);
    lv_obj_align(s_ctx.pin_card, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_color(s_ctx.pin_card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_ctx.pin_card, 2, 0);

    /* 4 dot indicators */
    for (uint8_t i = 0; i < PIN_LEN; i++) {
        s_ctx.pin_dots[i] = lv_obj_create(s_ctx.pin_card);
        lv_obj_set_size(s_ctx.pin_dots[i], 20, 20);
        lv_obj_set_style_radius(s_ctx.pin_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_ctx.pin_dots[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(s_ctx.pin_dots[i], COLOR_ACCENT, 0);
        lv_obj_set_style_border_width(s_ctx.pin_dots[i], 2, 0);
        int x_off = -45 + i * 30;
        lv_obj_align(s_ctx.pin_dots[i], LV_ALIGN_CENTER, x_off, 0);
        lv_obj_clear_flag(s_ctx.pin_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* ── Status label ──────────────────────────────────────────────── */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Enter PIN (default: 1234)");
    lv_obj_set_style_text_font(s_ctx.status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ctx.status_label, COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_MID, 0, 130);

    /* ── Attempts label ────────────────────────────────────────────── */
    s_ctx.attempts_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.attempts_label, "Attempts: 0/3");
    lv_obj_set_style_text_font(s_ctx.attempts_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ctx.attempts_label, lv_color_hex(0x808080), 0);
    lv_obj_align(s_ctx.attempts_label, LV_ALIGN_TOP_MID, 0, 154);

    /* ── Numpad (btnmatrix) ────────────────────────────────────────── */
    s_ctx.btnmatrix = lv_btnmatrix_create(parent);
    lv_btnmatrix_set_map(s_ctx.btnmatrix, s_btnm_map);
    lv_obj_set_size(s_ctx.btnmatrix, 320, 240);
    lv_obj_align(s_ctx.btnmatrix, LV_ALIGN_TOP_MID, 0, 180);

    /* Dark theme styling */
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_ctx.btnmatrix, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ctx.btnmatrix, 0, 0);
    lv_obj_set_style_pad_all(s_ctx.btnmatrix, 8, 0);
    lv_obj_set_style_pad_gap(s_ctx.btnmatrix, 8, 0);

    /* Button items */
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_KEY_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_ctx.btnmatrix, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_ctx.btnmatrix, COLOR_KEY_PRESS,
                               LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(s_ctx.btnmatrix, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_ctx.btnmatrix, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_ctx.btnmatrix, 10, LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_ctx.btnmatrix, 0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(s_ctx.btnmatrix, 4, LV_PART_ITEMS);
    lv_obj_set_style_shadow_color(s_ctx.btnmatrix, lv_color_hex(0x000000), LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(s_ctx.btnmatrix, LV_OPA_30, LV_PART_ITEMS);

    lv_obj_add_event_cb(s_ctx.btnmatrix, numpad_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Lockout card (hidden) ─────────────────────────────────────── */
    s_ctx.lockout_card = example_card_create(parent, 280, 120, COLOR_LOCKED);
    lv_obj_align(s_ctx.lockout_card, LV_ALIGN_TOP_MID, 0, 240);
    lv_obj_add_flag(s_ctx.lockout_card, LV_OBJ_FLAG_HIDDEN);

    s_ctx.lockout_label = lv_label_create(s_ctx.lockout_card);
    lv_label_set_text(s_ctx.lockout_label, LV_SYMBOL_WARNING " Too many attempts");
    lv_obj_set_style_text_font(s_ctx.lockout_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ctx.lockout_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ctx.lockout_label, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *retry_lbl = lv_label_create(s_ctx.lockout_card);
    lv_label_set_text(retry_lbl, "Retry in:");
    lv_obj_set_style_text_font(retry_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(retry_lbl, lv_color_hex(0xFFCDD2), 0);
    lv_obj_align(retry_lbl, LV_ALIGN_CENTER, -30, 8);

    s_ctx.lockout_timer_label = lv_label_create(s_ctx.lockout_card);
    lv_label_set_text_fmt(s_ctx.lockout_timer_label, "%d", LOCKOUT_SECONDS);
    lv_obj_set_style_text_font(s_ctx.lockout_timer_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_ctx.lockout_timer_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ctx.lockout_timer_label, LV_ALIGN_CENTER, 30, 8);

    /* ── Success card (hidden) ─────────────────────────────────────── */
    s_ctx.success_card = example_card_create(parent, 280, 60, COLOR_SUCCESS);
    lv_obj_align(s_ctx.success_card, LV_ALIGN_TOP_MID, 0, 430);
    lv_obj_add_flag(s_ctx.success_card, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ok_icon = lv_label_create(s_ctx.success_card);
    lv_label_set_text(ok_icon, LV_SYMBOL_OK " Access Granted");
    lv_obj_set_style_text_color(ok_icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ok_icon, &lv_font_montserrat_20, 0);
    lv_obj_center(ok_icon);

    /* ── IPC reference note ────────────────────────────────────────── */
    s_ctx.ipc_note = lv_label_create(parent);
    lv_label_set_text(s_ctx.ipc_note,
        "Production: IPC_CMD_OPTIGA_VERIFY_PIN (0xE5)\n"
        "PIN hash verified via OPTIGA Trust M SHA-256 on CM33_NS");
    lv_obj_set_style_text_font(s_ctx.ipc_note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ctx.ipc_note, lv_color_hex(0x808080), 0);
    lv_obj_set_style_text_align(s_ctx.ipc_note, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_ctx.ipc_note, 440);
    lv_obj_align(s_ctx.ipc_note, LV_ALIGN_BOTTOM_MID, 0, -8);
}
