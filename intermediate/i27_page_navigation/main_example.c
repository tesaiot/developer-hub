/**
 * @file    main_example.c
 * @brief   Page Navigation - Forward/back between two pages
 *
 * Simulates pm_navigate / pm_back within a self-contained example.
 * Page A (blue) ↔ Page B (green) with navigation buttons.
 */

#include "pse84_common.h"

/* ── State ───────────────────────────────────────────────────────── */
static lv_obj_t *s_parent;      /* root parent passed to example_main */
static lv_obj_t *s_page_obj;    /* current page container */
static uint32_t  s_visit_a;
static uint32_t  s_visit_b;
static bool      s_on_page_b;

/* Forward declarations */
static void show_page_a(void);
static void show_page_b(void);

/* ── Clear current page ──────────────────────────────────────────── */
static void clear_page(void)
{
    if (s_page_obj) {
        lv_obj_delete(s_page_obj);
        s_page_obj = NULL;
    }
}

/* ── Navigation events ───────────────────────────────────────────── */
static void btn_goto_b_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    clear_page();     /* simulate destroy */
    show_page_b();    /* simulate pm_navigate(PAGE_B) */
}

static void btn_back_a_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    clear_page();     /* simulate destroy */
    show_page_a();    /* simulate pm_back() → Page A */
}

/* ── Page A (Blue) ───────────────────────────────────────────────── */
static void show_page_a(void)
{
    s_on_page_b = false;
    s_visit_a++;

    s_page_obj = lv_obj_create(s_parent);
    lv_obj_set_size(s_page_obj, 700, 340);
    lv_obj_align(s_page_obj, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(s_page_obj, lv_color_hex(0x0d1b3e), 0);
    lv_obj_set_style_bg_opa(s_page_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_page_obj, 16, 0);
    lv_obj_set_style_border_width(s_page_obj, 2, 0);
    lv_obj_set_style_border_color(s_page_obj, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_clear_flag(s_page_obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_page_obj);
    lv_label_set_text(title, "PAGE A");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *visits = lv_label_create(s_page_obj);
    lv_label_set_text_fmt(visits, "Visits to A: %u  |  Visits to B: %u",
                          (unsigned)s_visit_a, (unsigned)s_visit_b);
    lv_obj_set_style_text_font(visits, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(visits, lv_color_white(), 0);
    lv_obj_align(visits, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *btn = lv_button_create(s_page_obj);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(btn, btn_goto_b_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Go to Page B " LV_SYMBOL_RIGHT);
    lv_obj_center(btn_lbl);
}

/* ── Page B (Green) ──────────────────────────────────────────────── */
static void show_page_b(void)
{
    s_on_page_b = true;
    s_visit_b++;

    s_page_obj = lv_obj_create(s_parent);
    lv_obj_set_size(s_page_obj, 700, 340);
    lv_obj_align(s_page_obj, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(s_page_obj, lv_color_hex(0x0b2e1b), 0);
    lv_obj_set_style_bg_opa(s_page_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_page_obj, 16, 0);
    lv_obj_set_style_border_width(s_page_obj, 2, 0);
    lv_obj_set_style_border_color(s_page_obj, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_clear_flag(s_page_obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_page_obj);
    lv_label_set_text(title, "PAGE B");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *visits = lv_label_create(s_page_obj);
    lv_label_set_text_fmt(visits, "Visits to A: %u  |  Visits to B: %u",
                          (unsigned)s_visit_a, (unsigned)s_visit_b);
    lv_obj_set_style_text_font(visits, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(visits, lv_color_white(), 0);
    lv_obj_align(visits, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *btn = lv_button_create(s_page_obj);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(btn, btn_back_a_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back to Page A");
    lv_obj_center(btn_lbl);
}

/* ── Entry point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_parent    = parent;
    s_page_obj  = NULL;
    s_visit_a   = 0;
    s_visit_b   = 0;
    s_on_page_b = false;

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "I24 - Page Navigation");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    /* นำทางระหว่างหน้า */
    lv_obj_t *th_sub = example_label_create(parent,
        "นำทางระหว่างหน้า",
        &lv_font_noto_thai_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 34);


    show_page_a();
}
