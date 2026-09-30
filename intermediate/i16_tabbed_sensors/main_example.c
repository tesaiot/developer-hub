/**
 * I16 - Tabbed Sensors
 *
 * Tab view where each tab shows one sensor's detailed data with
 * large readouts and auxiliary information.
 */
#include "pse84_common.h"

#define UPDATE_MS  150

typedef struct {
    /* IMU tab */
    lv_obj_t *lbl_ax, *lbl_ay, *lbl_az;
    lv_obj_t *lbl_gx, *lbl_gy, *lbl_gz;
    /* Baro tab */
    lv_obj_t *lbl_press, *lbl_btemp, *lbl_alt;
    /* Climate tab */
    lv_obj_t *lbl_ctemp, *lbl_hum, *lbl_dewpt;
    /* Compass tab */
    lv_obj_t *lbl_hdg, *lbl_dir;
} tab_ctx_t;

static const char *heading_dir(float deg)
{
    if (deg < 22.5f  || deg >= 337.5f) return "N";
    if (deg < 67.5f)  return "NE";
    if (deg < 112.5f) return "E";
    if (deg < 157.5f) return "SE";
    if (deg < 202.5f) return "S";
    if (deg < 247.5f) return "SW";
    if (deg < 292.5f) return "W";
    return "NW";
}

static void timer_cb(lv_timer_t *t)
{
    tab_ctx_t *ctx = (tab_ctx_t *)lv_timer_get_user_data(t);
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

#if BSP_HAS_BMI270
    {
        float ax = snap.bmi270.ax / 16384.0f;
        float ay = snap.bmi270.ay / 16384.0f;
        float az = snap.bmi270.az / 16384.0f;
        float gx = snap.bmi270.gx / 16.4f;
        float gy = snap.bmi270.gy / 16.4f;
        float gz = snap.bmi270.gz / 16.4f;
        lv_label_set_text_fmt(ctx->lbl_ax, "aX: %.3f g", (double)ax);
        lv_label_set_text_fmt(ctx->lbl_ay, "aY: %.3f g", (double)ay);
        lv_label_set_text_fmt(ctx->lbl_az, "aZ: %.3f g", (double)az);
        lv_label_set_text_fmt(ctx->lbl_gx, "gX: %.1f dps", (double)gx);
        lv_label_set_text_fmt(ctx->lbl_gy, "gY: %.1f dps", (double)gy);
        lv_label_set_text_fmt(ctx->lbl_gz, "gZ: %.1f dps", (double)gz);
    }
#endif

#if BSP_HAS_DPS368
    {
        float p = snap.dps368.pressure_x100 / 100.0f;
        float t2 = snap.dps368.temperature_x100 / 100.0f;
        /* Simple altitude estimate: (1013.25 - p) * 8.3 meters */
        float alt = (1013.25f - p) * 8.3f;
        lv_label_set_text_fmt(ctx->lbl_press, "%.2f hPa", (double)p);
        lv_label_set_text_fmt(ctx->lbl_btemp, "%.1f C", (double)t2);
        lv_label_set_text_fmt(ctx->lbl_alt, "Alt: %.0f m", (double)alt);
    }
#endif

#if BSP_HAS_SHT40
    {
        float t2 = snap.sht40.temperature_x100 / 100.0f;
        float h = snap.sht40.humidity_x100 / 100.0f;
        /* Dew point approximation (Magnus formula) */
        float alpha = (17.27f * t2) / (237.7f + t2);
        float lnrh = (h > 0) ? logf(h / 100.0f) : -5.0f;
        float dp = (237.7f * (alpha + lnrh)) / (17.27f - alpha - lnrh);
        lv_label_set_text_fmt(ctx->lbl_ctemp, "%.2f C", (double)t2);
        lv_label_set_text_fmt(ctx->lbl_hum, "%.1f %%", (double)h);
        lv_label_set_text_fmt(ctx->lbl_dewpt, "Dew pt: %.1f C", (double)dp);
    }
#endif

#if BSP_HAS_BMM350
    {
        float hdg = snap.bmm350.heading_x10 / 10.0f;
        lv_label_set_text_fmt(ctx->lbl_hdg, "%.1f deg", (double)hdg);
        lv_label_set_text(ctx->lbl_dir, heading_dir(hdg));
    }
#endif
}

static void make_data_row(lv_obj_t *parent, const char *label,
                           lv_obj_t **out, const lv_font_t *font)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);

    example_label_create(row, label, &lv_font_montserrat_14,
                         UI_COLOR_TEXT_DIM);
    *out = example_label_create(row, "--", font,
                                UI_COLOR_TEXT);
}

void example_main(lv_obj_t *parent)
{
    static tab_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* เซ็นเซอร์แบบแท็บ */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "เซ็นเซอร์แบบแท็บ");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 4);

    /* Tabview */
    lv_obj_t *tv = lv_tabview_create(parent);
    lv_tabview_set_tab_bar_size(tv, 44);
    lv_obj_set_size(tv, 800, 480);
    lv_obj_set_style_bg_color(tv, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);

    /* Style tab bar */
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tv);
    lv_obj_set_style_bg_color(tab_bar, UI_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_COVER, 0);

    /* IMU tab */
#if BSP_HAS_BMI270
    {
        lv_obj_t *tab = lv_tabview_add_tab(tv, "IMU");
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(tab, 12, 0);
        lv_obj_set_style_pad_row(tab, 4, 0);

        example_label_create(tab, "BMI270 - 6-Axis IMU",
                             &lv_font_montserrat_20,
                             UI_COLOR_BMI270);
        make_data_row(tab, "Accel X:", &ctx.lbl_ax, &lv_font_montserrat_16);
        make_data_row(tab, "Accel Y:", &ctx.lbl_ay, &lv_font_montserrat_16);
        make_data_row(tab, "Accel Z:", &ctx.lbl_az, &lv_font_montserrat_16);
        make_data_row(tab, "Gyro X:", &ctx.lbl_gx, &lv_font_montserrat_14);
        make_data_row(tab, "Gyro Y:", &ctx.lbl_gy, &lv_font_montserrat_14);
        make_data_row(tab, "Gyro Z:", &ctx.lbl_gz, &lv_font_montserrat_14);
    }
#endif

    /* Baro tab */
#if BSP_HAS_DPS368
    {
        lv_obj_t *tab = lv_tabview_add_tab(tv, "Baro");
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(tab, 12, 0);
        lv_obj_set_style_pad_row(tab, 4, 0);

        example_label_create(tab, "DPS368 - Barometer",
                             &lv_font_montserrat_20,
                             UI_COLOR_DPS368);
        make_data_row(tab, "Pressure:", &ctx.lbl_press, &lv_font_montserrat_20);
        make_data_row(tab, "Temperature:", &ctx.lbl_btemp, &lv_font_montserrat_16);
        make_data_row(tab, "Altitude:", &ctx.lbl_alt, &lv_font_montserrat_16);
    }
#endif

    /* Climate tab */
#if BSP_HAS_SHT40
    {
        lv_obj_t *tab = lv_tabview_add_tab(tv, "Climate");
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(tab, 12, 0);
        lv_obj_set_style_pad_row(tab, 4, 0);

        example_label_create(tab, "SHT40 - Temp & Humidity",
                             &lv_font_montserrat_20,
                             UI_COLOR_SHT40);
        make_data_row(tab, "Temperature:", &ctx.lbl_ctemp, &lv_font_montserrat_20);
        make_data_row(tab, "Humidity:", &ctx.lbl_hum, &lv_font_montserrat_20);
        make_data_row(tab, "Dew Point:", &ctx.lbl_dewpt, &lv_font_montserrat_16);
    }
#endif

    /* Compass tab */
#if BSP_HAS_BMM350
    {
        lv_obj_t *tab = lv_tabview_add_tab(tv, "Compass");
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(tab, 20, 0);
        lv_obj_set_style_pad_row(tab, 12, 0);

        example_label_create(tab, "BMM350 - Magnetometer",
                             &lv_font_montserrat_20,
                             UI_COLOR_BMM350);
        ctx.lbl_hdg = example_label_create(tab, "--",
                                           &lv_font_montserrat_28,
                                           UI_COLOR_TEXT);
        ctx.lbl_dir = example_label_create(tab, "--",
                                           &lv_font_montserrat_24,
                                           UI_COLOR_BMM350);
    }
#endif

    lv_timer_create(timer_cb, UPDATE_MS, &ctx);
}
