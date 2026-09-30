/**
 * @file    main_example.c
 * @brief   BMM350 Compass - Production compass dial with live magnetometer
 *
 * @description
 *   Apple-style rotating compass dial using lv_scale (from production
 *   tesaiot_ui_helpers.c). Full 360° circular scale with tick marks every
 *   10°, cardinal labels (N/E/S/W), red North indicator, fixed pointer at
 *   12 o'clock. The entire dial rotates so the current heading aligns with
 *   the fixed pointer.
 *
 *   Reads BMM350 heading_x10 from ipc_sensorhub_snapshot() (CM33_NS
 *   pushes sensor data via sensor_auto_task). Displays heading, cardinal
 *   direction, magnetic field XYZ components, and field strength.
 *
 *   Falls back to slow simulated rotation if BMM350 data is unavailable.
 *
 * @board    AI Kit (KIT_PSE84_AI)
 * @author   TESAIoT
 */

#include "pse84_common.h"

/* ---------------------------------------------------------------------------
 * Layout constants
 * --------------------------------------------------------------------------- */
#define COMPASS_SIZE         280   /* Compass dial diameter (px)               */
#define UPDATE_PERIOD_MS     100   /* 10 Hz update                            */
#define COLOR_NORTH          0xE85B5B  /* Red for North indicator              */
#define COLOR_MAG_FIELD      0xE040FB  /* Purple for BMM350 accent            */

/* ---------------------------------------------------------------------------
 * Module-static context
 * --------------------------------------------------------------------------- */
typedef struct {
    lv_obj_t *compass_scale;
    lv_obj_t *heading_label;
    lv_obj_t *field_label;
    lv_obj_t *source_label;
    bool      has_data;
    int       sim_heading;
} compass_ctx_t;

static compass_ctx_t s_ctx;
static char s_field_buf[160];

/* ---------------------------------------------------------------------------
 * Set heading (from production tesaiot_compass_apple_set_heading)
 * --------------------------------------------------------------------------- */
static void compass_set_heading(int32_t heading_deg)
{
    if (!s_ctx.compass_scale) return;

    heading_deg %= 360;
    if (heading_deg < 0) heading_deg += 360;

    /* Rotate dial so heading° aligns with fixed pointer at 12 o'clock */
    lv_scale_set_rotation(s_ctx.compass_scale, 270 - heading_deg);

    /* Update heading label */
    if (s_ctx.heading_label) {
        static const char * const cardinal[] = {
            "N", "NE", "E", "SE", "S", "SW", "W", "NW"
        };
        int idx = ((heading_deg + 22) / 45) % 8;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d\xC2\xB0 %s",
                 (int)heading_deg, cardinal[idx]);
        lv_label_set_text(s_ctx.heading_label, buf);
    }
}

/* ---------------------------------------------------------------------------
 * Timer callback - 10 Hz BMM350 update
 * --------------------------------------------------------------------------- */
static void compass_timer_cb(lv_timer_t *t)
{
    (void)t;

    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (snap.has_bmm350 && snap.bmm350_changed) {
        s_ctx.has_data = true;
        int heading_deg = (int)(snap.bmm350.heading_x10 / 10);
        compass_set_heading(heading_deg);

        /* Magnetic field components (uT * 100 → uT) */
        float mx = (float)snap.bmm350.mx_x100 / 100.0f;
        float my = (float)snap.bmm350.my_x100 / 100.0f;
        float mz = (float)snap.bmm350.mz_x100 / 100.0f;
        float strength = sqrtf(mx * mx + my * my + mz * mz);

        snprintf(s_field_buf, sizeof(s_field_buf),
                 "Magnetic Field\n"
                 "X: %+7.1f uT\n"
                 "Y: %+7.1f uT\n"
                 "Z: %+7.1f uT\n"
                 "Strength: %.1f uT\n"
                 "Heading:  %d deg",
                 (double)mx, (double)my, (double)mz,
                 (double)strength, heading_deg);
        lv_label_set_text(s_ctx.field_label, s_field_buf);

        if (s_ctx.source_label) {
            lv_label_set_text(s_ctx.source_label,
                              LV_SYMBOL_OK " Live BMM350");
            lv_obj_set_style_text_color(s_ctx.source_label,
                                        UI_COLOR_SUCCESS, 0);
        }
    } else if (!s_ctx.has_data) {
        /* Simulate slow rotation when no real sensor data */
        s_ctx.sim_heading = (s_ctx.sim_heading + 1) % 360;
        compass_set_heading(s_ctx.sim_heading);

        if (s_ctx.source_label) {
            lv_label_set_text(s_ctx.source_label,
                              LV_SYMBOL_WARNING " Simulated");
            lv_obj_set_style_text_color(s_ctx.source_label,
                                        UI_COLOR_WARNING, 0);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------------- */
void example_main(lv_obj_t *parent)
{
    memset(&s_ctx, 0, sizeof(s_ctx));

    /* Background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = example_label_create(parent, "BMM350 Compass",
                                            &lv_font_montserrat_20,
                                            lv_color_hex(COLOR_MAG_FIELD));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* เข็มทิศ BMM350 */
    thai_label(parent, "เข็มทิศแม่เหล็กจาก BMM350", 14, UI_COLOR_TEXT_DIM);

    /* Data source indicator */
    s_ctx.source_label = example_label_create(parent,
        LV_SYMBOL_WARNING " Waiting...",
        &lv_font_montserrat_14, UI_COLOR_WARNING);
    lv_obj_align(s_ctx.source_label, LV_ALIGN_TOP_RIGHT, -10, 8);

    /* ================================================================
     * LEFT SIDE: Compass dial (production lv_scale approach)
     * ================================================================ */
    lv_obj_t *compass_cont = lv_obj_create(parent);
    lv_obj_set_size(compass_cont, COMPASS_SIZE, COMPASS_SIZE + 40);
    lv_obj_set_style_bg_color(compass_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(compass_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(compass_cont, 0, 0);
    lv_obj_set_style_pad_all(compass_cont, 0, 0);
    lv_obj_set_style_radius(compass_cont, UI_CARD_RADIUS, 0);
    lv_obj_clear_flag(compass_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(compass_cont, LV_ALIGN_LEFT_MID, 20, 10);

    /* lv_scale: full 360° circular compass */
    lv_obj_t *scale = lv_scale_create(compass_cont);
    lv_obj_set_size(scale, COMPASS_SIZE, COMPASS_SIZE);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_obj_set_style_bg_opa(scale, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(scale, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(scale, LV_ALIGN_TOP_MID, 0, 0);

    lv_scale_set_label_show(scale, true);
    lv_scale_set_total_tick_count(scale, 37);
    lv_scale_set_major_tick_every(scale, 3);
    lv_scale_set_range(scale, 0, 360);
    lv_scale_set_angle_range(scale, 360);
    lv_scale_set_rotation(scale, 270);  /* N at 12 o'clock */

    static const char *compass_labels[] = {
        "N", "30", "60", "E", "120", "150",
        "S", "210", "240", "W", "300", "330", "", NULL
    };
    lv_scale_set_text_src(scale, compass_labels);

    /* Major ticks: white */
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(scale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, 15, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 2, LV_PART_INDICATOR);

    /* Minor ticks: dim grey */
    lv_obj_set_style_line_color(scale, lv_color_hex(0x555555), LV_PART_ITEMS);
    lv_obj_set_style_length(scale, 8, LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, 1, LV_PART_ITEMS);

    /* Arc ring */
    lv_obj_set_style_arc_color(scale, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(scale, 2, LV_PART_MAIN);

    /* North sections: red arc 0-10° and 350-360° */
    lv_scale_section_t *n1 = lv_scale_add_section(scale);
    lv_scale_section_set_range(n1, 0, 10);
    lv_scale_section_t *n2 = lv_scale_add_section(scale);
    lv_scale_section_set_range(n2, 350, 360);

    /* Fixed red pointer at top */
    lv_obj_t *pointer = lv_label_create(compass_cont);
    lv_label_set_text(pointer, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(pointer, lv_color_hex(COLOR_NORTH), 0);
    lv_obj_set_style_text_font(pointer, &lv_font_montserrat_20, 0);
    lv_obj_align_to(pointer, scale, LV_ALIGN_OUT_TOP_MID, 0, 12);

    /* Heading label below compass */
    s_ctx.heading_label = lv_label_create(compass_cont);
    lv_label_set_text(s_ctx.heading_label, "0\xC2\xB0 N");
    lv_obj_set_style_text_color(s_ctx.heading_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_ctx.heading_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(s_ctx.heading_label, scale, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    s_ctx.compass_scale = scale;

    /* ================================================================
     * RIGHT SIDE: Magnetic field info card
     * ================================================================ */
    lv_obj_t *info_card = example_card_create(parent, 430, 300, UI_COLOR_CARD_BG);
    lv_obj_set_style_border_color(info_card, lv_color_hex(COLOR_MAG_FIELD), 0);
    lv_obj_set_style_border_width(info_card, 2, 0);
    lv_obj_align(info_card, LV_ALIGN_RIGHT_MID, -20, 10);
    lv_obj_set_flex_flow(info_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(info_card, 6, 0);

    example_label_create(info_card, LV_SYMBOL_SETTINGS " BMM350 Magnetometer",
                          &lv_font_montserrat_16, lv_color_hex(COLOR_MAG_FIELD));

    example_label_create(info_card, "Infineon BMM350, 3-axis, I3C bus (0x15)",
                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    s_ctx.field_label = lv_label_create(info_card);
    lv_label_set_text(s_ctx.field_label,
                      "Magnetic Field\n"
                      "X:    --- uT\n"
                      "Y:    --- uT\n"
                      "Z:    --- uT\n"
                      "Strength: --- uT\n"
                      "Heading:  --- deg");
    lv_obj_set_style_text_font(s_ctx.field_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ctx.field_label, UI_COLOR_TEXT, 0);

    example_label_create(info_card, "Heading = atan2(My, Mx)",
                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    example_label_create(info_card, "Dial rotates - pointer stays fixed at top",
                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    /* Start 10 Hz timer */
    lv_timer_create(compass_timer_cb, UPDATE_PERIOD_MS, NULL);
}
