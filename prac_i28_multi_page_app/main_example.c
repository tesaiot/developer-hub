/**
 * i28_multi_page_app - Production Page Navigation Pattern
 *
 * Demonstrates a self-contained page manager with navigation stack,
 * bottom tab bar, and three pages: Home, Sensors, Settings.
 * No dependency on production page_manager.h - everything is local.
 *
 * Board:  PSoC Edge E84 (AI Kit / Eva Kit)
 * Core:   CM55 (display + UI)
 */

#include "pse84_common.h"
#include "app_interface.h"
#include "tesaiot_thai.h"

/* ── Layout Constants ────────────────────────────────────────────── */
#define TAB_BAR_H       56
#define CONTENT_Y       0
#define CONTENT_H       (DISPLAY_HEIGHT - TAB_BAR_H - 40)  /* minus title area */
#define NUM_TABS        3
#define NAV_STACK_MAX   8

/* ── Page IDs ────────────────────────────────────────────────────── */
enum {
    PAGE_HOME = 0,
    PAGE_SENSORS,
    PAGE_SETTINGS,
    PAGE_COUNT
};

/* ── Page Definition ─────────────────────────────────────────────── */
typedef struct {
    const char *title;
    const char *icon;
    void (*create_fn)(lv_obj_t *content);
} page_def_t;

/* Forward declarations */
static void create_home_page(lv_obj_t *content);
static void create_sensors_page(lv_obj_t *content);
static void create_settings_page(lv_obj_t *content);

static const page_def_t s_pages[PAGE_COUNT] = {
    { "Home",     LV_SYMBOL_HOME,     create_home_page     },
    { "Sensors",  LV_SYMBOL_EYE_OPEN, create_sensors_page  },
    { "Settings", LV_SYMBOL_SETTINGS, create_settings_page },
};

/* ── Navigation State ────────────────────────────────────────────── */
static lv_obj_t *s_content_area   = NULL;
static lv_obj_t *s_tab_btns[NUM_TABS];
static lv_obj_t *s_tab_labels[NUM_TABS];
static int       s_nav_stack[NAV_STACK_MAX];
static int       s_nav_depth = 0;
static int       s_current_page = -1;

/* Sensor page labels */
static lv_obj_t *s_ax_label = NULL;
static lv_obj_t *s_ay_label = NULL;
static lv_obj_t *s_az_label = NULL;
static lv_obj_t *s_gx_label = NULL;
static lv_obj_t *s_gy_label = NULL;
static lv_obj_t *s_gz_label = NULL;
static lv_timer_t *s_sensor_timer = NULL;

/* Demo tick counter for placeholder data */
static uint32_t s_demo_tick;

/* ── Navigate To Page ────────────────────────────────────────────── */
static void navigate_to(int page_id, bool push_stack)
{
    if (page_id < 0 || page_id >= PAGE_COUNT) return;
    if (page_id == s_current_page) return;

    /* Push current page onto nav stack */
    if (push_stack && s_current_page >= 0 && s_nav_depth < NAV_STACK_MAX) {
        s_nav_stack[s_nav_depth++] = s_current_page;
    }

    /* Stop sensor timer when leaving sensors page */
    if (s_sensor_timer != NULL) {
        lv_timer_delete(s_sensor_timer);
        s_sensor_timer = NULL;
    }

    /* Clear content area */
    lv_obj_clean(s_content_area);

    /* Create new page content */
    s_pages[page_id].create_fn(s_content_area);
    s_current_page = page_id;

    /* Update tab bar highlights */
    for (int i = 0; i < NUM_TABS; i++) {
        lv_color_t bg = (i == page_id) ? UI_COLOR_PRIMARY : lv_color_hex(0x1A2A4A);
        lv_color_t txt = (i == page_id) ? lv_color_white() : UI_COLOR_TEXT_DIM;
        lv_obj_set_style_bg_color(s_tab_btns[i], bg, 0);
        lv_obj_set_style_text_color(s_tab_labels[i], txt, 0);
    }
}

static void navigate_back(void)
{
    if (s_nav_depth > 0) {
        int prev = s_nav_stack[--s_nav_depth];
        navigate_to(prev, false);
    }
}

/* ── Tab Button Event ────────────────────────────────────────────── */
static void tab_event_cb(lv_event_t *e)
{
    int page_id = (int)(intptr_t)lv_event_get_user_data(e);
    navigate_to(page_id, true);
}

/* ── Home Page ───────────────────────────────────────────────────── */
static void nav_btn_event_cb(lv_event_t *e)
{
    int page_id = (int)(intptr_t)lv_event_get_user_data(e);
    navigate_to(page_id, true);
}

static void create_home_page(lv_obj_t *content)
{
    /* Welcome card */
    lv_obj_t *card = example_card_create(content, 420, 100, UI_COLOR_CARD_BG);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *welcome = example_label_create(card, "TESAIoT Multi-Page App",
                                             &lv_font_montserrat_20, UI_COLOR_PRIMARY);
    lv_obj_align(welcome, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *desc = example_label_create(card, "Tap tabs below or buttons to navigate",
                                          &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Navigation buttons */
    for (int i = 1; i < PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(content);
        lv_obj_set_size(btn, 200, 50);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 140 + (i - 1) * 70);
        lv_obj_set_style_bg_color(btn, UI_COLOR_CARD_BG, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, UI_COLOR_PRIMARY, 0);
        lv_obj_set_style_radius(btn, UI_CARD_RADIUS, 0);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "%s  %s", s_pages[i].icon, s_pages[i].title);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT, 0);
        lv_obj_center(lbl);
    }

    /* Stack depth indicator */
    lv_obj_t *stack_info = example_label_create(content, "",
                                                &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_label_set_text_fmt(stack_info, "Nav stack depth: %d", s_nav_depth);
    lv_obj_align(stack_info, LV_ALIGN_BOTTOM_MID, 0, -8);
}

/* ── Sensors Page ────────────────────────────────────────────────── */
static void sensor_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_demo_tick++;

    /* TODO: Replace with direct sensor reads via app_sensor */
    /* Original code used ipc_sensorhub_snapshot(&snap) for live BMI270 data.
     * Using placeholder/demo values so the UI still works without IPC. */
    float ax = 0.01f * (s_demo_tick % 100);
    float ay = -0.5f + 0.02f * (s_demo_tick % 50);
    float az = 9.78f + 0.01f * (s_demo_tick % 20);
    float gx = 1.0f * (s_demo_tick % 30);
    float gy = -0.5f * (s_demo_tick % 20);
    float gz = 0.2f * (s_demo_tick % 15);

    if (s_ax_label) lv_label_set_text_fmt(s_ax_label, "aX: %.2f g", (double)ax);
    if (s_ay_label) lv_label_set_text_fmt(s_ay_label, "aY: %.2f g", (double)ay);
    if (s_az_label) lv_label_set_text_fmt(s_az_label, "aZ: %.2f g", (double)az);
    if (s_gx_label) lv_label_set_text_fmt(s_gx_label, "gX: %.1f dps", (double)gx);
    if (s_gy_label) lv_label_set_text_fmt(s_gy_label, "gY: %.1f dps", (double)gy);
    if (s_gz_label) lv_label_set_text_fmt(s_gz_label, "gZ: %.1f dps", (double)gz);
}

static void create_sensors_page(lv_obj_t *content)
{
    lv_obj_t *title = example_label_create(content, LV_SYMBOL_EYE_OPEN " BMI270 IMU Data",
                                           &lv_font_montserrat_20, UI_COLOR_BMI270);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    /* Accelerometer card */
    lv_obj_t *accel_card = example_card_create(content, 420, 130, UI_COLOR_CARD_BG);
    lv_obj_align(accel_card, LV_ALIGN_TOP_MID, 0, 52);

    lv_obj_t *accel_title = example_label_create(accel_card, "Accelerometer",
                                                 &lv_font_montserrat_16, UI_COLOR_BMI270);
    lv_obj_align(accel_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ax_label = example_label_create(accel_card, "aX: --- g", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_ax_label, LV_ALIGN_TOP_LEFT, 0, 28);

    s_ay_label = example_label_create(accel_card, "aY: --- g", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_ay_label, LV_ALIGN_TOP_LEFT, 0, 52);

    s_az_label = example_label_create(accel_card, "aZ: --- g", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_az_label, LV_ALIGN_TOP_LEFT, 0, 76);

    /* Gyroscope card */
    lv_obj_t *gyro_card = example_card_create(content, 420, 130, UI_COLOR_CARD_BG);
    lv_obj_align(gyro_card, LV_ALIGN_TOP_MID, 0, 196);

    lv_obj_t *gyro_title = example_label_create(gyro_card, "Gyroscope",
                                                &lv_font_montserrat_16, UI_COLOR_INFO);
    lv_obj_align(gyro_title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_gx_label = example_label_create(gyro_card, "gX: --- dps", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_gx_label, LV_ALIGN_TOP_LEFT, 0, 28);

    s_gy_label = example_label_create(gyro_card, "gY: --- dps", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_gy_label, LV_ALIGN_TOP_LEFT, 0, 52);

    s_gz_label = example_label_create(gyro_card, "gZ: --- dps", &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(s_gz_label, LV_ALIGN_TOP_LEFT, 0, 76);

    /* Back button */
    lv_obj_t *back = lv_button_create(content);
    lv_obj_set_size(back, 120, 40);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x1A2A4A), 0);
    lv_obj_add_event_cb(back, (lv_event_cb_t)navigate_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = example_label_create(back, LV_SYMBOL_LEFT " Back",
                                              &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_center(back_lbl);

    /* Start sensor timer */
    s_sensor_timer = lv_timer_create(sensor_timer_cb, 200, NULL);
}

/* ── Settings Page ───────────────────────────────────────────────── */
static void create_settings_page(lv_obj_t *content)
{
    lv_obj_t *title = example_label_create(content, LV_SYMBOL_SETTINGS " Settings",
                                           &lv_font_montserrat_20, UI_COLOR_WARNING);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    /* Brightness card */
    lv_obj_t *bright_card = example_card_create(content, 420, 100, UI_COLOR_CARD_BG);
    lv_obj_align(bright_card, LV_ALIGN_TOP_MID, 0, 52);

    lv_obj_t *bright_title = example_label_create(bright_card, "Brightness",
                                                  &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(bright_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *slider = lv_slider_create(bright_card);
    lv_obj_set_size(slider, 340, 16);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x1A2A4A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, UI_COLOR_PRIMARY, LV_PART_KNOB);

    /* Theme card */
    lv_obj_t *theme_card = example_card_create(content, 420, 90, UI_COLOR_CARD_BG);
    lv_obj_align(theme_card, LV_ALIGN_TOP_MID, 0, 168);

    lv_obj_t *theme_title = example_label_create(theme_card, "Dark Theme",
                                                 &lv_font_montserrat_16, UI_COLOR_TEXT);
    lv_obj_align(theme_title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *sw = lv_switch_create(theme_card);
    lv_obj_set_size(sw, 50, 26);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, UI_COLOR_PRIMARY, LV_PART_INDICATOR | LV_STATE_CHECKED);

    /* Info card */
    lv_obj_t *info_card = example_card_create(content, 420, 90, UI_COLOR_CARD_BG);
    lv_obj_align(info_card, LV_ALIGN_TOP_MID, 0, 274);

    lv_obj_t *info_title = example_label_create(info_card, "About",
                                                &lv_font_montserrat_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *info_text = example_label_create(info_card, "TESAIoT Multi-Page Demo\nPSoC Edge E84 | LVGL 9.2",
                                               &lv_font_montserrat_14, UI_COLOR_TEXT_DIM);
    lv_obj_align(info_text, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Back button */
    lv_obj_t *back = lv_button_create(content);
    lv_obj_set_size(back, 120, 40);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x1A2A4A), 0);
    lv_obj_add_event_cb(back, (lv_event_cb_t)navigate_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = example_label_create(back, LV_SYMBOL_LEFT " Back",
                                              &lv_font_montserrat_14, UI_COLOR_TEXT);
    lv_obj_center(back_lbl);
}

/* ── Create Tab Bar ──────────────────────────────────────────────── */
static void create_tab_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, DISPLAY_WIDTH, TAB_BAR_H);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    int btn_w = DISPLAY_WIDTH / NUM_TABS;
    for (int i = 0; i < NUM_TABS; i++) {
        lv_obj_t *btn = lv_obj_create(bar);
        lv_obj_set_size(btn, btn_w - 8, TAB_BAR_H - 8);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A2A4A), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, tab_event_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        char buf[32];
        snprintf(buf, sizeof(buf), "%s\n%s", s_pages[i].icon, s_pages[i].title);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DIM, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl);

        s_tab_btns[i] = btn;
        s_tab_labels[i] = lbl;
    }
}

/* ── Entry Point ─────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    tesaiot_add_thai_support_badge();

    s_demo_tick = 0;

    /* Content area (above tab bar) */
    s_content_area = lv_obj_create(parent);
    lv_obj_set_size(s_content_area, DISPLAY_WIDTH, DISPLAY_HEIGHT - TAB_BAR_H);
    lv_obj_align(s_content_area, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_content_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_content_area, 0, 0);
    lv_obj_set_style_pad_all(s_content_area, 0, 0);
    lv_obj_clear_flag(s_content_area, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *th_sub = lv_label_create(s_content_area);
    lv_label_set_text(th_sub, "แอปพลิเคชันหลายหน้า");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 4);

    /* Tab bar at bottom */
    create_tab_bar(parent);

    /* Start on Home page */
    navigate_to(PAGE_HOME, false);
}
