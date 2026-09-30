/**
 * I05 - WiFi Scanner
 *
 * Scans for nearby WiFi networks and displays SSID, RSSI signal bar,
 * channel, and security type in a scrollable list.
 */
#include "pse84_common.h"
#include "wifi_manager.h"

#define MAX_NETWORKS    20
#define SCAN_INTERVAL   10000  /* 10 seconds between scans */

typedef struct {
    lv_obj_t *list;
    lv_obj_t *lbl_status;
    lv_obj_t *btn_scan;
    bool      scanning;
} wifi_ctx_t;

static const char *security_str(uint8_t sec)
{
    switch (sec) {
        case 0: return "Open";
        case 1: return "WEP";
        case 2: return "WPA";
        case 3: return "WPA2";
        case 4: return "WPA3";
        default: return "Other";
    }
}

static int rssi_to_bars(int8_t rssi)
{
    if (rssi >= -50) return 4;
    if (rssi >= -60) return 3;
    if (rssi >= -70) return 2;
    if (rssi >= -80) return 1;
    return 0;
}

static const char *bars_str(int bars)
{
    switch (bars) {
        case 4: return LV_SYMBOL_WIFI;
        case 3: return LV_SYMBOL_WIFI;
        case 2: return LV_SYMBOL_WIFI;
        case 1: return LV_SYMBOL_WIFI;
        default: return LV_SYMBOL_CLOSE;
    }
}

static void populate_list(wifi_ctx_t *ctx)
{
    wifi_mgr_scan_entry_t entries[MAX_NETWORKS];
    int count = wifi_manager_scan_result(entries, MAX_NETWORKS);

    /* Clear list */
    lv_obj_clean(ctx->list);

    if (count <= 0) {
        lv_label_set_text(ctx->lbl_status, "No networks found");
        return;
    }

    lv_label_set_text_fmt(ctx->lbl_status, "Found %d networks", count);

    for (int i = 0; i < count; i++) {
        /* Create row card */
        lv_obj_t *row = lv_obj_create(ctx->list);
        lv_obj_set_size(row, 740, 50);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(row, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_hor(row, 12, 0);

        /* Signal icon */
        int bars = rssi_to_bars(entries[i].rssi);
        lv_color_t sig_color = bars >= 3 ? UI_COLOR_SUCCESS
                             : bars >= 2 ? UI_COLOR_WARNING
                             :             UI_COLOR_ERROR;
        example_label_create(row, bars_str(bars), &lv_font_montserrat_16,
                             sig_color);

        /* SSID */
        lv_obj_t *lbl_ssid = example_label_create(row, entries[i].ssid,
                                                   &lv_font_montserrat_16,
                                                   UI_COLOR_TEXT);
        lv_obj_set_flex_grow(lbl_ssid, 1);

        /* RSSI bar (visual) */
        lv_obj_t *bar = lv_bar_create(row);
        lv_obj_set_size(bar, 80, 14);
        lv_bar_set_range(bar, 0, 100);
        /* Map -90..-30 to 0..100 */
        int pct = (entries[i].rssi + 90) * 100 / 60;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        lv_bar_set_value(bar, pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x1A3050), 0);
        lv_obj_set_style_bg_color(bar, sig_color, LV_PART_INDICATOR);

        /* RSSI dBm */
        char rssi_buf[16];
        snprintf(rssi_buf, sizeof(rssi_buf), "%d dBm", entries[i].rssi);
        example_label_create(row, rssi_buf, &lv_font_montserrat_14,
                             UI_COLOR_TEXT_DIM);

        /* Channel */
        char ch_buf[8];
        snprintf(ch_buf, sizeof(ch_buf), "Ch %d", entries[i].channel);
        example_label_create(row, ch_buf, &lv_font_montserrat_14,
                             UI_COLOR_TEXT_DIM);

        /* Security */
        example_label_create(row, security_str(entries[i].security),
                             &lv_font_montserrat_14,
                             UI_COLOR_INFO);
    }
}

static void timer_cb(lv_timer_t *t)
{
    wifi_ctx_t *ctx = (wifi_ctx_t *)lv_timer_get_user_data(t);

    if (ctx->scanning) {
        if (wifi_manager_scan_ready()) {
            ctx->scanning = false;
            populate_list(ctx);
            lv_label_set_text(ctx->lbl_status, "Scan complete - tap Scan to refresh");
        }
    }
}

static void btn_scan_cb(lv_event_t *e)
{
    wifi_ctx_t *ctx = (wifi_ctx_t *)lv_event_get_user_data(e);
    if (!ctx->scanning) {
        ctx->scanning = true;
        wifi_manager_scan_start();
        lv_label_set_text(ctx->lbl_status, "Scanning...");
    }
}

void example_main(lv_obj_t *parent)
{
    static wifi_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 8, 0);

    /* Header row */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 770, 50);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    example_label_create(hdr, "WiFi Scanner",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);

    ctx.btn_scan = lv_btn_create(hdr);
    lv_obj_set_size(ctx.btn_scan, 100, 36);
    lv_obj_set_style_bg_color(ctx.btn_scan, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(ctx.btn_scan, 8, 0);
    lv_obj_t *btn_lbl = lv_label_create(ctx.btn_scan);
    lv_label_set_text(btn_lbl, "Scan");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_center(btn_lbl);
    lv_obj_add_event_cb(ctx.btn_scan, btn_scan_cb, LV_EVENT_CLICKED, &ctx);

    ctx.lbl_status = example_label_create(parent, "Tap Scan to start",
                                          &lv_font_montserrat_14,
                                          UI_COLOR_TEXT_DIM);
    /* สแกนเครือข่าย WiFi */
    example_label_create(parent,
        "สแกนเครือข่าย WiFi",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);


    /* Scrollable list */
    ctx.list = lv_obj_create(parent);
    lv_obj_set_size(ctx.list, 770, 360);
    lv_obj_set_flex_flow(ctx.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ctx.list, 4, 0);
    lv_obj_set_style_pad_row(ctx.list, 4, 0);
    lv_obj_set_style_bg_opa(ctx.list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx.list, 0, 0);
    lv_obj_set_scroll_dir(ctx.list, LV_DIR_VER);

    lv_timer_create(timer_cb, 200, &ctx);
}
