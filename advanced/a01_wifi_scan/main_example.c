/**
 * @file    main_example.c
 * @brief   WiFi Scanner - Non-blocking scan with LVGL table display
 *
 * @description
 *   Scan WiFi access points using wifi_manager non-blocking API and display
 *   results in a sorted LVGL table. Columns: SSID, RSSI, Channel, Security.
 *   Spinner shown during scan. Max 6 APs per scan batch, sorted by RSSI.
 *
 * @board    AI Kit (KIT_PSE84_AI), Eva Kit (KIT_PSE84_EVAL_EPC2)
 * @author   TESAIoT
 */

#include "pse84_common.h"
#include "wifi_manager.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */
#define MAX_DISPLAY_APS     WIFI_MGR_SCAN_MAX_ENTRIES
#define POLL_INTERVAL_MS    200

/* RSSI thresholds for color coding */
#define RSSI_STRONG         (-50)
#define RSSI_MODERATE       (-70)

/* ---------------------------------------------------------------------------
 * Context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t        *parent;
    lv_obj_t        *table;
    lv_obj_t        *spinner;
    lv_obj_t        *status_label;
    lv_obj_t        *btn_scan;
    lv_obj_t        *btn_label;
    lv_timer_t      *poll_timer;
    wifi_mgr_scan_entry_t results[MAX_DISPLAY_APS];
    int              count;
    bool             scan_in_progress;
} wifi_scan_ctx_t;

static wifi_scan_ctx_t s_ctx;

/* ---------------------------------------------------------------------------
 * Helper: Security enum to readable string
 * --------------------------------------------------------------------------- */
static const char *security_to_str(uint8_t sec)
{
    switch (sec) {
        case 0:  return "Open";
        case 1:  return "WEP";
        case 2:  return "WPA";
        case 3:  return "WPA2";
        case 4:  return "WPA3";
        default: return "Other";
    }
}

/* ---------------------------------------------------------------------------
 * Sort results by RSSI descending (simple insertion sort)
 * --------------------------------------------------------------------------- */
static void sort_by_rssi(wifi_mgr_scan_entry_t *arr, int count)
{
    for (int i = 1; i < count; i++) {
        wifi_mgr_scan_entry_t tmp = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].rssi < tmp.rssi) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = tmp;
    }
}

/* ---------------------------------------------------------------------------
 * RSSI to signal quality description
 * --------------------------------------------------------------------------- */
static const char *rssi_quality(int8_t rssi)
{
    if (rssi >= RSSI_STRONG)   return "Strong";
    if (rssi >= RSSI_MODERATE) return "Fair";
    return "Weak";
}

/* ---------------------------------------------------------------------------
 * Populate LVGL table with scan results
 * --------------------------------------------------------------------------- */
static void populate_table(wifi_scan_ctx_t *ctx)
{
    lv_table_set_row_count(ctx->table, (uint16_t)(ctx->count + 1));
    lv_table_set_column_count(ctx->table, 4);

    lv_table_set_column_width(ctx->table, 0, 220);  /* SSID */
    lv_table_set_column_width(ctx->table, 1, 100);  /* RSSI */
    lv_table_set_column_width(ctx->table, 2, 80);   /* Channel */
    lv_table_set_column_width(ctx->table, 3, 100);  /* Security */

    /* Header row */
    lv_table_set_cell_value(ctx->table, 0, 0, "SSID");
    lv_table_set_cell_value(ctx->table, 0, 1, "RSSI (dBm)");
    lv_table_set_cell_value(ctx->table, 0, 2, "Channel");
    lv_table_set_cell_value(ctx->table, 0, 3, "Security");

    /* Data rows */
    char buf[32];
    for (int i = 0; i < ctx->count; i++) {
        uint16_t row = (uint16_t)(i + 1);

        lv_table_set_cell_value(ctx->table, row, 0, ctx->results[i].ssid);

        snprintf(buf, sizeof(buf), "%d (%s)", ctx->results[i].rssi,
                 rssi_quality(ctx->results[i].rssi));
        lv_table_set_cell_value(ctx->table, row, 1, buf);

        snprintf(buf, sizeof(buf), "%u", ctx->results[i].channel);
        lv_table_set_cell_value(ctx->table, row, 2, buf);

        lv_table_set_cell_value(ctx->table, row, 3,
                                security_to_str(ctx->results[i].security));
    }
}

/* ---------------------------------------------------------------------------
 * Poll timer: check if scan results are ready
 * --------------------------------------------------------------------------- */
static void scan_poll_cb(lv_timer_t *timer)
{
    wifi_scan_ctx_t *ctx = (wifi_scan_ctx_t *)lv_timer_get_user_data(timer);

    if (!ctx->scan_in_progress) return;

    if (!wifi_manager_scan_ready()) return;

    /* Results are ready */
    ctx->count = wifi_manager_scan_result(ctx->results, MAX_DISPLAY_APS);
    ctx->scan_in_progress = false;

    if (ctx->count < 0) {
        ctx->count = 0;
        lv_label_set_text(ctx->status_label, "Scan failed");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
    } else if (ctx->count == 0) {
        lv_label_set_text(ctx->status_label, "No networks found");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_WARNING, 0);
    } else {
        /* Sort by signal strength */
        sort_by_rssi(ctx->results, ctx->count);
        populate_table(ctx);

        char status_buf[32];
        snprintf(status_buf, sizeof(status_buf), "Found %d networks", ctx->count);
        lv_label_set_text(ctx->status_label, status_buf);
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_SUCCESS, 0);
    }

    /* Hide spinner, show button */
    lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
}

/* ---------------------------------------------------------------------------
 * Button event: trigger scan
 * --------------------------------------------------------------------------- */
static void btn_scan_cb(lv_event_t *e)
{
    wifi_scan_ctx_t *ctx = (wifi_scan_ctx_t *)lv_event_get_user_data(e);

    if (ctx->scan_in_progress) return;

    /* Start non-blocking scan */
    if (!wifi_manager_scan_start()) {
        lv_label_set_text(ctx->status_label, "Scan start failed");
        lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_ERROR, 0);
        return;
    }

    ctx->scan_in_progress = true;

    /* Show spinner, hide button */
    lv_obj_remove_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->btn_scan, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->status_label, "Scanning...");
    lv_obj_set_style_text_color(ctx->status_label, UI_COLOR_TEXT, 0);
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.parent = parent;

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_WIFI " WiFi Scanner");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* สแกน WiFi */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "สแกน WiFi");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);

    /* --- Status label --- */
    s_ctx.status_label = lv_label_create(parent);
    lv_label_set_text(s_ctx.status_label, "Press Scan to find networks");
    lv_obj_set_style_text_color(s_ctx.status_label, UI_COLOR_TEXT, 0);
    lv_obj_align(s_ctx.status_label, LV_ALIGN_TOP_LEFT, 16, 44);

    /* --- Spinner (hidden initially) --- */
    s_ctx.spinner = lv_spinner_create(parent);
    lv_spinner_set_anim_params(s_ctx.spinner, 1000, 270);
    lv_obj_set_size(s_ctx.spinner, 40, 40);
    lv_obj_align(s_ctx.spinner, LV_ALIGN_TOP_RIGHT, -16, 36);
    lv_obj_add_flag(s_ctx.spinner, LV_OBJ_FLAG_HIDDEN);

    /* --- Table --- */
    s_ctx.table = lv_table_create(parent);
    lv_obj_set_size(s_ctx.table, 520, 280);
    lv_obj_align(s_ctx.table, LV_ALIGN_TOP_MID, 0, 68);
    lv_obj_set_style_bg_color(s_ctx.table, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_text_color(s_ctx.table, UI_COLOR_TEXT, 0);
    lv_obj_set_style_border_color(s_ctx.table, UI_COLOR_INFO, 0);
    lv_obj_set_style_border_width(s_ctx.table, 1, 0);
    lv_obj_set_style_radius(s_ctx.table, 8, 0);

    /* Header style */
    lv_obj_set_style_bg_color(s_ctx.table, lv_color_hex(0x1A237E), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_ctx.table, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);

    /* Set initial column config */
    lv_table_set_column_count(s_ctx.table, 4);
    lv_table_set_row_count(s_ctx.table, 1);
    lv_table_set_column_width(s_ctx.table, 0, 220);
    lv_table_set_column_width(s_ctx.table, 1, 100);
    lv_table_set_column_width(s_ctx.table, 2, 80);
    lv_table_set_column_width(s_ctx.table, 3, 100);
    lv_table_set_cell_value(s_ctx.table, 0, 0, "SSID");
    lv_table_set_cell_value(s_ctx.table, 0, 1, "RSSI (dBm)");
    lv_table_set_cell_value(s_ctx.table, 0, 2, "Channel");
    lv_table_set_cell_value(s_ctx.table, 0, 3, "Security");

    /* --- Scan Button --- */
    s_ctx.btn_scan = lv_btn_create(parent);
    lv_obj_set_size(s_ctx.btn_scan, 160, 44);
    lv_obj_align(s_ctx.btn_scan, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(s_ctx.btn_scan, UI_COLOR_INFO, 0);
    lv_obj_set_style_radius(s_ctx.btn_scan, 8, 0);
    lv_obj_add_event_cb(s_ctx.btn_scan, btn_scan_cb, LV_EVENT_CLICKED, &s_ctx);

    s_ctx.btn_label = lv_label_create(s_ctx.btn_scan);
    lv_label_set_text(s_ctx.btn_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(s_ctx.btn_label);

    /* --- Create poll timer for non-blocking scan --- */
    s_ctx.poll_timer = lv_timer_create(scan_poll_cb, POLL_INTERVAL_MS, &s_ctx);
}
