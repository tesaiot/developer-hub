/*******************************************************************************
* File Name        : hw_button_menu_ui.c
*
* Description      : Physical-input menu navigation. The two QWA309 push buttons
*                    drive an LVGL menu with no touch required:
*                      SW6 (P17.5) = MOVE  (advance highlight, wraps)
*                      SW5 (P17.7) = SELECT (activate the highlighted item)
*                    Demonstrates a headless/kiosk UX pattern using the buttons
*                    verified by the button_monitor practise.
*******************************************************************************/
#include "hw_button_menu_ui.h"

#include "cy_gpio.h"
#include "cybsp.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

#define BTN_MOVE_PORT     P17_5_PORT
#define BTN_MOVE_PIN      P17_5_PIN     /* SW6 */
#define BTN_SEL_PORT      P17_7_PORT
#define BTN_SEL_PIN       P17_7_PIN     /* SW5 */
#define MENU_POLL_MS      (30U)
#define MENU_DEBOUNCE     (2U)
#define MENU_ITEMS        (4U)

static const char *s_items[MENU_ITEMS] = {
    "Dashboard", "Sensors", "Network", "Settings"
};
static lv_obj_t *s_rows[MENU_ITEMS];
static lv_obj_t *s_status;
static uint8_t   s_sel;

/* Simple active-low debounce per button. */
typedef struct { GPIO_PRT_Type *port; uint32_t pin; bool stable; bool last; uint8_t cnt; } btn_t;
static btn_t s_move = { P17_5_PORT, P17_5_PIN, false, false, 0U };
static btn_t s_selb = { P17_7_PORT, P17_7_PIN, false, false, 0U };

static void highlight(void)
{
    for (uint8_t i = 0U; i < MENU_ITEMS; i++) {
        bool on = (i == s_sel);
        lv_obj_set_style_bg_color(s_rows[i], lv_color_hex(on ? 0x2563EB : 0x111827), 0);
        lv_obj_set_style_border_color(s_rows[i], lv_color_hex(on ? 0x60A5FA : 0x263241), 0);
    }
}

/* Returns true on a fresh press edge (debounced). */
static bool btn_pressed_edge(btn_t *b)
{
    bool raw = (0U == Cy_GPIO_Read(b->port, b->pin));   /* active low */
    bool edge = false;
    if (raw == b->last) {
        if (b->cnt < MENU_DEBOUNCE) { b->cnt++; }
        if ((b->cnt >= MENU_DEBOUNCE) && (raw != b->stable)) {
            b->stable = raw;
            if (raw) { edge = true; }   /* press edge */
        }
    } else {
        b->cnt = 0U;
    }
    b->last = raw;
    return edge;
}

static void menu_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (btn_pressed_edge(&s_move)) {
        s_sel = (uint8_t)((s_sel + 1U) % MENU_ITEMS);
        highlight();
        lv_label_set_text_fmt(s_status, "MOVE -> %s", s_items[s_sel]);
    }
    if (btn_pressed_edge(&s_selb)) {
        lv_label_set_text_fmt(s_status, "SELECT: %s", s_items[s_sel]);
    }
}

void hw_button_menu_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    Cy_GPIO_Pin_FastInit(BTN_MOVE_PORT, BTN_MOVE_PIN, CY_GPIO_DM_PULLUP, 1UL, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(BTN_SEL_PORT,  BTN_SEL_PIN,  CY_GPIO_DM_PULLUP, 1UL, HSIOM_SEL_GPIO);

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 16, 0);
    lv_obj_set_style_pad_gap(screen, 10, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Hardware Button Menu");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), 0);

    lv_obj_t *hint = lv_label_create(screen);
    lv_label_set_text(hint, "SW6 = Move    SW5 = Select");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x5EEAD4), 0);

    for (uint8_t i = 0U; i < MENU_ITEMS; i++) {
        s_rows[i] = lv_obj_create(screen);
        lv_obj_set_width(s_rows[i], lv_pct(100));
        lv_obj_set_height(s_rows[i], 56);
        lv_obj_set_style_radius(s_rows[i], 6, 0);
        lv_obj_set_style_border_width(s_rows[i], 2, 0);
        lv_obj_set_style_pad_left(s_rows[i], 16, 0);
        lv_obj_set_flex_flow(s_rows[i], LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(s_rows[i], LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *lbl = lv_label_create(s_rows[i]);
        lv_label_set_text(lbl, s_items[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC), 0);
    }

    s_status = lv_label_create(screen);
    lv_label_set_text(s_status, "Ready");
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x94A3B8), 0);

    s_sel = 0U;
    highlight();

    lv_timer_t *t = lv_timer_create(menu_timer_cb, MENU_POLL_MS, NULL);
    lv_timer_ready(t);
}

/* [] END OF FILE */
