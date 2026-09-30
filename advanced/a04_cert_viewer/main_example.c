/**
 * A04 - Certificate Viewer
 *
 * X.509 certificate chain viewer with parsed fields,
 * expiration status, and chain validation visualization.
 */

#include "pse84_common.h"
#include <string.h>
#include <stdio.h>

#define NUM_CERTS       3
#define FIELD_GAP       20
#define TAB_H           36

typedef struct {
    const char *cn;
    const char *issuer;
    const char *serial;
    const char *not_before;
    const char *not_after;
    const char *algo;
    const char *key_bits;
    const char *usage;
    bool        valid;
    int         days_left;
} cert_info_t;

typedef struct {
    lv_obj_t    *parent;
    lv_obj_t    *tab_btns[NUM_CERTS];
    lv_obj_t    *detail_panel;
    lv_obj_t    *field_values[8];
    lv_obj_t    *validity_dot;
    lv_obj_t    *validity_label;
    lv_obj_t    *chain_dots[NUM_CERTS];
    lv_obj_t    *chain_lines[NUM_CERTS - 1];
    cert_info_t  certs[NUM_CERTS];
    int           selected;
} app_ctx_t;

static app_ctx_t g_ctx;

static void init_certs(app_ctx_t *ctx)
{
    ctx->certs[0] = (cert_info_t){
        .cn = "PSoC E84 Device 001",
        .issuer = "TESAIoT Intermediate CA",
        .serial = "7A:3F:C2:01:88:DE:00:01",
        .not_before = "2025-01-15 00:00:00 UTC",
        .not_after  = "2027-01-15 23:59:59 UTC",
        .algo = "ECDSA P-256 with SHA-256",
        .key_bits = "256",
        .usage = "Digital Signature, Key Agreement",
        .valid = true,
        .days_left = 672,
    };
    ctx->certs[1] = (cert_info_t){
        .cn = "TESAIoT Intermediate CA",
        .issuer = "TESAIoT Root CA",
        .serial = "3B:AF:11:00:00:00:00:02",
        .not_before = "2024-06-01 00:00:00 UTC",
        .not_after  = "2034-06-01 23:59:59 UTC",
        .algo = "ECDSA P-384 with SHA-384",
        .key_bits = "384",
        .usage = "Certificate Signing, CRL Signing",
        .valid = true,
        .days_left = 3000,
    };
    ctx->certs[2] = (cert_info_t){
        .cn = "TESAIoT Root CA",
        .issuer = "TESAIoT Root CA (self-signed)",
        .serial = "01:00:00:00:00:00:00:01",
        .not_before = "2024-01-01 00:00:00 UTC",
        .not_after  = "2044-01-01 23:59:59 UTC",
        .algo = "ECDSA P-384 with SHA-384",
        .key_bits = "384",
        .usage = "Certificate Signing, CRL Signing",
        .valid = true,
        .days_left = 6500,
    };
}

static void show_cert(app_ctx_t *ctx, int idx)
{
    cert_info_t *c = &ctx->certs[idx];
    ctx->selected = idx;

    /* Highlight selected tab */
    for (int i = 0; i < NUM_CERTS; i++) {
        if (i == idx) {
            lv_obj_set_style_bg_color(ctx->tab_btns[i], UI_COLOR_PRIMARY, 0);
            lv_obj_set_style_bg_opa(ctx->tab_btns[i], LV_OPA_40, 0);
        } else {
            lv_obj_set_style_bg_color(ctx->tab_btns[i], lv_color_hex(0x1a2332), 0);
            lv_obj_set_style_bg_opa(ctx->tab_btns[i], LV_OPA_COVER, 0);
        }
    }

    /* Highlight chain dot */
    for (int i = 0; i < NUM_CERTS; i++) {
        lv_color_t dotc = (i == idx) ? UI_COLOR_PRIMARY : UI_COLOR_SUCCESS;
        lv_obj_set_style_bg_color(ctx->chain_dots[i], dotc, 0);
        lv_obj_set_style_border_color(ctx->chain_dots[i],
            (i == idx) ? UI_COLOR_PRIMARY : lv_color_hex(0x2a3a5c), 0);
    }

    static const char *field_names[] = {
        "Subject CN", "Issuer", "Serial Number", "Valid From",
        "Valid Until", "Algorithm", "Key Size (bits)", "Key Usage"
    };
    const char *values[] = {
        c->cn, c->issuer, c->serial, c->not_before,
        c->not_after, c->algo, c->key_bits, c->usage
    };

    for (int i = 0; i < 8; i++) {
        lv_label_set_text(ctx->field_values[i], values[i]);
    }

    lv_obj_set_style_bg_color(ctx->validity_dot,
        c->valid ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);

    char vbuf[48];
    if (c->valid) {
        snprintf(vbuf, sizeof(vbuf), "Valid (%d days remaining)", c->days_left);
        lv_obj_set_style_text_color(ctx->validity_label, UI_COLOR_SUCCESS, 0);
    } else {
        snprintf(vbuf, sizeof(vbuf), "EXPIRED");
        lv_obj_set_style_text_color(ctx->validity_label, UI_COLOR_ERROR, 0);
    }
    lv_label_set_text(ctx->validity_label, vbuf);
}

static void tab_event_cb(lv_event_t *e)
{
    app_ctx_t *ctx = (app_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    for (int i = 0; i < NUM_CERTS; i++) {
        if (ctx->tab_btns[i] == btn) {
            show_cert(ctx, i);
            break;
        }
    }
}

void example_main(lv_obj_t *parent)
{
    app_ctx_t *ctx = &g_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->parent = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0d1117), 0);
    init_certs(ctx);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_LIST " Certificate Chain Viewer");
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 16, 10);

    /* UI Reference banner */
    lv_obj_t *ref_banner = lv_label_create(parent);
    lv_label_set_text(ref_banner, LV_SYMBOL_WARNING " UI Reference - real cert parsing via IPC_CMD_HSM_READ_CERT (0xB7)");
    lv_obj_set_style_text_color(ref_banner, lv_color_hex(0xFF9800), 0);
    lv_obj_set_pos(ref_banner, 16, 36);

    /* ดูใบรับรองดิจิทัล */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "ดูใบรับรองดิจิทัล");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(th_sub, 16, 32);

    /* Chain visualization (top right) */
    lv_obj_t *chain_panel = lv_obj_create(parent);
    lv_obj_set_size(chain_panel, 300, 50);
    lv_obj_set_pos(chain_panel, 470, 4);
    lv_obj_set_style_bg_opa(chain_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chain_panel, 0, 0);
    lv_obj_clear_flag(chain_panel, LV_OBJ_FLAG_SCROLLABLE);

    static const char *chain_names[] = {"Device", "Intermediate", "Root"};
    for (int i = 0; i < NUM_CERTS; i++) {
        lv_coord_t cx = i * 110;

        ctx->chain_dots[i] = lv_obj_create(chain_panel);
        lv_obj_set_size(ctx->chain_dots[i], 20, 20);
        lv_obj_set_pos(ctx->chain_dots[i], cx + 30, 2);
        lv_obj_set_style_bg_color(ctx->chain_dots[i], UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_opa(ctx->chain_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->chain_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ctx->chain_dots[i], 2, 0);
        lv_obj_set_style_border_color(ctx->chain_dots[i], lv_color_hex(0x2a3a5c), 0);
        lv_obj_clear_flag(ctx->chain_dots[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *cn = lv_label_create(chain_panel);
        lv_label_set_text(cn, chain_names[i]);
        lv_obj_set_style_text_color(cn, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(cn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(cn, cx + 10, 26);

        if (i < NUM_CERTS - 1) {
            ctx->chain_lines[i] = lv_obj_create(chain_panel);
            lv_obj_set_size(ctx->chain_lines[i], 80, 2);
            lv_obj_set_pos(ctx->chain_lines[i], cx + 54, 11);
            lv_obj_set_style_bg_color(ctx->chain_lines[i], UI_COLOR_SUCCESS, 0);
            lv_obj_set_style_bg_opa(ctx->chain_lines[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(ctx->chain_lines[i], 0, 0);
            lv_obj_clear_flag(ctx->chain_lines[i], LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    /* Tab buttons */
    static const char *tab_names[] = {
        "Device Cert", "Intermediate CA", "Root CA"
    };
    for (int i = 0; i < NUM_CERTS; i++) {
        ctx->tab_btns[i] = lv_btn_create(parent);
        lv_obj_set_size(ctx->tab_btns[i], 250, TAB_H);
        lv_obj_set_pos(ctx->tab_btns[i], 10 + i * 258, 56);
        lv_obj_set_style_bg_color(ctx->tab_btns[i], lv_color_hex(0x1a2332), 0);
        lv_obj_set_style_radius(ctx->tab_btns[i], 8, 0);

        lv_obj_t *tl = lv_label_create(ctx->tab_btns[i]);
        lv_label_set_text(tl, tab_names[i]);
        lv_obj_set_style_text_color(tl, UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
        lv_obj_center(tl);

        lv_obj_add_event_cb(ctx->tab_btns[i], tab_event_cb, LV_EVENT_CLICKED, ctx);
    }

    /* Detail panel */
    ctx->detail_panel = lv_obj_create(parent);
    lv_obj_set_size(ctx->detail_panel, 780, 330);
    lv_obj_set_pos(ctx->detail_panel, 10, 98);
    lv_obj_set_style_bg_color(ctx->detail_panel, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx->detail_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->detail_panel, 12, 0);
    lv_obj_set_style_border_width(ctx->detail_panel, 1, 0);
    lv_obj_set_style_border_color(ctx->detail_panel, lv_color_hex(0x2a3a5c), 0);
    lv_obj_set_style_pad_all(ctx->detail_panel, 16, 0);
    lv_obj_clear_flag(ctx->detail_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Validity status bar */
    ctx->validity_dot = lv_obj_create(ctx->detail_panel);
    lv_obj_set_size(ctx->validity_dot, 14, 14);
    lv_obj_set_pos(ctx->validity_dot, 0, 0);
    lv_obj_set_style_bg_opa(ctx->validity_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->validity_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->validity_dot, 0, 0);
    lv_obj_clear_flag(ctx->validity_dot, LV_OBJ_FLAG_SCROLLABLE);

    ctx->validity_label = lv_label_create(ctx->detail_panel);
    lv_obj_set_style_text_font(ctx->validity_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(ctx->validity_label, 22, -2);

    /* Field rows */
    static const char *field_names[] = {
        "Subject CN", "Issuer", "Serial Number", "Valid From",
        "Valid Until", "Algorithm", "Key Size (bits)", "Key Usage"
    };

    for (int i = 0; i < 8; i++) {
        lv_coord_t fy = 28 + i * 34;

        lv_obj_t *fn = lv_label_create(ctx->detail_panel);
        lv_label_set_text(fn, field_names[i]);
        lv_obj_set_style_text_color(fn, lv_color_hex(0x78909c), 0);
        lv_obj_set_style_text_font(fn, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(fn, 0, fy);

        ctx->field_values[i] = lv_label_create(ctx->detail_panel);
        lv_obj_set_style_text_color(ctx->field_values[i], UI_COLOR_TEXT, 0);
        lv_obj_set_style_text_font(ctx->field_values[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(ctx->field_values[i], 180, fy);
        lv_label_set_text(ctx->field_values[i], "---");
    }

    /* ASN.1 hex preview */
    lv_obj_t *hex_hdr = lv_label_create(ctx->detail_panel);
    lv_label_set_text(hex_hdr, "DER Hex (first 32 bytes):");
    lv_obj_set_style_text_color(hex_hdr, lv_color_hex(0x546e7a), 0);
    lv_obj_set_style_text_font(hex_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(hex_hdr, LV_ALIGN_BOTTOM_LEFT, 0, -20);

    lv_obj_t *hex_val = lv_label_create(ctx->detail_panel);
    lv_label_set_text(hex_val, "30 82 01 A4 30 82 01 4A A0 03 02 01 02 02 08 7A 3F C2 01 88 DE 00 01 30 0A 06 08 2A 86 48 CE 3D 04");
    lv_obj_set_style_text_color(hex_val, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(hex_val, &lv_font_montserrat_14, 0);
    lv_obj_align(hex_val, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Show first cert */
    show_cert(ctx, 0);
}
