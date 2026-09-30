/**
 * I19 - WiFi Status Bar
 *
 * Top status bar showing WiFi state, NTP time, and sensor activity
 * indicators. Below is a main content area with sensor summary.
 */
#include "pse84_common.h"
#include "wifi_manager.h"

#define UPDATE_MS  500

typedef struct {
    /* Status bar */
    lv_obj_t *lbl_wifi;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_rssi;
    lv_obj_t *dot_ntp;
    /* Sensor indicators */
    lv_obj_t *dot_imu, *dot_baro, *dot_clim, *dot_mag;
    /* Content area */
    lv_obj_t *lbl_content;
    uint32_t  update_count;
} bar_ctx_t;

static void timer_cb(lv_timer_t *t)
{
    bar_ctx_t *ctx = (bar_ctx_t *)lv_timer_get_user_data(t);
    ctx->update_count++;

    /* WiFi */
    bool wifi = ipc_sensorhub_wifi_connected();
    if (wifi) {
        wifi_mgr_status_t ws;
        wifi_manager_get_status(&ws);
        lv_label_set_text_fmt(ctx->lbl_wifi, "%s %s", LV_SYMBOL_WIFI, ws.ssid);
        lv_obj_set_style_text_color(ctx->lbl_wifi,
                                    UI_COLOR_SUCCESS, 0);
        lv_label_set_text_fmt(ctx->lbl_rssi, "%d dBm", ws.rssi);
    } else {
        lv_label_set_text_fmt(ctx->lbl_wifi, "%s Disconnected", LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(ctx->lbl_wifi,
                                    UI_COLOR_ERROR, 0);
        lv_label_set_text(ctx->lbl_rssi, "--");
    }

    /* NTP / Time */
    bool ntp = ipc_sensorhub_ntp_synced();
    lv_obj_set_style_bg_color(ctx->dot_ntp,
        ntp ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);

    char time_buf[32];
    ipc_sensorhub_get_time_str(time_buf, sizeof(time_buf));
    lv_label_set_text(ctx->lbl_time, time_buf);

    /* Sensor activity - blink dots */
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    bool blink = (ctx->update_count % 2) == 0;

#if BSP_HAS_BMI270
    lv_obj_set_style_bg_opa(ctx->dot_imu, blink ? LV_OPA_COVER : LV_OPA_60, 0);
#endif
#if BSP_HAS_DPS368
    lv_obj_set_style_bg_opa(ctx->dot_baro, blink ? LV_OPA_COVER : LV_OPA_60, 0);
#endif
#if BSP_HAS_SHT40
    lv_obj_set_style_bg_opa(ctx->dot_clim, blink ? LV_OPA_COVER : LV_OPA_60, 0);
#endif
#if BSP_HAS_BMM350
    lv_obj_set_style_bg_opa(ctx->dot_mag, blink ? LV_OPA_COVER : LV_OPA_60, 0);
#endif

    /* Content summary */
    char buf[256];
    int len = 0;
    len += snprintf(buf + len, sizeof(buf) - len, "Sensor Summary\n\n");
#if BSP_HAS_BMI270
    float ax = snap.bmi270.ax / 16384.0f;
    len += snprintf(buf + len, sizeof(buf) - len,
                    "IMU: aX=%.2f g\n", (double)ax);
#endif
#if BSP_HAS_DPS368
    float p = snap.dps368.pressure_x100 / 100.0f;
    len += snprintf(buf + len, sizeof(buf) - len,
                    "Baro: P=%.1f hPa\n", (double)p);
#endif
#if BSP_HAS_SHT40
    float temp = snap.sht40.temperature_x100 / 100.0f;
    float hum = snap.sht40.humidity_x100 / 100.0f;
    len += snprintf(buf + len, sizeof(buf) - len,
                    "Climate: T=%.1f C  H=%.0f%%\n", (double)temp, (double)hum);
#endif
#if BSP_HAS_BMM350
    float hdg = snap.bmm350.heading_x10 / 10.0f;
    len += snprintf(buf + len, sizeof(buf) - len,
                    "Compass: %.0f deg\n", (double)hdg);
#endif
    len += snprintf(buf + len, sizeof(buf) - len,
                    "\nUpdates: %lu", (unsigned long)ctx->update_count);
    lv_label_set_text(ctx->lbl_content, buf);
}

static lv_obj_t *make_dot(lv_obj_t *parent, lv_color_t color, const char *label)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, 50, 24);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_set_style_pad_column(c, 3, 0);

    lv_obj_t *dot = lv_obj_create(c);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);

    example_label_create(c, label, &lv_font_montserrat_14, color);

    return dot;
}

void example_main(lv_obj_t *parent)
{
    static bar_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_pad_row(parent, 0, 0);

    /* ── Status bar ──────────────────────────────────────────────── */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 800, 36);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(bar, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 12, 0);
    lv_obj_set_style_pad_ver(bar, 2, 0);

    /* Left: WiFi */
    ctx.lbl_wifi = example_label_create(bar, LV_SYMBOL_WIFI " --",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT_DIM);
    ctx.lbl_rssi = example_label_create(bar, "--",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT_DIM);

    /* Center: sensor dots */
    lv_obj_t *dots = lv_obj_create(bar);
    lv_obj_set_size(dots, 280, 28);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots, 0, 0);
    lv_obj_set_style_pad_all(dots, 0, 0);
    lv_obj_set_style_pad_column(dots, 4, 0);

#if BSP_HAS_BMI270
    ctx.dot_imu = make_dot(dots, UI_COLOR_BMI270, "IMU");
#endif
#if BSP_HAS_DPS368
    ctx.dot_baro = make_dot(dots, UI_COLOR_DPS368, "Bar");
#endif
#if BSP_HAS_SHT40
    ctx.dot_clim = make_dot(dots, UI_COLOR_SHT40, "Clm");
#endif
#if BSP_HAS_BMM350
    ctx.dot_mag = make_dot(dots, UI_COLOR_BMM350, "Mag");
#endif

    /* Right: NTP dot + time */
    ctx.dot_ntp = lv_obj_create(bar);
    lv_obj_set_size(ctx.dot_ntp, 8, 8);
    lv_obj_set_style_radius(ctx.dot_ntp, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ctx.dot_ntp, UI_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(ctx.dot_ntp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx.dot_ntp, 0, 0);

    ctx.lbl_time = example_label_create(bar, "--:--",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT);

    /* ── Content area ────────────────────────────────────────────── */
    lv_obj_t *content = example_card_create(parent, 780, 420,
                                            UI_COLOR_CARD_BG);
    lv_obj_set_style_margin_all(content, 10, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(content, 20, 0);

    /* แถบสถานะ WiFi */
    example_label_create(content, "แถบสถานะ WiFi",
                         &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_content = example_label_create(content, "Starting...",
                                           &lv_font_montserrat_20,
                                           UI_COLOR_TEXT);
    lv_obj_set_style_text_line_space(ctx.lbl_content, 6, 0);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
