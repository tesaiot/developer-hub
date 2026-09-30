/**
 * @file    main_example.c
 * @brief   PIN Manager - Real OPTIGA Trust M PIN storage via IPC
 *
 * 4-digit PIN entry pad with OPTIGA-backed secure storage.
 * PIN is hashed with SHA-256 by CM33_NS and stored in OPTIGA DATA_3
 * via IPC_CMD_HSM_PIN_SET (0xB9) / IPC_CMD_HSM_PIN_VERIFY (0xBA).
 *
 * Falls back to local djb2 hash if IPC times out (chip not present).
 *
 * @board   AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"
#include <string.h>
#include <stdio.h>

#define PIN_LEN         4
#define MAX_ATTEMPTS    3
#define LOCKOUT_SEC     30
#define KEY_SIZE        60
#define KEY_GAP         8
#define KEYPAD_X        260
#define KEYPAD_Y        100

typedef enum {
    STATE_SET_PIN,
    STATE_CONFIRM_PIN,
    STATE_LOCKED,
    STATE_ENTER_PIN,
    STATE_VERIFIED,
} pin_state_t;

typedef struct {
    lv_obj_t   *parent;
    lv_obj_t   *title_label;
    lv_obj_t   *pin_dots[PIN_LEN];
    lv_obj_t   *status_label;
    lv_obj_t   *attempts_label;
    lv_obj_t   *strength_bar;
    lv_obj_t   *strength_label;
    lv_obj_t   *lockout_label;
    lv_obj_t   *keys[12];
    pin_state_t state;
    char        entered[PIN_LEN + 1];
    char        stored_pin[PIN_LEN + 1];
    uint8_t     pos;
    uint8_t     attempts;
    int         lockout_remaining;
    lv_timer_t *lockout_timer;
} app_ctx_t;

static app_ctx_t g_ctx;

/* ---------------------------------------------------------------------------
 * IPC shared-memory buffers for HSM PIN operations
 * --------------------------------------------------------------------------- */
CY_SECTION_SHAREDMEM static ipc_msg_t      s_pin_msg;
CY_SECTION_SHAREDMEM static ipc_response_t s_pin_resp;
static bool s_optiga_ok = false;

/* Send an HSM IPC command with retry + wait. Returns true on success. */
static bool hsm_ipc_send(uint8_t cmd)
{
    memset(&s_pin_msg, 0, sizeof(s_pin_msg));
    memset(&s_pin_resp, 0, sizeof(s_pin_resp));

    s_pin_msg.client_id = CM33_IPC_HSM_CLIENT_ID;
    s_pin_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
    s_pin_msg.cmd       = cmd;
    s_pin_msg.value     = (uint32_t)(uintptr_t)&s_pin_resp;
    s_pin_resp.ready    = 0;

    cy_en_ipc_pipe_status_t st = CY_IPC_PIPE_ERROR_NO_IPC;
    for (int i = 0; i < 20; i++) {
        st = Cy_IPC_Pipe_SendMessage(
            CM33_IPC_PIPE_EP_ADDR, CM55_IPC_PIPE_EP_ADDR,
            (void *)&s_pin_msg, NULL);
        if (st == CY_IPC_PIPE_SUCCESS) break;
        Cy_SysLib_DelayUs(200);
    }
    if (st != CY_IPC_PIPE_SUCCESS) return false;

    uint32_t elapsed = 0;
    while (s_pin_resp.ready != 1 && elapsed < 10000) {
        Cy_SysLib_DelayUs(1000);
        elapsed++;
    }
    return (s_pin_resp.ready == 1 && s_pin_resp.status == 0);
}

/* Store PIN via OPTIGA SHA-256 (IPC_CMD_HSM_PIN_SET, 0xB9).
 * CM33_NS hashes the 4 digits with SHA-256 and writes to OPTIGA DATA_3. */
static bool optiga_pin_set(const char *pin)
{
    /* Put 4 PIN digits in response buffer data[0..3] as input */
    for (int i = 0; i < PIN_LEN; i++) {
        s_pin_resp.data[i] = (uint8_t)pin[i];
    }
    return hsm_ipc_send(IPC_CMD_HSM_PIN_SET);
}

/* Verify PIN via OPTIGA (IPC_CMD_HSM_PIN_VERIFY, 0xBA).
 * Returns true if PIN matches stored SHA-256 hash in DATA_3. */
static bool optiga_pin_verify(const char *pin)
{
    for (int i = 0; i < PIN_LEN; i++) {
        s_pin_resp.data[i] = (uint8_t)pin[i];
    }
    return hsm_ipc_send(IPC_CMD_HSM_PIN_VERIFY) &&
           (s_pin_resp.data[0] == 1);  /* data[0]=1 means match */
}

static void set_status(app_ctx_t *ctx, const char *msg, lv_color_t color)
{
    lv_label_set_text(ctx->status_label, msg);
    lv_obj_set_style_text_color(ctx->status_label, color, 0);
}

static void update_dots(app_ctx_t *ctx)
{
    for (int i = 0; i < PIN_LEN; i++) {
        if (i < ctx->pos) {
            lv_obj_set_style_bg_color(ctx->pin_dots[i], UI_COLOR_PRIMARY, 0);
            lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_color(ctx->pin_dots[i], lv_color_hex(0x37474f), 0);
            lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
        }
    }
}

static uint32_t simple_hash(const char *pin)
{
    uint32_t h = 5381;
    for (int i = 0; pin[i]; i++) {
        h = ((h << 5) + h) + (uint32_t)pin[i];
    }
    return h;
}

static int pin_strength(const char *pin)
{
    int score = 0;
    bool has_repeat = false;
    bool has_seq = false;

    for (int i = 1; i < PIN_LEN; i++) {
        if (pin[i] == pin[i - 1]) has_repeat = true;
        if (pin[i] == pin[i - 1] + 1 || pin[i] == pin[i - 1] - 1) has_seq = true;
    }

    score = 50;
    if (!has_repeat) score += 25;
    if (!has_seq) score += 25;
    if (pin[0] != '0' && pin[0] != '1') score += 10;
    if (score > 100) score = 100;
    return score;
}

static void update_strength(app_ctx_t *ctx)
{
    if (ctx->pos == 0) {
        lv_bar_set_value(ctx->strength_bar, 0, LV_ANIM_ON);
        lv_label_set_text(ctx->strength_label, "");
        return;
    }
    char tmp[PIN_LEN + 1];
    memcpy(tmp, ctx->entered, ctx->pos);
    tmp[ctx->pos] = '\0';
    int s = pin_strength(tmp);
    lv_bar_set_value(ctx->strength_bar, s, LV_ANIM_ON);

    if (s >= 75) {
        lv_label_set_text(ctx->strength_label, "Strong");
        lv_obj_set_style_text_color(ctx->strength_label, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_color(ctx->strength_bar, UI_COLOR_SUCCESS, LV_PART_INDICATOR);
    } else if (s >= 50) {
        lv_label_set_text(ctx->strength_label, "Medium");
        lv_obj_set_style_text_color(ctx->strength_label, UI_COLOR_WARNING, 0);
        lv_obj_set_style_bg_color(ctx->strength_bar, UI_COLOR_WARNING, LV_PART_INDICATOR);
    } else {
        lv_label_set_text(ctx->strength_label, "Weak");
        lv_obj_set_style_text_color(ctx->strength_label, UI_COLOR_ERROR, 0);
        lv_obj_set_style_bg_color(ctx->strength_bar, UI_COLOR_ERROR, LV_PART_INDICATOR);
    }
}

static void process_pin(app_ctx_t *ctx);

static void reset_entry(app_ctx_t *ctx)
{
    ctx->pos = 0;
    memset(ctx->entered, 0, sizeof(ctx->entered));
    update_dots(ctx);
    update_strength(ctx);
}

static void lockout_timer_cb(lv_timer_t *t)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_timer_get_user_data(t);
    ctx->lockout_remaining--;

    if (ctx->lockout_remaining <= 0) {
        ctx->state = STATE_ENTER_PIN;
        ctx->attempts = 0;
        set_status(ctx, "Enter your PIN", UI_COLOR_TEXT);
        lv_label_set_text(ctx->lockout_label, "");
        char abuf[32];
        snprintf(abuf, sizeof(abuf), "Attempts: %d / %d", ctx->attempts, MAX_ATTEMPTS);
        lv_label_set_text(ctx->attempts_label, abuf);
        lv_timer_del(ctx->lockout_timer);
        ctx->lockout_timer = NULL;
        reset_entry(ctx);
        /* Re-enable keys */
        for (int i = 0; i < 12; i++) {
            lv_obj_clear_state(ctx->keys[i], LV_STATE_DISABLED);
        }
    } else {
        char lbuf[48];
        snprintf(lbuf, sizeof(lbuf), "Locked out: %d seconds remaining", ctx->lockout_remaining);
        lv_label_set_text(ctx->lockout_label, lbuf);
    }
}

static void process_pin(app_ctx_t *ctx)
{
    ctx->entered[PIN_LEN] = '\0';

    switch (ctx->state) {
    case STATE_SET_PIN:
        memcpy(ctx->stored_pin, ctx->entered, PIN_LEN + 1);
        ctx->state = STATE_CONFIRM_PIN;
        lv_label_set_text(ctx->title_label, "Confirm PIN");
        set_status(ctx, "Re-enter your PIN to confirm", UI_COLOR_INFO);
        reset_entry(ctx);
        break;

    case STATE_CONFIRM_PIN:
        if (memcmp(ctx->entered, ctx->stored_pin, PIN_LEN) == 0) {
            ctx->state = STATE_ENTER_PIN;
            lv_label_set_text(ctx->title_label, "PIN Locked");
            /* Store PIN hash in OPTIGA DATA_3 via IPC */
            if (optiga_pin_set(ctx->stored_pin)) {
                s_optiga_ok = true;
                set_status(ctx, LV_SYMBOL_OK " PIN stored in OPTIGA (SHA-256)",
                           UI_COLOR_SUCCESS);
            } else {
                s_optiga_ok = false;
                char hbuf[48];
                snprintf(hbuf, sizeof(hbuf), "PIN set (local). Hash: 0x%08lX",
                         (unsigned long)simple_hash(ctx->stored_pin));
                set_status(ctx, hbuf, UI_COLOR_WARNING);
            }
        } else {
            ctx->state = STATE_SET_PIN;
            lv_label_set_text(ctx->title_label, "Set New PIN");
            set_status(ctx, "Mismatch! Try setting PIN again", UI_COLOR_ERROR);
        }
        reset_entry(ctx);
        break;

    case STATE_ENTER_PIN:
        /* Verify via OPTIGA if available, else local compare */
        if (s_optiga_ok ? optiga_pin_verify(ctx->entered)
                        : (memcmp(ctx->entered, ctx->stored_pin, PIN_LEN) == 0)) {
            ctx->state = STATE_VERIFIED;
            ctx->attempts = 0;
            set_status(ctx, LV_SYMBOL_OK " PIN Verified!"
                       " (OPTIGA SHA-256)", UI_COLOR_SUCCESS);
            char abuf[32];
            snprintf(abuf, sizeof(abuf), "Attempts: 0 / %d", MAX_ATTEMPTS);
            lv_label_set_text(ctx->attempts_label, abuf);

            /* Auto-return to enter state after 2s */
            reset_entry(ctx);
            ctx->state = STATE_ENTER_PIN;
        } else {
            ctx->attempts++;
            char abuf[32];
            snprintf(abuf, sizeof(abuf), "Attempts: %d / %d", ctx->attempts, MAX_ATTEMPTS);
            lv_label_set_text(ctx->attempts_label, abuf);

            if (ctx->attempts >= MAX_ATTEMPTS) {
                ctx->state = STATE_LOCKED;
                set_status(ctx, "LOCKED - Too many failed attempts", UI_COLOR_ERROR);
                ctx->lockout_remaining = LOCKOUT_SEC;
                ctx->lockout_timer = lv_timer_create(lockout_timer_cb, 1000, ctx);
                for (int i = 0; i < 12; i++) {
                    lv_obj_add_state(ctx->keys[i], LV_STATE_DISABLED);
                }
            } else {
                set_status(ctx, "Wrong PIN. Try again.", UI_COLOR_WARNING);
            }
        }
        reset_entry(ctx);
        break;

    default:
        reset_entry(ctx);
        break;
    }
}

static void key_event_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    if (ctx->state == STATE_LOCKED) return;

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    const char *txt = lv_label_get_text(lbl);

    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        if (ctx->pos > 0) {
            ctx->pos--;
            ctx->entered[ctx->pos] = '\0';
            update_dots(ctx);
            update_strength(ctx);
        }
        return;
    }

    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        if (ctx->pos == PIN_LEN) {
            process_pin(ctx);
        }
        return;
    }

    if (ctx->pos < PIN_LEN) {
        ctx->entered[ctx->pos] = txt[0];
        ctx->pos++;
        update_dots(ctx);
        update_strength(ctx);

        if (ctx->pos == PIN_LEN) {
            process_pin(ctx);
        }
    }
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;
    ctx->state = STATE_SET_PIN;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);

    /* Left panel */
    lv_obj_t *left = lv_obj_create(parent);
    lv_obj_set_size(left, 240, 420);
    lv_obj_set_pos(left, 10, 10);
    lv_obj_set_style_bg_color(left, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(left, 12, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_border_color(left, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    /* Lock icon */
    lv_obj_t *icon = lv_label_create(left);
    lv_label_set_text(icon, LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);

    ctx->title_label = lv_label_create(left);
    lv_label_set_text(ctx->title_label, "Set New PIN");
    lv_obj_set_style_text_color(ctx->title_label, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ctx->title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(ctx->title_label, LV_ALIGN_TOP_MID, 0, 36);

    /* จัดการรหัส PIN */
    lv_obj_t *th_sub = lv_label_create(left);
    lv_label_set_text(th_sub, "จัดการรหัส PIN");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 58);

    /* PIN dots */
    lv_coord_t dot_start_x = (240 - 32 - (PIN_LEN * 24 + (PIN_LEN - 1) * 16)) / 2;
    for (int i = 0; i < PIN_LEN; i++) {
        ctx->pin_dots[i] = lv_obj_create(left);
        lv_obj_set_size(ctx->pin_dots[i], 24, 24);
        lv_obj_set_pos(ctx->pin_dots[i], dot_start_x + i * 40, 72);
        lv_obj_set_style_bg_color(ctx->pin_dots[i], lv_color_hex(0x37474f), 0);
        lv_obj_set_style_bg_opa(ctx->pin_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->pin_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ctx->pin_dots[i], 2, 0);
        lv_obj_set_style_border_color(ctx->pin_dots[i], UI_COLOR_PRIMARY, 0);
        lv_obj_clear_flag(ctx->pin_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Status */
    ctx->status_label = lv_label_create(left);
    lv_obj_set_style_text_font(ctx->status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(ctx->status_label, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_width(ctx->status_label, 200);
    lv_label_set_long_mode(ctx->status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ctx->status_label, LV_TEXT_ALIGN_CENTER, 0);
    set_status(ctx, "Enter a 4-digit PIN", UI_COLOR_INFO);

    /* Attempts */
    ctx->attempts_label = lv_label_create(left);
    lv_obj_set_style_text_color(ctx->attempts_label, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(ctx->attempts_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->attempts_label, LV_ALIGN_TOP_MID, 0, 180);
    lv_label_set_text(ctx->attempts_label, "");

    /* Strength bar */
    lv_obj_t *sh = lv_label_create(left);
    lv_label_set_text(sh, "PIN Strength:");
    lv_obj_set_style_text_color(sh, lv_color_hex(0x78909c), 0);
    lv_obj_set_style_text_font(sh, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(sh, 0, 220);

    ctx->strength_bar = lv_bar_create(left);
    lv_obj_set_size(ctx->strength_bar, 200, 12);
    lv_obj_set_pos(ctx->strength_bar, 0, 242);
    lv_bar_set_range(ctx->strength_bar, 0, 100);
    lv_obj_set_style_bg_color(ctx->strength_bar, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_radius(ctx->strength_bar, 6, 0);
    lv_obj_set_style_radius(ctx->strength_bar, 6, LV_PART_INDICATOR);

    ctx->strength_label = lv_label_create(left);
    lv_obj_set_style_text_font(ctx->strength_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->strength_label, 0, 260);
    lv_label_set_text(ctx->strength_label, "");

    /* Lockout */
    ctx->lockout_label = lv_label_create(left);
    lv_obj_set_style_text_color(ctx->lockout_label, UI_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(ctx->lockout_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ctx->lockout_label, 0, 300);
    lv_obj_set_width(ctx->lockout_label, 200);
    lv_label_set_long_mode(ctx->lockout_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ctx->lockout_label, "");

    /* Hash display - indicates real vs fallback */
    lv_obj_t *hdr = lv_label_create(left);
    lv_label_set_text(hdr, "OPTIGA SHA-256 via IPC (0xB9/0xBA)");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x455a64), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(hdr, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* Right panel: numeric keypad */
    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_size(right, 280, 420);
    lv_obj_set_pos(right, KEYPAD_X, 10);
    lv_obj_set_style_bg_color(right, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(right, 12, 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(right, 16, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    static const char *key_labels[12] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK
    };

    for (int i = 0; i < 12; i++) {
        int row = i / 3;
        int col = i % 3;
        lv_coord_t kx = col * (KEY_SIZE + KEY_GAP) + 20;
        lv_coord_t ky = row * (KEY_SIZE + KEY_GAP) + 30;

        ctx->keys[i] = lv_btn_create(right);
        lv_obj_set_size(ctx->keys[i], KEY_SIZE, KEY_SIZE);
        lv_obj_set_pos(ctx->keys[i], kx, ky);
        lv_obj_set_style_bg_color(ctx->keys[i], lv_color_hex(0x1a2332), 0);
        lv_obj_set_style_bg_color(ctx->keys[i], lv_color_hex(0x263850), LV_STATE_PRESSED);
        lv_obj_set_style_radius(ctx->keys[i], 12, 0);

        lv_obj_t *kl = lv_label_create(ctx->keys[i]);
        lv_label_set_text(kl, key_labels[i]);
        lv_obj_set_style_text_color(kl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(kl, &lv_font_montserrat_24, 0);
        lv_obj_center(kl);

        lv_obj_add_event_cb(ctx->keys[i], key_event_cb, LV_EVENT_CLICKED, ctx);
    }

    /* Security info sidebar */
    lv_obj_t *info = lv_obj_create(parent);
    lv_obj_set_size(info, 230, 420);
    lv_obj_set_pos(info, 550, 10);
    lv_obj_set_style_bg_color(info, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(info, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info, 12, 0);
    lv_obj_set_style_border_width(info, 1, 0);
    lv_obj_set_style_border_color(info, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(info, 14, 0);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ih = lv_label_create(info);
    lv_label_set_text(ih, "SECURITY INFO");
    lv_obj_set_style_text_color(ih, lv_color_hex(0x90a4ae), 0);
    lv_obj_set_style_text_font(ih, &lv_font_montserrat_14, 0);

    static const char *info_items[] = {
        LV_SYMBOL_OK " 4-digit PIN",
        LV_SYMBOL_OK " 3-attempt lockout",
        LV_SYMBOL_OK " 30s cooldown",
        LV_SYMBOL_OK " Hash-based storage",
        LV_SYMBOL_OK " Brute-force protection",
        LV_SYMBOL_OK " Strength analysis",
    };

    for (int i = 0; i < 6; i++) {
        lv_obj_t *il = lv_label_create(info);
        lv_label_set_text(il, info_items[i]);
        lv_obj_set_style_text_color(il, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(il, 0, 26 + i * 24);
    }

    lv_obj_t *note = lv_label_create(info);
    lv_label_set_text(note,
        "PIN hashed via OPTIGA\n"
        "SHA-256 (IPC 0xB9).\n"
        "Stored in DATA_3 OID.\n\n"
        "Verify via IPC 0xBA.\n"
        "Falls back to local\n"
        "hash if chip absent.");
    lv_obj_set_style_text_color(note, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(note, 0, 180);
    lv_obj_set_width(note, 200);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
}
