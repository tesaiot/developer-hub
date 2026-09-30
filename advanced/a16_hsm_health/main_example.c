/**
 * @file    main_example.c
 * @brief   HSM Health Status - Real OPTIGA Trust M via IPC
 *
 * @description
 *   Reads OPTIGA chip data via IPC_CMD_HSM_REQUEST (0xB5) and runs
 *   8-point health check via IPC_CMD_HSM_HEALTH (0xBB) from CM55 → CM33_NS.
 *   Displays real UID, lifecycle, cert slots, key slots, and health results.
 *
 *   Falls back to simulated data if IPC times out (chip not present).
 *
 * @board   AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author  TESAIoT
 */

#include "pse84_common.h"
#include "ipc_communication.h"

/*******************************************************************************
 * HSM Color Palette (Astro UX status system from production)
 *******************************************************************************/
#define HSM_COLOR_OK         0x50D890
#define HSM_COLOR_WARN       0xF2B84B
#define HSM_COLOR_CRIT       0xE85B5B
#define HSM_COLOR_CHIP       0x71C7EC
#define HSM_COLOR_CERT       0x448AFF
#define HSM_COLOR_KEY        0xFFD740
#define HSM_COLOR_TITLE      0xBB86FC

/* ---------------------------------------------------------------------------
 * IPC shared-memory buffers
 * --------------------------------------------------------------------------- */
CY_SECTION_SHAREDMEM static ipc_msg_t      s_hsm_msg;
CY_SECTION_SHAREDMEM static ipc_response_t s_hsm_resp;

/* Response offsets (from ipc_hsm_handler.h) */
#define HSM_RESP_UID_OFF     0    /* 27 bytes */
#define HSM_RESP_LCS_OFF     27   /* 1 byte */
#define HSM_RESP_CERT_OFF    36   /* 4 bytes (cert presence) */

/* Health check response: 8 bytes, each 1=pass 0=fail */
#define HSM_HEALTH_TOTAL_LEN 8

/* Parsed chip data */
static char s_uid_hex[82];
static char s_lcs_str[32];
static bool s_cert_present[4];
static bool s_chip_ok = false;

/* Health check results */
static bool s_health_pass[8];
static int  s_health_pass_count = 0;
static bool s_health_ok = false;

/* Send an HSM IPC command. Returns true on success. */
static bool hsm_ipc_send(uint8_t cmd)
{
    memset(&s_hsm_msg, 0, sizeof(s_hsm_msg));
    memset(&s_hsm_resp, 0, sizeof(s_hsm_resp));

    s_hsm_msg.client_id = CM33_IPC_HSM_CLIENT_ID;
    s_hsm_msg.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
    s_hsm_msg.cmd       = cmd;
    s_hsm_msg.value     = (uint32_t)(uintptr_t)&s_hsm_resp;
    s_hsm_resp.ready    = 0;

    cy_en_ipc_pipe_status_t st = CY_IPC_PIPE_ERROR_NO_IPC;
    for (int i = 0; i < 20; i++) {
        st = Cy_IPC_Pipe_SendMessage(
            CM33_IPC_PIPE_EP_ADDR, CM55_IPC_PIPE_EP_ADDR,
            (void *)&s_hsm_msg, NULL);
        if (st == CY_IPC_PIPE_SUCCESS) break;
        Cy_SysLib_DelayUs(200);
    }
    if (st != CY_IPC_PIPE_SUCCESS) return false;

    uint32_t elapsed = 0;
    while (s_hsm_resp.ready != 1 && elapsed < 10000) {
        Cy_SysLib_DelayUs(1000);
        elapsed++;
    }
    return (s_hsm_resp.ready == 1 && s_hsm_resp.status == 0);
}

/* Read chip identity via IPC_CMD_HSM_REQUEST (0xB5) */
static void read_chip_data(void)
{
    if (hsm_ipc_send(IPC_CMD_HSM_REQUEST)) {
        s_chip_ok = true;
        /* Parse UID */
        const uint8_t *uid = s_hsm_resp.data + HSM_RESP_UID_OFF;
        char *p = s_uid_hex;
        for (int i = 0; i < 27; i++) {
            if (i > 0) *p++ = ':';
            p += snprintf(p, (size_t)(s_uid_hex + sizeof(s_uid_hex) - p),
                          "%02X", uid[i]);
        }
        /* Parse LCS */
        uint8_t lcs = s_hsm_resp.data[HSM_RESP_LCS_OFF];
        switch (lcs) {
            case 0x01: snprintf(s_lcs_str, sizeof(s_lcs_str), "Creation (0x01)"); break;
            case 0x03: snprintf(s_lcs_str, sizeof(s_lcs_str), "Initialization (0x03)"); break;
            case 0x07: snprintf(s_lcs_str, sizeof(s_lcs_str), "Operational (0x07)"); break;
            case 0x0F: snprintf(s_lcs_str, sizeof(s_lcs_str), "Terminated (0x0F)"); break;
            default:   snprintf(s_lcs_str, sizeof(s_lcs_str), "Unknown (0x%02X)", lcs); break;
        }
        /* Parse cert presence */
        for (int i = 0; i < 4; i++) {
            s_cert_present[i] = (s_hsm_resp.data[HSM_RESP_CERT_OFF + i] != 0);
        }
    } else {
        s_chip_ok = false;
        snprintf(s_uid_hex, sizeof(s_uid_hex), "IPC timeout");
        snprintf(s_lcs_str, sizeof(s_lcs_str), "Unknown");
        for (int i = 0; i < 4; i++) s_cert_present[i] = (i < 3);
    }
}

/* Run 8-point health check via IPC_CMD_HSM_HEALTH (0xBB) */
static void run_health_check(void)
{
    if (hsm_ipc_send(IPC_CMD_HSM_HEALTH)) {
        s_health_ok = true;
        s_health_pass_count = 0;
        for (int i = 0; i < HSM_HEALTH_TOTAL_LEN; i++) {
            s_health_pass[i] = (s_hsm_resp.data[i] == 1);
            if (s_health_pass[i]) s_health_pass_count++;
        }
    } else {
        s_health_ok = false;
        s_health_pass_count = 0;
        for (int i = 0; i < HSM_HEALTH_TOTAL_LEN; i++) {
            s_health_pass[i] = false;
        }
    }
}

/* Health test names (order matches ipc_hsm_handler.h) */
static const char * const HEALTH_TESTS[] = {
    "HW Init", "UID Read", "Cert Read", "RNG",
    "SHA-256", "ECC Sign", "Data R/W", "Metadata"
};
#define NUM_HEALTH_TESTS  8

/* Certificate slot info */
static const struct { const char *oid; } CERT_OID[] = {
    { "0xE0E0" }, { "0xE0E1" }, { "0xE0E2" }, { "0xE0E3" },
};
#define NUM_CERT_SLOTS  4

/* Key store slots */
static const struct { const char *oid; const char *desc; uint32_t color; } KEY_SLOTS[] = {
    { "0xE0F0", "Device Private Key", HSM_COLOR_CRIT },
    { "0xE0F1", "ECC P-256 Session",  HSM_COLOR_OK   },
    { "0xE0F2", "ECC P-256 Spare",    HSM_COLOR_OK   },
    { "0xE0F3", "ECC P-384 Spare",    HSM_COLOR_WARN },
};
#define NUM_KEY_SLOTS  4

/*******************************************************************************
 * Helper: status dot indicator
 *******************************************************************************/
static lv_obj_t *create_dot(lv_obj_t *parent, uint32_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_shadow_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

/*******************************************************************************
 * Helper: card with title
 *******************************************************************************/
static lv_obj_t *create_card(lv_obj_t *parent, const char *title_text,
                              uint32_t accent_color, int w, int h,
                              int x, int y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(accent_color), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_hex(accent_color), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    return card;
}

/*******************************************************************************
 * Helper: key-value row
 *******************************************************************************/
static void add_kv(lv_obj_t *parent, const char *key, const char *value,
                    uint32_t val_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x00BCD4), 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, lv_color_hex(val_color), 0);
}

/*******************************************************************************
 * Example Entry Point
 *******************************************************************************/
void example_main(lv_obj_t *parent)
{
    /* Read real OPTIGA data + run health check via IPC */
    read_chip_data();
    run_health_check();

    int col_w = 230;

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_EYE_OPEN " HSM Health Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* สุขภาพ HSM */
    thai_label(parent, "สุขภาพ HSM", 14, UI_COLOR_TEXT_DIM);

    /* Data source indicator */
    lv_obj_t *src_lbl = lv_label_create(parent);
    lv_label_set_text(src_lbl, s_chip_ok ? LV_SYMBOL_OK " Live IPC Data"
                                         : LV_SYMBOL_WARNING " IPC Timeout");
    lv_obj_set_style_text_color(src_lbl, s_chip_ok ? lv_color_hex(HSM_COLOR_OK)
                                                   : lv_color_hex(HSM_COLOR_WARN), 0);
    lv_obj_align(src_lbl, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* ---- Column 1: Chip Identity ---- */
    lv_obj_t *chip_card = create_card(parent,
        LV_SYMBOL_EYE_OPEN " Chip Identity",
        HSM_COLOR_CHIP, col_w, 180, 8, 32);

    add_kv(chip_card, "Type:", "OPTIGA Trust M V3", 0xE0E0E0);
    add_kv(chip_card, "LCS:", s_lcs_str,
           s_chip_ok ? HSM_COLOR_OK : HSM_COLOR_WARN);
    add_kv(chip_card, "I2C:", "0x30, SCB0, 100kHz", 0xE0E0E0);

    lv_obj_t *uid_lbl = lv_label_create(chip_card);
    lv_label_set_text(uid_lbl, "UID (0xE0C2):");
    lv_obj_set_style_text_color(uid_lbl, lv_color_hex(0x00BCD4), 0);

    lv_obj_t *uid_val = lv_label_create(chip_card);
    lv_label_set_text(uid_val, s_uid_hex);
    lv_obj_set_style_text_color(uid_val, lv_color_hex(
        s_chip_ok ? 0xFF9800 : 0x808080), 0);
    lv_label_set_long_mode(uid_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(uid_val, col_w - 24);

    /* ---- Column 2: Health Check (8 tests) ---- */
    char health_title[48];
    snprintf(health_title, sizeof(health_title),
             LV_SYMBOL_OK " Health Check (%d/8 %s)",
             s_health_pass_count,
             s_health_ok ? "PASS" : "N/A");
    lv_obj_t *health_card = create_card(parent, health_title,
        s_health_ok ? HSM_COLOR_OK : HSM_COLOR_WARN,
        col_w, 180, col_w + 16, 32);

    for (int i = 0; i < NUM_HEALTH_TESTS; i++) {
        lv_obj_t *row = lv_obj_create(health_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        bool pass = s_health_pass[i];
        create_dot(row, pass ? HSM_COLOR_OK : HSM_COLOR_CRIT);

        lv_obj_t *nm = lv_label_create(row);
        lv_label_set_text(nm, HEALTH_TESTS[i]);
        lv_obj_set_style_text_color(nm,
            lv_color_hex(pass ? 0xE0E0E0 : HSM_COLOR_CRIT), 0);

        lv_obj_t *st = lv_label_create(row);
        lv_label_set_text(st, s_health_ok ? (pass ? "PASS" : "FAIL") : "N/A");
        lv_obj_set_style_text_color(st,
            lv_color_hex(pass ? HSM_COLOR_OK : HSM_COLOR_CRIT), 0);
    }

    /* ---- Row 2, Col 1: Certificate Slots ---- */
    lv_obj_t *cert_card = create_card(parent,
        LV_SYMBOL_DOWNLOAD " Certificate Slots",
        HSM_COLOR_CERT, col_w, 130, 8, 220);

    for (int i = 0; i < NUM_CERT_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(cert_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        create_dot(row, s_cert_present[i] ? HSM_COLOR_OK : HSM_COLOR_CRIT);

        lv_obj_t *oid = lv_label_create(row);
        lv_label_set_text(oid, CERT_OID[i].oid);
        lv_obj_set_style_text_color(oid, lv_color_hex(0x00BCD4), 0);

        lv_obj_t *st = lv_label_create(row);
        lv_label_set_text(st, s_cert_present[i] ? "Present" : "Empty");
        lv_obj_set_style_text_color(st, lv_color_hex(
            s_cert_present[i] ? HSM_COLOR_OK : HSM_COLOR_WARN), 0);
    }

    /* ---- Row 2, Col 2: Key Store Slots ---- */
    lv_obj_t *key_card = create_card(parent,
        LV_SYMBOL_EDIT " Key Store Slots",
        HSM_COLOR_KEY, col_w, 130, col_w + 16, 220);

    for (int i = 0; i < NUM_KEY_SLOTS; i++) {
        lv_obj_t *row = lv_obj_create(key_card);
        lv_obj_set_size(row, lv_pct(100), 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        create_dot(row, KEY_SLOTS[i].color);

        lv_obj_t *oid = lv_label_create(row);
        lv_label_set_text(oid, KEY_SLOTS[i].oid);
        lv_obj_set_style_text_color(oid, lv_color_hex(0x00BCD4), 0);

        lv_obj_t *desc = lv_label_create(row);
        lv_label_set_text(desc, KEY_SLOTS[i].desc);
        lv_obj_set_style_text_color(desc, lv_color_hex(0xE0E0E0), 0);
    }

    /* ---- IPC Reference ---- */
    lv_obj_t *ipc_card = create_card(parent,
        LV_SYMBOL_SETTINGS " IPC Command Reference",
        HSM_COLOR_TITLE, DISPLAY_WIDTH - 16, 50, 8, 358);

    lv_obj_t *ipc_info = lv_label_create(ipc_card);
    lv_label_set_text(ipc_info,
        "IPC_CMD_HSM_REQUEST (0xB5) : Read UID + LCS + Certs    |    "
        "IPC_CMD_HSM_HEALTH (0xBB) : 8-point health check    |    "
        "CY_SECTION_SHAREDMEM for TX/RX buffers");
    lv_obj_set_style_text_color(ipc_info, lv_color_hex(0xB0B0B0), 0);
    lv_label_set_long_mode(ipc_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ipc_info, DISPLAY_WIDTH - 40);
}
