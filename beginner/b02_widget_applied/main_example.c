/**
 * @file    main_example.c
 * @brief   Widget Applied - Grid Menu with 10 practical widget demos
 *
 * A scrollable grid of styled cards (3 columns) serves as the Home screen.
 * Tapping a card navigates to the corresponding applied widget demo.
 * A "Home" button in each demo returns to the grid.
 *
 * Board:   PSoC Edge E84 (KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2)
 * Display: 480x800 AMOLED, LVGL 9.2
 */

#include "pse84_common.h"
#include "pages.h"

/* ── Menu item definition ───────────────────────────────────────────── */
typedef struct {
    const char *title;
    const char *icon;
    void (*create_fn)(lv_obj_t *parent);
} menu_item_t;

static const menu_item_t s_items[] = {
    { "LED Brightness", LV_SYMBOL_SETTINGS, page_led_brightness_create },
    { "Switch LED",     LV_SYMBOL_POWER,    page_switch_led_create     },
    { "Stopwatch",      LV_SYMBOL_PLAY,     page_stopwatch_create      },
    { "Reaction Game",  LV_SYMBOL_SHUFFLE,  page_reaction_game_create  },
    { "Bar Chart",      LV_SYMBOL_BARS,     page_bar_chart_create      },
    { "IMU Display",    LV_SYMBOL_GPS,      page_imu_display_create    },
    { "Temperature",    LV_SYMBOL_EYE_OPEN, page_temperature_create    },
    { "Humidity",       LV_SYMBOL_CHARGE,   page_humidity_create       },
    { "Compass",        LV_SYMBOL_GPS,      page_compass_create        },
    { "System Info",    LV_SYMBOL_SETTINGS, page_system_info_create    },
};
#define ITEM_COUNT (sizeof(s_items) / sizeof(s_items[0]))

/* ── Static state ───────────────────────────────────────────────────── */
static lv_obj_t *s_parent;
static lv_obj_t *s_grid;
static lv_obj_t *s_page;

/* ── Card dimensions ────────────────────────────────────────────────── */
#define CARD_W          138
#define CARD_H          100
#define CARD_BG         0x1A2A40
#define CARD_BG_PRESSED 0x263B5A
#define GRID_BG         0x0D1B2A

/* ── Navigate back to Home grid ─────────────────────────────────────── */
static void home_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_page) {
        lv_obj_del(s_page);
        s_page = NULL;
    }
    lv_obj_remove_flag(s_grid, LV_OBJ_FLAG_HIDDEN);
}

/* ── Card click: open sub-page ──────────────────────────────────────── */
static void card_click_cb(lv_event_t *e)
{
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= (intptr_t)ITEM_COUNT) return;

    /* Hide the grid */
    lv_obj_add_flag(s_grid, LV_OBJ_FLAG_HIDDEN);

    /* Create sub-page container */
    s_page = lv_obj_create(s_parent);
    lv_obj_set_size(s_page, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(s_page, lv_color_hex(GRID_BG), 0);
    lv_obj_set_style_bg_opa(s_page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_page, 0, 0);
    lv_obj_set_style_pad_all(s_page, 0, 0);
    lv_obj_center(s_page);

    /* Home button (top-left) */
    lv_obj_t *btn_home = lv_btn_create(s_page);
    lv_obj_set_size(btn_home, 100, 40);
    lv_obj_set_pos(btn_home, 10, 10);
    lv_obj_set_style_bg_color(btn_home, lv_color_hex(0x263B5A), 0);
    lv_obj_set_style_radius(btn_home, 8, 0);

    lv_obj_t *lbl_home = lv_label_create(btn_home);
    lv_label_set_text(lbl_home, LV_SYMBOL_HOME " Home");
    lv_obj_set_style_text_font(lbl_home, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_home, UI_COLOR_TEXT, 0);
    lv_obj_center(lbl_home);
    lv_obj_add_event_cb(btn_home, home_btn_cb, LV_EVENT_CLICKED, NULL);

    /* Page title (top-center) */
    lv_obj_t *lbl_title = lv_label_create(s_page);
    lv_label_set_text(lbl_title, s_items[idx].title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 16);

    /* Content area for the demo (below Home button and title) */
    lv_obj_t *content = lv_obj_create(s_page);
    lv_obj_set_size(content, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 70);
    lv_obj_set_pos(content, 10, 60);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    /* Call the sub-page create function */
    s_items[idx].create_fn(content);
}

/* ── Create a single card in the grid ───────────────────────────────── */
static lv_obj_t *create_card(lv_obj_t *parent, int idx)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, CARD_W, CARD_H);
    lv_obj_set_style_bg_color(card, lv_color_hex(CARD_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(CARD_BG_PRESSED), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, UI_CARD_BORDER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x2A3A5C), 0);
    lv_obj_set_style_shadow_width(card, 6, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

    /* Flex row: icon on left, title on right */
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(card, 8, 0);

    /* Icon */
    lv_obj_t *lbl_icon = lv_label_create(card);
    lv_label_set_text(lbl_icon, s_items[idx].icon);
    lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_icon, UI_COLOR_PRIMARY, 0);

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, s_items[idx].title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_TEXT, 0);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(lbl_title, 1);

    /* Click event */
    lv_obj_add_event_cb(card, card_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

    return card;
}

/* ── Entry point ────────────────────────────────────────────────────── */
void example_main(lv_obj_t *parent)
{
    s_parent = parent;
    s_page = NULL;

    /* Dark background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(GRID_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Widget Applied");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 12);

    /* Subtitle */
    lv_obj_t *lbl_sub = lv_label_create(parent);
    lv_label_set_text(lbl_sub, "Tap a card to explore");
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_TOP_MID, 0, 42);

    /* วิดเจ็ตประยุกต์ */
    lv_obj_t *th_sub = lv_label_create(parent);
    lv_label_set_text(th_sub, "วิดเจ็ตประยุกต์");
    lv_obj_set_style_text_font(th_sub, &lv_font_noto_thai_14, 0);
    lv_obj_set_style_text_color(th_sub, UI_COLOR_TEXT_DIM, 0);
    lv_obj_align(th_sub, LV_ALIGN_TOP_MID, 0, 58);

    /* Scrollable grid container */
    s_grid = lv_obj_create(parent);
    lv_obj_set_size(s_grid, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 70);
    lv_obj_set_pos(s_grid, 5, 65);
    lv_obj_set_style_bg_opa(s_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_grid, 0, 0);
    lv_obj_set_style_pad_all(s_grid, 5, 0);
    lv_obj_set_scrollbar_mode(s_grid, LV_SCROLLBAR_MODE_AUTO);

    /* Flex wrap: 3 columns */
    lv_obj_set_flex_flow(s_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(s_grid, 10, 0);
    lv_obj_set_style_pad_column(s_grid, 10, 0);

    /* Create cards */
    for (int i = 0; i < (int)ITEM_COUNT; i++) {
        create_card(s_grid, i);
    }
}
