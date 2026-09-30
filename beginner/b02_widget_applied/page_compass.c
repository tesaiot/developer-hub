/**
 * @file    page_compass.c
 * @brief   BMM350 heading in degrees + cardinal direction with rotating indicator
 */

#include "pages.h"

#if BSP_HAS_BMM350

static lv_obj_t *s_lbl_heading;
static lv_obj_t *s_lbl_cardinal;
static lv_obj_t *s_line;

/* Line points for compass needle (updated each frame) */
static lv_point_precise_t s_line_pts[2];

static const char *heading_to_cardinal(float hdg)
{
    if (hdg < 22.5f  || hdg >= 337.5f) return "N";
    if (hdg < 67.5f)  return "NE";
    if (hdg < 112.5f) return "E";
    if (hdg < 157.5f) return "SE";
    if (hdg < 202.5f) return "S";
    if (hdg < 247.5f) return "SW";
    if (hdg < 292.5f) return "W";
    return "NW";
}

static void compass_timer_cb(lv_timer_t *t)
{
    (void)t;
    sensorhub_snapshot_t snap;
    ipc_sensorhub_snapshot(&snap);

    if (!snap.has_bmm350) return;

    float heading = snap.bmm350.heading_x10 / 10.0f;

    lv_label_set_text_fmt(s_lbl_heading, "%.0f\xC2\xB0", (double)heading);
    lv_label_set_text(s_lbl_cardinal, heading_to_cardinal(heading));

    /* Update needle line: center (100,100) → endpoint on circle r=80 */
    float rad = heading * 3.14159f / 180.0f;
    s_line_pts[0].x = 100;
    s_line_pts[0].y = 100;
    s_line_pts[1].x = 100 + (lv_coord_t)(80.0f * sinf(rad));
    s_line_pts[1].y = 100 - (lv_coord_t)(80.0f * cosf(rad));
    lv_line_set_points(s_line, s_line_pts, 2);
}

void page_compass_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 15, 0);

    /* Compass circle container */
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, 200, 200);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circle, 100, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_border_color(circle, UI_COLOR_BMM350, 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    /* Needle line */
    s_line = lv_line_create(circle);
    s_line_pts[0].x = 100; s_line_pts[0].y = 100;
    s_line_pts[1].x = 100; s_line_pts[1].y = 20;
    lv_line_set_points(s_line, s_line_pts, 2);
    lv_obj_set_style_line_color(s_line, UI_COLOR_BMM350, 0);
    lv_obj_set_style_line_width(s_line, 3, 0);
    lv_obj_set_style_line_rounded(s_line, true, 0);

    /* N label on circle */
    lv_obj_t *n_lbl = lv_label_create(circle);
    lv_label_set_text(n_lbl, "N");
    lv_obj_set_style_text_font(n_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(n_lbl, UI_COLOR_ERROR, 0);
    lv_obj_align(n_lbl, LV_ALIGN_TOP_MID, 0, 2);

    /* Heading value */
    s_lbl_heading = example_label_create(parent, "--\xC2\xB0",
                                         &lv_font_montserrat_28, UI_COLOR_BMM350);

    /* Cardinal direction */
    s_lbl_cardinal = example_label_create(parent, "--",
                                          &lv_font_montserrat_24, UI_COLOR_TEXT);

    /* Sensor hint */
    example_label_create(parent, "BMM350 Magnetometer",
                         &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);

    /* 200 ms update timer */
    lv_timer_create(compass_timer_cb, 200, NULL);
}

#else /* !BSP_HAS_BMM350 */

void page_compass_create(lv_obj_t *parent)
{
    lv_obj_t *lbl = example_label_create(parent,
        "BMM350 not available on this board.",
        &lv_font_montserrat_20, UI_COLOR_ERROR);
    lv_obj_center(lbl);
}

#endif
