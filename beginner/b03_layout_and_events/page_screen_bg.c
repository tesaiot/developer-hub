/**
 * @file    page_screen_bg.c
 * @brief   Screen BG — 3 buttons that change parent background color
 *
 * Demonstrates lv_obj_set_style_bg_color on a container.
 */

#include "pages.h"

/* Store parent pointer for callbacks */
static lv_obj_t *s_bg_parent;

static void bg_dark_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_obj_set_style_bg_color(s_bg_parent, lv_color_hex(0x0D1B2A), 0);
}

static void bg_blue_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_obj_set_style_bg_color(s_bg_parent, lv_color_hex(0x1A237E), 0);
}

static void bg_green_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_obj_set_style_bg_color(s_bg_parent, lv_color_hex(0x1B5E20), 0);
}

void page_screen_bg_create(lv_obj_t *parent)
{
    /* Make parent opaque so bg color changes are visible */
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    s_bg_parent = parent;

    /* Description */
    lv_obj_t *lbl_desc = lv_label_create(parent);
    lv_label_set_text(lbl_desc, "Tap a button to change the\nbackground color of this page.");
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_desc, LV_ALIGN_TOP_MID, 0, 0);

    /* Button data */
    static const struct {
        const char  *text;
        uint32_t     color;
        lv_event_cb_t cb;
    } s_btns[] = {
        { "Dark",  0x0D1B2A, bg_dark_cb  },
        { "Blue",  0x1A237E, bg_blue_cb  },
        { "Green", 0x1B5E20, bg_green_cb },
    };

    /* Create 3 buttons in a row */
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 120, 50);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, (i - 1) * 140, 80);
        lv_obj_set_style_bg_color(btn, lv_color_hex(s_btns[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, s_btns[i].text);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, s_btns[i].cb, LV_EVENT_CLICKED, NULL);
    }

    /* Code hint */
    lv_obj_t *lbl_code = lv_label_create(parent);
    lv_label_set_text(lbl_code, "lv_obj_set_style_bg_color(parent,\n"
                                "    lv_color_hex(0x1A237E), 0);");
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_code, UI_COLOR_SUCCESS, 0);
    lv_obj_align(lbl_code, LV_ALIGN_TOP_MID, 0, 170);
}
