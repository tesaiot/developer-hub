/**
 * I09 - Data Logger (Practise)
 *
 * Logs sensor readings with timestamp into a scrollable list.
 * Configurable sample rate and up to 200 entries before oldest are removed.
 *
 * Ported from Developer Hub — uses direct app_sensor drivers instead of IPC.
 * Sensors: BMI270 + DPS368 + SHT4x
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"
#include "sensor_bus.h"

#include "bmi270/bmi270_reader.h"
#include "dps368/dps368_reader.h"
#include "sht4x/sht4x_reader.h"

#define UPDATE_MS       1000
#define MAX_ENTRIES     200
#define ENTRY_HEIGHT    36

typedef struct {
    lv_obj_t *list;
    lv_obj_t *lbl_count;
    lv_obj_t *lbl_rate;
    uint32_t  entry_count;
    bool      logging;
    uint32_t  start_tick;
} logger_ctx_t;

static void add_log_entry(logger_ctx_t *ctx)
{
    /* Build timestamp from elapsed ticks */
    uint32_t elapsed_s = (lv_tick_get() - ctx->start_tick) / 1000;
    uint32_t mm = elapsed_s / 60;
    uint32_t ss = elapsed_s % 60;

    /* Build entry text */
    char buf[256];
    int len = 0;
    len += snprintf(buf + len, sizeof(buf) - len, "[%02lu:%02lu] ",
                    (unsigned long)mm, (unsigned long)ss);

    /* BMI270 */
    {
        bmi270_sample_t bmi_sample;
        if (bmi270_reader_poll(&bmi_sample)) {
            len += snprintf(buf + len, sizeof(buf) - len, "aX:%.2f ",
                            (double)bmi_sample.acc_g_x);
        }
    }

    /* DPS368 */
    {
        dps368_sample_t dps_sample;
        if (dps368_reader_poll(&dps_sample)) {
            len += snprintf(buf + len, sizeof(buf) - len, "P:%.1f ",
                            (double)dps_sample.pressure_hpa);
        }
    }

    /* SHT4x */
    {
        sht4x_sample_t sht_sample;
        if (sht4x_reader_poll(&sht_sample)) {
            len += snprintf(buf + len, sizeof(buf) - len, "T:%.1f H:%.0f%% ",
                            (double)sht_sample.temperature_c,
                            (double)sht_sample.humidity_rh);
        }
    }

    /* Remove oldest if at capacity */
    uint32_t child_count = lv_obj_get_child_count(ctx->list);
    if (child_count >= MAX_ENTRIES) {
        lv_obj_t *oldest = lv_obj_get_child(ctx->list, 0);
        if (oldest) lv_obj_del(oldest);
    }

    /* Create new entry row */
    lv_obj_t *row = lv_obj_create(ctx->list);
    lv_obj_set_size(row, 746, ENTRY_HEIGHT);
    lv_obj_set_style_bg_color(row, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    /* Entry number */
    char num_buf[8];
    snprintf(num_buf, sizeof(num_buf), "#%lu", (unsigned long)ctx->entry_count);
    lv_obj_t *lbl_num = example_label_create(row, num_buf,
                                             &lv_font_montserrat_14,
                                             UI_COLOR_PRIMARY);
    lv_obj_align(lbl_num, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *lbl_data = example_label_create(row, buf,
                                              &lv_font_montserrat_14,
                                              UI_COLOR_TEXT);
    lv_obj_align(lbl_data, LV_ALIGN_LEFT_MID, 50, 0);

    ctx->entry_count++;

    /* Auto-scroll to bottom */
    lv_obj_scroll_to_y(ctx->list, LV_COORD_MAX, LV_ANIM_ON);

    lv_label_set_text_fmt(ctx->lbl_count, "Entries: %lu",
                          (unsigned long)ctx->entry_count);
}

static void timer_cb(lv_timer_t *t)
{
    logger_ctx_t *ctx = (logger_ctx_t *)lv_timer_get_user_data(t);
    if (ctx->logging) {
        add_log_entry(ctx);
    }
}

static void btn_toggle_cb(lv_event_t *e)
{
    logger_ctx_t *ctx = (logger_ctx_t *)lv_event_get_user_data(e);
    ctx->logging = !ctx->logging;
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (ctx->logging) {
        lv_label_set_text(lbl, "Stop");
        lv_obj_set_style_bg_color(btn, UI_COLOR_ERROR, 0);
    } else {
        lv_label_set_text(lbl, "Start");
        lv_obj_set_style_bg_color(btn, UI_COLOR_SUCCESS, 0);
    }
}

static void btn_clear_cb(lv_event_t *e)
{
    logger_ctx_t *ctx = (logger_ctx_t *)lv_event_get_user_data(e);
    lv_obj_clean(ctx->list);
    ctx->entry_count = 0;
    lv_label_set_text(ctx->lbl_count, "Entries: 0");
}

void example_main(lv_obj_t *parent)
{
    static logger_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.logging = true;
    ctx.start_tick = lv_tick_get();

    tesaiot_add_thai_support_badge();

    /* Initialize sensors */
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);
    dps368_reader_init(&sensor_i2c_controller_hal_obj);
    sht4x_reader_init(&sensor_i2c_controller_hal_obj);

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 10, 0);
    lv_obj_set_style_pad_row(parent, 6, 0);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, 770, 50);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    example_label_create(hdr, "Data Logger",
                         &lv_font_montserrat_24,
                         UI_COLOR_PRIMARY);

    example_label_create(hdr,
        "บันทึกข้อมูล",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    ctx.lbl_count = example_label_create(hdr, "Entries: 0",
                                         &lv_font_montserrat_14,
                                         UI_COLOR_TEXT_DIM);

    /* Buttons */
    lv_obj_t *btn_row = lv_obj_create(parent);
    lv_obj_set_size(btn_row, 770, 44);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 8, 0);

    /* Stop button */
    lv_obj_t *btn_toggle = lv_btn_create(btn_row);
    lv_obj_set_size(btn_toggle, 90, 36);
    lv_obj_set_style_bg_color(btn_toggle, UI_COLOR_ERROR, 0);
    lv_obj_set_style_radius(btn_toggle, 8, 0);
    lv_obj_t *lbl_t = lv_label_create(btn_toggle);
    lv_label_set_text(lbl_t, "Stop");
    lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_t, lv_color_white(), 0);
    lv_obj_center(lbl_t);
    lv_obj_add_event_cb(btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, &ctx);

    /* Clear button */
    lv_obj_t *btn_clear = lv_btn_create(btn_row);
    lv_obj_set_size(btn_clear, 90, 36);
    lv_obj_set_style_bg_color(btn_clear, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_radius(btn_clear, 8, 0);
    lv_obj_t *lbl_c = lv_label_create(btn_clear);
    lv_label_set_text(lbl_c, "Clear");
    lv_obj_set_style_text_font(lbl_c, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_white(), 0);
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_clear, btn_clear_cb, LV_EVENT_CLICKED, &ctx);

    ctx.lbl_rate = example_label_create(btn_row, "1 sample/sec",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT_DIM);

    /* Scrollable log list */
    ctx.list = lv_obj_create(parent);
    lv_obj_set_size(ctx.list, 770, 340);
    lv_obj_set_flex_flow(ctx.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ctx.list, 4, 0);
    lv_obj_set_style_pad_row(ctx.list, 2, 0);
    lv_obj_set_style_bg_color(ctx.list, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(ctx.list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ctx.list, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_border_width(ctx.list, 1, 0);
    lv_obj_set_style_radius(ctx.list, 8, 0);
    lv_obj_set_scroll_dir(ctx.list, LV_DIR_VER);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
