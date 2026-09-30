/**
 * @file    main_example.c
 * @brief   All Sensors Dashboard - BSP-guarded multi-sensor display
 *
 * Displays all available sensor readings based on BSP feature flags.
 * Reads all data via ipc_sensorhub_snapshot().
 * Adapts automatically to AI Kit or Eva Kit hardware.
 */

#include "pse84_common.h"

typedef struct {
    /* BMI270 - always available */
    lv_obj_t *lbl_accel;

#if BSP_HAS_DPS368
    lv_obj_t *lbl_temp;
    lv_obj_t *lbl_press;
#endif

#if BSP_HAS_SHT40
    lv_obj_t *lbl_humidity;
#endif

#if BSP_HAS_CAPSENSE
    lv_obj_t *lbl_capsense;
#endif

#if BSP_HAS_POTENTIOMETER
    lv_obj_t *lbl_pot;
#endif
} sensor_ctx_t;

static sensor_ctx_t ctx;

static lv_obj_t *create_sensor_card(lv_obj_t *parent, const char *title,
                                     lv_color_t color, lv_obj_t **lbl_out)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 110);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_color(card, color, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, color, 0);

    *lbl_out = lv_label_create(card);
    lv_label_set_text(*lbl_out, "Waiting...");
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(*lbl_out, UI_COLOR_TEXT, 0);

    return card;
}

static void sensor_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    /* BMI270 Accelerometer */
    if (snap.has_bmi270) {
        float ax = snap.bmi270.ax / 16384.0f;
        float ay = snap.bmi270.ay / 16384.0f;
        float az = snap.bmi270.az / 16384.0f;
        lv_label_set_text_fmt(ctx.lbl_accel,
            "X:%+.2f\nY:%+.2f\nZ:%+.2f", (double)ax, (double)ay, (double)az);
    }

#if BSP_HAS_DPS368
    if (snap.has_dps368) {
        float temp = snap.dps368.temperature_x100 / 100.0f;
        float press = snap.dps368.pressure_x100 / 100.0f;
        lv_label_set_text_fmt(ctx.lbl_temp, "%.1f \xC2\xB0""C", (double)temp);
        lv_label_set_text_fmt(ctx.lbl_press, "%.1f hPa", (double)press);
    }
#endif

#if BSP_HAS_SHT40
    if (snap.has_sht40) {
        float temp = snap.sht40.temperature_x100 / 100.0f;
        float hum = snap.sht40.humidity_x100 / 100.0f;
        lv_label_set_text_fmt(ctx.lbl_humidity, "%.1f%% RH\n%.1f \xC2\xB0""C",
                              (double)hum, (double)temp);
    }
#endif

#if BSP_HAS_CAPSENSE
    if (snap.has_capsense) {
        lv_label_set_text_fmt(ctx.lbl_capsense, "Btn0:%d Btn1:%d\nSlider:%u%%",
                              snap.capsense.btn0_pressed,
                              snap.capsense.btn1_pressed,
                              snap.capsense.slider);
    }
#endif

#if BSP_HAS_POTENTIOMETER
    if (snap.has_pot) {
        int32_t pct = snap.pot.percent_x10 / 10;
        lv_label_set_text_fmt(ctx.lbl_pot, "Raw: %u\n%"PRId32"%%",
                              snap.pot.raw, pct);
    }
#endif
}

void example_main(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = example_label_create(parent, "All Sensors Dashboard",
                                            &lv_font_montserrat_20, UI_COLOR_TEXT);
    /* ป้ายกำกับเซ็นเซอร์ */
    example_label_create(parent,
        "ป้ายกำกับเซ็นเซอร์",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Card container with flex wrap */
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 740, 300);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    /* BMI270 card - always present */
    create_sensor_card(grid, LV_SYMBOL_REFRESH " IMU (BMI270)",
                       UI_COLOR_BMI270, &ctx.lbl_accel);

#if BSP_HAS_DPS368
    create_sensor_card(grid, "Temp (DPS368)",
                       UI_COLOR_DPS368, &ctx.lbl_temp);
    create_sensor_card(grid, "Press (DPS368)",
                       lv_palette_main(LV_PALETTE_TEAL), &ctx.lbl_press);
#endif

#if BSP_HAS_SHT40
    create_sensor_card(grid, "Climate (SHT40)",
                       UI_COLOR_SHT40, &ctx.lbl_humidity);
#endif

#if BSP_HAS_CAPSENSE
    create_sensor_card(grid, "CapSense Touch",
                       lv_palette_main(LV_PALETTE_CYAN), &ctx.lbl_capsense);
#endif

#if BSP_HAS_POTENTIOMETER
    create_sensor_card(grid, "Potentiometer",
                       lv_palette_main(LV_PALETTE_PURPLE), &ctx.lbl_pot);
#endif

    /* Timer: 200ms */
    lv_timer_create(sensor_timer_cb, 200, NULL);
}
