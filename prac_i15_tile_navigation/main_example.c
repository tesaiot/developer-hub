/**
 * I15 - Tile Navigation
 *
 * Multi-page tile view with swipe navigation between 4 pages.
 * Each page shows different content (sensors, chart, status, about).
 */
#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

#define UPDATE_MS  200

typedef struct {
    lv_obj_t *tileview;
    lv_obj_t *lbl_page;
    /* Page 1: Sensors */
    lv_obj_t *lbl_imu, *lbl_env;
    /* Page 2: Chart */
    lv_obj_t *chart;
    lv_chart_series_t *ser;
} tile_ctx_t;

static void page_changed_cb(lv_event_t *e)
{
    tile_ctx_t *ctx = (tile_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *active = lv_tileview_get_tile_active(ctx->tileview);

    int idx = 0;
    for (int i = 0; i < 4; i++) {
        if (lv_obj_get_child(ctx->tileview, i) == active) {
            idx = i;
            break;
        }
    }

    const char *names[] = {"Sensors", "Chart", "Status", "About"};
    lv_label_set_text_fmt(ctx->lbl_page, "Page %d/4: %s", idx + 1, names[idx]);
}

/* Demo tick counter for placeholder data */
static uint32_t s_demo_tick;

static void timer_cb(lv_timer_t *t)
{
    tile_ctx_t *ctx = (tile_ctx_t *)lv_timer_get_user_data(t);
    s_demo_tick++;

    /* TODO: Replace with direct sensor reads via app_sensor */
    /* Original code used ipc_sensorhub_snapshot(&snap) for live sensor data.
     * Using placeholder/demo values so the UI still works without IPC. */

    /* Page 1: Sensors - demo IMU values */
    {
        float ax = 0.01f * (s_demo_tick % 100);
        float ay = -0.5f + 0.02f * (s_demo_tick % 50);
        float az = 9.78f + 0.01f * (s_demo_tick % 20);
        lv_label_set_text_fmt(ctx->lbl_imu,
            "IMU: aX=%.2f  aY=%.2f  aZ=%.2f g", (double)ax, (double)ay, (double)az);

        /* Page 2: Chart */
        lv_chart_set_next_value(ctx->chart, ctx->ser, (int32_t)(ax * 100));
    }

    /* Page 1: Sensors - demo environment values */
    {
        char buf[64] = "Env: ";
        float p = 1013.25f + 0.1f * (s_demo_tick % 30);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "P=%.0f hPa ", (double)p);
        float t2 = 25.0f + 0.1f * (s_demo_tick % 10);
        float h = 55.0f + 0.2f * (s_demo_tick % 20);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "T=%.1f C H=%.0f%%", (double)t2, (double)h);
        lv_label_set_text(ctx->lbl_env, buf);
    }
}

static void make_page_sensors(lv_obj_t *tile, tile_ctx_t *ctx)
{
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tile, 20, 0);
    lv_obj_set_style_pad_row(tile, 12, 0);

    example_label_create(tile, "Live Sensors", &lv_font_montserrat_24,
                         UI_COLOR_BMI270);

    ctx->lbl_imu = example_label_create(tile, "IMU: --",
                                        &lv_font_montserrat_16,
                                        UI_COLOR_TEXT);
    ctx->lbl_env = example_label_create(tile, "Env: --",
                                        &lv_font_montserrat_16,
                                        UI_COLOR_TEXT);

    example_label_create(tile, "Swipe left for chart >>",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
}

static void make_page_chart(lv_obj_t *tile, tile_ctx_t *ctx)
{
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tile, 16, 0);

    example_label_create(tile, "Accel X Chart", &lv_font_montserrat_20,
                         UI_COLOR_PRIMARY);

    ctx->chart = lv_chart_create(tile);
    lv_obj_set_size(ctx->chart, 720, 340);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, 80);
    lv_chart_set_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, -150, 150);
    lv_obj_set_style_bg_color(ctx->chart, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(ctx->chart, LV_OPA_COVER, 0);
    lv_obj_set_style_size(ctx->chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(ctx->chart, 2, LV_PART_ITEMS);

    ctx->ser = lv_chart_add_series(ctx->chart,
                                   UI_COLOR_ERROR,
                                   LV_CHART_AXIS_PRIMARY_Y);
}

static void make_page_status(lv_obj_t *tile)
{
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tile, 20, 0);
    lv_obj_set_style_pad_row(tile, 10, 0);

    example_label_create(tile, "System Status", &lv_font_montserrat_24,
                         UI_COLOR_WARNING);

    /* TODO: Replace with direct sensor reads via app_sensor */
    /* Static demo sensor status */
    const char *sensors[] = {
        "BMI270: Available",
        "DPS368: Available",
        "SHT40: Available",
        "BMM350: Available",
    };
    int n = sizeof(sensors) / sizeof(sensors[0]);
    for (int i = 0; i < n; i++) {
        example_label_create(tile, sensors[i], &lv_font_montserrat_16,
                             UI_COLOR_SUCCESS);
    }
}

static void make_page_about(lv_obj_t *tile)
{
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tile, 20, 0);
    lv_obj_set_style_pad_row(tile, 8, 0);

    example_label_create(tile, "Tile Navigation", &lv_font_montserrat_24,
                         UI_COLOR_INFO);
    example_label_create(tile, "TESAIoT : : Make Anything.",
                         &lv_font_montserrat_20,
                         UI_COLOR_PRIMARY);
    example_label_create(tile, "PSoC Edge E84 Developer Hub",
                         &lv_font_montserrat_16,
                         UI_COLOR_TEXT);
    example_label_create(tile, "Swipe horizontally between 4 pages",
                         &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
}

void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    static tile_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    s_demo_tick = 0;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, 0);

    /* Page indicator */
    ctx.lbl_page = example_label_create(parent, "Page 1/4: Sensors",
                                        &lv_font_montserrat_14,
                                        UI_COLOR_TEXT_DIM);
    example_label_create(parent,
        "นำทางแบบไทล์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_set_width(ctx.lbl_page, lv_pct(100));
    lv_obj_set_style_text_align(ctx.lbl_page, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_ver(ctx.lbl_page, 4, 0);

    /* Tileview - horizontal swipe */
    ctx.tileview = lv_tileview_create(parent);
    lv_obj_set_size(ctx.tileview, 800, 440);
    lv_obj_set_style_bg_color(ctx.tileview, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(ctx.tileview, LV_OPA_COVER, 0);

    lv_obj_t *t1 = lv_tileview_add_tile(ctx.tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *t2 = lv_tileview_add_tile(ctx.tileview, 1, 0,
                                         LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *t3 = lv_tileview_add_tile(ctx.tileview, 2, 0,
                                         LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *t4 = lv_tileview_add_tile(ctx.tileview, 3, 0, LV_DIR_LEFT);

    make_page_sensors(t1, &ctx);
    make_page_chart(t2, &ctx);
    make_page_status(t3);
    make_page_about(t4);

    lv_obj_add_event_cb(ctx.tileview, page_changed_cb,
                        LV_EVENT_VALUE_CHANGED, &ctx);

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
