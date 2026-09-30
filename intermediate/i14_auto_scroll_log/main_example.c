/**
 * I14 - Auto-Scrolling Console Log
 *
 * Console-style auto-scrolling log showing all sensor events.
 * Green text on dark background, monospace-feel.
 */
#include "pse84_common.h"

#define UPDATE_MS    200
#define MAX_LOG_LEN  4000

typedef struct {
    lv_obj_t *ta;
    lv_obj_t *lbl_line_count;
    uint32_t  line_count;
    bool      paused;
} log_ctx_t;

static void append_line(log_ctx_t *ctx, const char *line)
{
    if (ctx->paused) return;

    ctx->line_count++;
    char numbered[256];
    snprintf(numbered, sizeof(numbered), "%04lu> %s\n",
             (unsigned long)ctx->line_count, line);

    /* Trim if too long */
    const char *old = lv_textarea_get_text(ctx->ta);
    if (strlen(old) > MAX_LOG_LEN) {
        const char *trim = old + strlen(old) - (MAX_LOG_LEN / 2);
        /* Find next newline */
        const char *nl = strchr(trim, '\n');
        if (nl) trim = nl + 1;
        lv_textarea_set_text(ctx->ta, trim);
    }

    lv_textarea_add_text(ctx->ta, numbered);
    /* Auto-scroll to end */
    lv_textarea_set_cursor_pos(ctx->ta, LV_TEXTAREA_CURSOR_LAST);

    lv_label_set_text_fmt(ctx->lbl_line_count, "Lines: %lu",
                          (unsigned long)ctx->line_count);
}

static void timer_cb(lv_timer_t *t)
{
    log_ctx_t *ctx = (log_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    char buf[128];

#if BSP_HAS_BMI270
    {
        float ax = snap.bmi270.ax / 16384.0f;
        float ay = snap.bmi270.ay / 16384.0f;
        float az = snap.bmi270.az / 16384.0f;
        snprintf(buf, sizeof(buf), "[IMU] aX=%.2f aY=%.2f aZ=%.2f",
                 (double)ax, (double)ay, (double)az);
        append_line(ctx, buf);
    }
#endif

#if BSP_HAS_DPS368
    {
        float p = snap.dps368.pressure_x100 / 100.0f;
        float t2 = snap.dps368.temperature_x100 / 100.0f;
        snprintf(buf, sizeof(buf), "[BARO] P=%.1f hPa T=%.1f C",
                 (double)p, (double)t2);
        append_line(ctx, buf);
    }
#endif

#if BSP_HAS_SHT40
    {
        float t2 = snap.sht40.temperature_x100 / 100.0f;
        float h = snap.sht40.humidity_x100 / 100.0f;
        snprintf(buf, sizeof(buf), "[CLIM] T=%.1f C H=%.0f%%",
                 (double)t2, (double)h);
        append_line(ctx, buf);
    }
#endif

#if BSP_HAS_BMM350
    {
        float hdg = snap.bmm350.heading_x10 / 10.0f;
        snprintf(buf, sizeof(buf), "[MAG] Heading=%.1f deg", (double)hdg);
        append_line(ctx, buf);
    }
#endif

    /* WiFi status */
    bool wifi = ipc_sensorhub_wifi_connected();
    snprintf(buf, sizeof(buf), "[SYS] WiFi=%s NTP=%s",
             wifi ? "OK" : "OFF",
             ipc_sensorhub_ntp_synced() ? "SYNC" : "NO");
    append_line(ctx, buf);
}

static void btn_pause_cb(lv_event_t *e)
{
    log_ctx_t *ctx = (log_ctx_t *)lv_event_get_user_data(e);
    ctx->paused = !ctx->paused;
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, ctx->paused ? "Resume" : "Pause");
    lv_obj_set_style_bg_color(btn,
        ctx->paused ? UI_COLOR_SUCCESS : UI_COLOR_WARNING, 0);
}

static void btn_clear_cb(lv_event_t *e)
{
    log_ctx_t *ctx = (log_ctx_t *)lv_event_get_user_data(e);
    lv_textarea_set_text(ctx->ta, "");
    ctx->line_count = 0;
    lv_label_set_text(ctx->lbl_line_count, "Lines: 0");
}

void example_main(lv_obj_t *parent)
{
    static log_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_pad_row(parent, 4, 0);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 780, 44);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    example_label_create(hdr, "Sensor Console", &lv_font_montserrat_20,
                         UI_COLOR_SUCCESS);

    /* บันทึกเลื่อนอัตโนมัติ */
    example_label_create(hdr, "บันทึกเลื่อนอัตโนมัติ",
                         &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_line_count = example_label_create(hdr, "Lines: 0",
                                              &lv_font_montserrat_14,
                                              UI_COLOR_TEXT_DIM);

    /* Buttons */
    lv_obj_t *btn_row = lv_obj_create(hdr);
    lv_obj_set_size(btn_row, 200, 36);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 6, 0);

    lv_obj_t *btn_pause = lv_btn_create(btn_row);
    lv_obj_set_size(btn_pause, 80, 32);
    lv_obj_set_style_bg_color(btn_pause, UI_COLOR_WARNING, 0);
    lv_obj_set_style_radius(btn_pause, 6, 0);
    lv_obj_t *lbl_p = lv_label_create(btn_pause);
    lv_label_set_text(lbl_p, "Pause");
    lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_p, lv_color_white(), 0);
    lv_obj_center(lbl_p);
    lv_obj_add_event_cb(btn_pause, btn_pause_cb, LV_EVENT_CLICKED, &ctx);

    lv_obj_t *btn_clear = lv_btn_create(btn_row);
    lv_obj_set_size(btn_clear, 80, 32);
    lv_obj_set_style_bg_color(btn_clear, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_radius(btn_clear, 6, 0);
    lv_obj_t *lbl_c = lv_label_create(btn_clear);
    lv_label_set_text(lbl_c, "Clear");
    lv_obj_set_style_text_font(lbl_c, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_white(), 0);
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_clear, btn_clear_cb, LV_EVENT_CLICKED, &ctx);

    /* Log textarea */
    ctx.ta = lv_textarea_create(parent);
    lv_obj_set_size(ctx.ta, 780, 400);
    lv_textarea_set_text(ctx.ta, "");
    lv_textarea_set_cursor_click_pos(ctx.ta, false);
    lv_obj_set_style_bg_color(ctx.ta, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(ctx.ta, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(ctx.ta, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(ctx.ta, &lv_font_montserrat_14, 0);
    lv_obj_set_style_border_color(ctx.ta, lv_color_hex(0x1A3050), 0);
    lv_obj_set_style_border_width(ctx.ta, 1, 0);
    lv_obj_set_style_radius(ctx.ta, 4, 0);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
