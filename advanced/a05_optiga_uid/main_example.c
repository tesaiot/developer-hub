/**
 * @file    main_example.c
 * @brief   Security Info Display - Real OPTIGA Trust M Chip Data
 *
 * @description
 *   Reads OPTIGA Trust M chip information via IPC_CMD_HSM_REQUEST
 *   (0xB5) from CM55 → CM33_NS → OPTIGA I2C. Displays real UID,
 *   lifecycle state, certificate slot status, and supported algorithms.
 *
 *   Falls back to placeholder data if IPC times out (chip not present).
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/* ---------------------------------------------------------------------------
 * Colors
 * --------------------------------------------------------------------------- */
#define COLOR_BG                lv_color_hex(0x142240)
#define COLOR_CARD              lv_color_hex(0x0D1B2A)
#define COLOR_TEXT              lv_color_hex(0xE0E0E0)
#define COLOR_ACCENT            lv_color_hex(0x00BCD4)
#define COLOR_SUCCESS           lv_color_hex(0x4CAF50)
#define COLOR_WARNING           lv_color_hex(0xFF9800)
#define COLOR_SECTION           lv_color_hex(0x7C4DFF)

/* ---------------------------------------------------------------------------
 * IPC shared-memory buffers for HSM request
 * --------------------------------------------------------------------------- */
CY_SECTION_SHAREDMEM static ipc_msg_t      s_hsm_msg;
CY_SECTION_SHAREDMEM static ipc_response_t s_hsm_resp;

/* Response offsets (from production page_hsm.c) */
#define HSM_RESP_UID_OFF    0   /* 27 bytes */
#define HSM_RESP_LCS_OFF   27   /* 1 byte */
#define HSM_RESP_CERT_OFF  32   /* 4 bytes (cert presence flags) */

/* Parsed data buffers */
static char s_uid_hex[82];    /* 27 bytes * 3 chars + nul */
static char s_lcs_str[32];
static char s_cert_status[4][20];
static bool s_optiga_ok = false;

static const char *CHIP_NAME = "Infineon OPTIGA Trust M (SLS 32AIA)";

/* ---------------------------------------------------------------------------
 * Read real OPTIGA data via IPC_CMD_HSM_REQUEST (CM55 → CM33)
 * --------------------------------------------------------------------------- */
static void read_optiga_data(void)
{
    memset(&s_hsm_msg, 0, sizeof(s_hsm_msg));
    memset(&s_hsm_resp, 0, sizeof(s_hsm_resp));

    s_hsm_msg.client_id = CM33_IPC_HSM_CLIENT_ID;
    s_hsm_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
    s_hsm_msg.cmd       = IPC_CMD_HSM_REQUEST;
    s_hsm_msg.value     = (uint32_t)(uintptr_t)&s_hsm_resp;
    s_hsm_resp.ready    = 0;

    /* Send with retry */
    cy_en_ipc_pipe_status_t st = CY_IPC_PIPE_ERROR_NO_IPC;
    for (int i = 0; i < 20; i++) {
        st = Cy_IPC_Pipe_SendMessage(
            CM33_IPC_PIPE_EP_ADDR, CM55_IPC_PIPE_EP_ADDR,
            (void *)&s_hsm_msg, NULL);
        if (st == CY_IPC_PIPE_SUCCESS) break;
        Cy_SysLib_DelayUs(200);
    }
    if (st != CY_IPC_PIPE_SUCCESS) goto fallback;

    /* Wait for response (max 10s) */
    uint32_t elapsed = 0;
    while (s_hsm_resp.ready != 1 && elapsed < 10000) {
        Cy_SysLib_DelayUs(1000);
        elapsed++;
    }
    if (s_hsm_resp.ready != 1 || s_hsm_resp.status != 0) goto fallback;

    s_optiga_ok = true;

    /* Parse UID (27 bytes → hex string with colons) */
    {
        const uint8_t *uid = s_hsm_resp.data + HSM_RESP_UID_OFF;
        char *p = s_uid_hex;
        for (int i = 0; i < 27; i++) {
            if (i > 0) *p++ = ':';
            p += snprintf(p, (size_t)(s_uid_hex + sizeof(s_uid_hex) - p),
                          "%02X", uid[i]);
        }
    }

    /* Parse LCS */
    {
        uint8_t lcs = s_hsm_resp.data[HSM_RESP_LCS_OFF];
        switch (lcs) {
            case 0x01: snprintf(s_lcs_str, sizeof(s_lcs_str), "Creation (0x01)"); break;
            case 0x03: snprintf(s_lcs_str, sizeof(s_lcs_str), "Initialization (0x03)"); break;
            case 0x07: snprintf(s_lcs_str, sizeof(s_lcs_str), "Operational (0x07)"); break;
            case 0x0F: snprintf(s_lcs_str, sizeof(s_lcs_str), "Terminated (0x0F)"); break;
            default:   snprintf(s_lcs_str, sizeof(s_lcs_str), "Unknown (0x%02X)", lcs); break;
        }
    }

    /* Parse cert presence flags */
    for (int i = 0; i < 4; i++) {
        snprintf(s_cert_status[i], sizeof(s_cert_status[i]),
                 s_hsm_resp.data[HSM_RESP_CERT_OFF + i] ? "Present" : "Empty");
    }
    return;

fallback:
    s_optiga_ok = false;
    snprintf(s_uid_hex, sizeof(s_uid_hex),
             "IPC timeout - chip not responding");
    snprintf(s_lcs_str, sizeof(s_lcs_str), "Unknown");
    for (int i = 0; i < 4; i++) {
        snprintf(s_cert_status[i], sizeof(s_cert_status[i]), "Unknown");
    }
}

typedef struct {
    const char *slot;
    const char *type;
    const char *status;
} cert_slot_info_t;

static const cert_slot_info_t CERT_SLOTS[] = {
    { "0xE0E0", "X.509 Certificate", "Pre-provisioned" },
    { "0xE0E1", "X.509 Certificate", "Available"        },
    { "0xE0E2", "X.509 Certificate", "Available"        },
    { "0xE0E3", "X.509 Certificate", "Available"        },
};
#define NUM_CERT_SLOTS  4

typedef struct {
    const char *name;
    const char *detail;
} algo_info_t;

static const algo_info_t ALGORITHMS[] = {
    { "ECC P-256",   "ECDSA Sign/Verify, ECDH" },
    { "ECC P-384",   "ECDSA Sign/Verify, ECDH" },
    { "SHA-256",     "Hardware hash engine"     },
    { "AES-128/256", "CCM encrypt/decrypt"      },
    { "TRNG",        "True random number gen"   },
    { "RSA 1024/2048","Sign/Verify/Encrypt"     },
};
#define NUM_ALGORITHMS  6

/* ---------------------------------------------------------------------------
 * Helper: create a section card
 * --------------------------------------------------------------------------- */
static lv_obj_t *create_section(lv_obj_t *parent, const char *title_text,
                                 int w, int h, int x, int y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_border_color(card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, COLOR_SECTION, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    return card;
}

/* ---------------------------------------------------------------------------
 * Helper: create a key-value row inside a container
 * --------------------------------------------------------------------------- */
static void add_kv_row(lv_obj_t *parent, const char *key, const char *value,
                        lv_color_t val_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 8, 0);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, COLOR_ACCENT, 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, val_color, 0);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    /* Read real OPTIGA data via IPC (falls back on timeout) */
    read_optiga_data();

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " Security Info - OPTIGA Trust M");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* อ่าน UID ชิป OPTIGA */
    thai_label(parent, "อ่าน UID ชิป OPTIGA", 14, UI_COLOR_TEXT_DIM);

    /* Data source indicator */
    lv_obj_t *src_lbl = lv_label_create(parent);
    lv_label_set_text(src_lbl, s_optiga_ok ? LV_SYMBOL_OK " Live IPC Data"
                                           : LV_SYMBOL_WARNING " IPC Timeout - Fallback");
    lv_obj_set_style_text_color(src_lbl, s_optiga_ok ? COLOR_SUCCESS : COLOR_WARNING, 0);
    lv_obj_align(src_lbl, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* ---- Chip Identity Card (top-left) ---- */
    lv_obj_t *id_card = create_section(parent, LV_SYMBOL_EYE_OPEN " Chip Identity",
                                        380, 160, 8, 32);

    add_kv_row(id_card, "Chip:", CHIP_NAME, COLOR_TEXT);
    add_kv_row(id_card, "Lifecycle:", s_lcs_str,
               s_optiga_ok ? COLOR_SUCCESS : COLOR_WARNING);
    add_kv_row(id_card, "I2C Addr:", "0x30 (SCB0, 100 kHz)", COLOR_TEXT);

    lv_obj_t *uid_title = lv_label_create(id_card);
    lv_label_set_text(uid_title, "UID (0xE0C2):");
    lv_obj_set_style_text_color(uid_title, COLOR_ACCENT, 0);

    lv_obj_t *uid_val = lv_label_create(id_card);
    lv_label_set_text(uid_val, s_uid_hex);
    lv_obj_set_style_text_color(uid_val, s_optiga_ok ? COLOR_WARNING : COLOR_TEXT, 0);
    lv_label_set_long_mode(uid_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(uid_val, 350);

    /* ---- Certificate Slots Card (top-right) ---- */
    lv_obj_t *cert_card = create_section(parent, LV_SYMBOL_DOWNLOAD " Certificate Slots",
                                          370, 160, 396, 32);

    for (int i = 0; i < NUM_CERT_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(cert_card);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(row, 8, 0);

        lv_obj_t *slot = lv_label_create(row);
        lv_label_set_text(slot, CERT_SLOTS[i].slot);
        lv_obj_set_style_text_color(slot, COLOR_ACCENT, 0);

        lv_obj_t *type = lv_label_create(row);
        lv_label_set_text(type, CERT_SLOTS[i].type);
        lv_obj_set_style_text_color(type, COLOR_TEXT, 0);

        /* Use real cert status from IPC if available */
        lv_obj_t *status = lv_label_create(row);
        lv_label_set_text(status, s_optiga_ok ? s_cert_status[i]
                                              : CERT_SLOTS[i].status);
        bool present = s_optiga_ok
            ? (strcmp(s_cert_status[i], "Present") == 0)
            : (i == 0);
        lv_obj_set_style_text_color(status, present ? COLOR_SUCCESS : COLOR_WARNING, 0);
    }

    /* ---- Supported Algorithms Card (bottom) ---- */
    lv_obj_t *algo_card = create_section(parent, LV_SYMBOL_SETTINGS " Supported Algorithms",
                                          758, 170, 8, 200);
    lv_obj_set_flex_flow(algo_card, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(algo_card, 8, 0);
    lv_obj_set_style_pad_row(algo_card, 6, 0);

    /* Re-add title since flex-row messes with it - title already added by helper */

    for (int i = 0; i < NUM_ALGORITHMS; i++) {
        lv_obj_t *chip = lv_obj_create(algo_card);
        lv_obj_set_size(chip, 230, 50);
        lv_obj_set_style_bg_color(chip, COLOR_BG, 0);
        lv_obj_set_style_border_color(chip, COLOR_ACCENT, 0);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_set_style_radius(chip, 8, 0);
        lv_obj_set_style_pad_all(chip, 6, 0);
        lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *name = lv_label_create(chip);
        lv_label_set_text(name, ALGORITHMS[i].name);
        lv_obj_set_style_text_color(name, COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *detail = lv_label_create(chip);
        lv_label_set_text(detail, ALGORITHMS[i].detail);
        lv_obj_set_style_text_color(detail, COLOR_TEXT, 0);
        lv_obj_align(detail, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}
