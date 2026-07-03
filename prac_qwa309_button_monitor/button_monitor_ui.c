/*******************************************************************************
* File Name        : button_monitor_ui.c
*
* Description      : LVGL dashboard and GPIO sampling for two active-low buttons.
*******************************************************************************/

#include "button_monitor_ui.h"

#include "cy_gpio.h"
#include "cybsp.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_COUNT               (2U)
#define BUTTON_REFRESH_PERIOD_MS   (25U)
#define BUTTON_DEBOUNCE_TICKS      (2U)

typedef struct
{
    const char *name;
    const char *pin;
    GPIO_PRT_Type *port;
    uint32_t pin_num;
    uint32_t accent;
    bool stable_pressed;
    bool last_sample_pressed;
    uint8_t debounce_count;
    uint32_t press_count;
    uint32_t hold_time_ms;
    lv_obj_t *card;
    lv_obj_t *state_label;
    lv_obj_t *level_label;
    lv_obj_t *count_label;
    lv_obj_t *hold_label;
    lv_obj_t *indicator;
} button_channel_t;

static button_channel_t buttons[BUTTON_COUNT] =
{
    {
        "SW5",
        "P17.7",
        P17_7_PORT,
        P17_7_PIN,
        0x22C55E,
        false,
        false,
        0U,
        0U,
        0U,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "SW6",
        "P17.5",
        P17_5_PORT,
        P17_5_PIN,
        0x38BDF8,
        false,
        false,
        0U,
        0U,
        0U,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
};

static lv_obj_t *summary_label;

static void button_inputs_init(void);
static void button_timer_cb(lv_timer_t *timer);
static void update_button(button_channel_t *button);
static void apply_button_visual(button_channel_t *button);
static void create_header(lv_obj_t *screen);
static void create_button_row(lv_obj_t *screen);
static void create_button_card(lv_obj_t *parent, button_channel_t *button);
static void style_label(lv_obj_t *label, uint32_t color, const lv_font_t *font);

void button_monitor_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    button_inputs_init();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 16, 0);
    lv_obj_set_style_pad_gap(screen, 14, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    create_header(screen);
    create_button_row(screen);

    lv_timer_t *timer = lv_timer_create(button_timer_cb,
                                        BUTTON_REFRESH_PERIOD_MS,
                                        NULL);
    lv_timer_ready(timer);
}

static void button_inputs_init(void)
{
    Cy_GPIO_Pin_FastInit(P17_5_PORT,
                         P17_5_PIN,
                         CY_GPIO_DM_PULLUP,
                         1UL,
                         HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(P17_7_PORT,
                         P17_7_PIN,
                         CY_GPIO_DM_PULLUP,
                         1UL,
                         HSIOM_SEL_GPIO);
}

static void create_header(lv_obj_t *screen)
{
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_t *title;

    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 78);
    lv_obj_set_style_pad_gap(header, 4, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);

    title = lv_label_create(header);
    lv_label_set_text(title, "Button Monitor");
    style_label(title, 0xF8FAFC, &lv_font_montserrat_24);

    summary_label = lv_label_create(header);
    lv_label_set_text(summary_label, "SW5 / SW6 released");
    style_label(summary_label, 0x94A3B8, &lv_font_montserrat_16);
}

static void create_button_row(lv_obj_t *screen)
{
    lv_obj_t *row = lv_obj_create(screen);

    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_style_pad_gap(row, 14, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
    {
        create_button_card(row, &buttons[i]);
    }
}

static void create_button_card(lv_obj_t *parent, button_channel_t *button)
{
    lv_obj_t *top_row;
    lv_obj_t *name_label;
    lv_obj_t *pin_label;
    lv_obj_t *info_row;

    button->card = lv_obj_create(parent);
    lv_obj_set_flex_grow(button->card, 1);
    lv_obj_set_height(button->card, lv_pct(100));
    lv_obj_set_style_radius(button->card, 6, 0);
    lv_obj_set_style_bg_color(button->card, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_width(button->card, 2, 0);
    lv_obj_set_style_border_color(button->card, lv_color_hex(0x263241), 0);
    lv_obj_set_style_pad_all(button->card, 18, 0);
    lv_obj_set_style_pad_gap(button->card, 12, 0);
    lv_obj_set_flex_flow(button->card, LV_FLEX_FLOW_COLUMN);

    top_row = lv_obj_create(button->card);
    lv_obj_remove_style_all(top_row);
    lv_obj_set_width(top_row, lv_pct(100));
    lv_obj_set_height(top_row, 34);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    name_label = lv_label_create(top_row);
    lv_label_set_text(name_label, button->name);
    style_label(name_label, button->accent, &lv_font_montserrat_16);

    pin_label = lv_label_create(top_row);
    lv_label_set_text(pin_label, button->pin);
    lv_obj_set_flex_grow(pin_label, 1);
    lv_obj_set_style_text_align(pin_label, LV_TEXT_ALIGN_RIGHT, 0);
    style_label(pin_label, 0x94A3B8, &lv_font_montserrat_14);

    button->indicator = lv_obj_create(button->card);
    lv_obj_remove_style_all(button->indicator);
    lv_obj_set_size(button->indicator, 118, 118);
    lv_obj_set_style_radius(button->indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(button->indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button->indicator, lv_color_hex(0x1F2937), 0);
    lv_obj_set_style_border_width(button->indicator, 5, 0);
    lv_obj_set_style_border_color(button->indicator, lv_color_hex(0x334155), 0);
    lv_obj_set_style_align(button->indicator, LV_ALIGN_CENTER, 0);

    button->state_label = lv_label_create(button->card);
    lv_label_set_text(button->state_label, "RELEASED");
    lv_obj_set_width(button->state_label, lv_pct(100));
    lv_obj_set_style_text_align(button->state_label, LV_TEXT_ALIGN_CENTER, 0);
    style_label(button->state_label, 0xCBD5E1, &lv_font_montserrat_24);

    info_row = lv_obj_create(button->card);
    lv_obj_remove_style_all(info_row);
    lv_obj_set_width(info_row, lv_pct(100));
    lv_obj_set_height(info_row, 70);
    lv_obj_set_style_pad_gap(info_row, 8, 0);
    lv_obj_set_flex_flow(info_row, LV_FLEX_FLOW_COLUMN);

    button->level_label = lv_label_create(info_row);
    lv_label_set_text(button->level_label, "Level HIGH  idle");
    style_label(button->level_label, 0x94A3B8, &lv_font_montserrat_14);

    button->count_label = lv_label_create(info_row);
    lv_label_set_text(button->count_label, "Presses 0");
    style_label(button->count_label, 0xCBD5E1, &lv_font_montserrat_16);

    button->hold_label = lv_label_create(info_row);
    lv_label_set_text(button->hold_label, "Hold 0 ms");
    style_label(button->hold_label, 0x94A3B8, &lv_font_montserrat_14);

    apply_button_visual(button);
}

static void button_timer_cb(lv_timer_t *timer)
{
    uint8_t pressed_count = 0U;
    (void)timer;

    for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
    {
        update_button(&buttons[i]);
        if (buttons[i].stable_pressed)
        {
            pressed_count++;
        }
    }

    if (0U == pressed_count)
    {
        lv_label_set_text(summary_label, "SW5 / SW6 released");
        lv_obj_set_style_text_color(summary_label, lv_color_hex(0x94A3B8), 0);
    }
    else if (BUTTON_COUNT == pressed_count)
    {
        lv_label_set_text(summary_label, "Both buttons pressed");
        lv_obj_set_style_text_color(summary_label, lv_color_hex(0xFBBF24), 0);
    }
    else
    {
        lv_label_set_text_fmt(summary_label,
                              "%s pressed",
                              buttons[0].stable_pressed ? buttons[0].name :
                                                           buttons[1].name);
        lv_obj_set_style_text_color(summary_label, lv_color_hex(0x5EEAD4), 0);
    }
}

static void update_button(button_channel_t *button)
{
    bool sampled_pressed = (0U == Cy_GPIO_Read(button->port, button->pin_num));

    if (sampled_pressed == button->last_sample_pressed)
    {
        if (button->debounce_count < BUTTON_DEBOUNCE_TICKS)
        {
            button->debounce_count++;
        }
    }
    else
    {
        button->last_sample_pressed = sampled_pressed;
        button->debounce_count = 0U;
    }

    if ((button->debounce_count >= BUTTON_DEBOUNCE_TICKS) &&
        (sampled_pressed != button->stable_pressed))
    {
        button->stable_pressed = sampled_pressed;
        if (button->stable_pressed)
        {
            button->press_count++;
        }
        else
        {
            button->hold_time_ms = 0U;
        }
    }

    if (button->stable_pressed)
    {
        button->hold_time_ms += BUTTON_REFRESH_PERIOD_MS;
    }

    apply_button_visual(button);
}

static void apply_button_visual(button_channel_t *button)
{
    uint32_t card_bg = button->stable_pressed ? 0x10261F : 0x111827;
    uint32_t border = button->stable_pressed ? button->accent : 0x263241;
    uint32_t indicator_bg = button->stable_pressed ? button->accent : 0x1F2937;
    uint32_t indicator_border = button->stable_pressed ? 0xA7F3D0 : 0x334155;
    uint32_t state_color = button->stable_pressed ? 0xF8FAFC : 0xCBD5E1;

    lv_obj_set_style_bg_color(button->card, lv_color_hex(card_bg), 0);
    lv_obj_set_style_border_color(button->card, lv_color_hex(border), 0);
    lv_obj_set_style_bg_color(button->indicator, lv_color_hex(indicator_bg), 0);
    lv_obj_set_style_border_color(button->indicator,
                                  lv_color_hex(indicator_border),
                                  0);

    lv_label_set_text(button->state_label,
                      button->stable_pressed ? "PRESSED" : "RELEASED");
    lv_obj_set_style_text_color(button->state_label,
                                lv_color_hex(state_color),
                                0);

    lv_label_set_text(button->level_label,
                      button->stable_pressed ? "Level LOW  active" :
                                               "Level HIGH  idle");
    lv_label_set_text_fmt(button->count_label,
                          "Presses %lu",
                          (unsigned long)button->press_count);
    lv_label_set_text_fmt(button->hold_label,
                          "Hold %lu ms",
                          (unsigned long)button->hold_time_ms);
}

static void style_label(lv_obj_t *label, uint32_t color, const lv_font_t *font)
{
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);
}

/* [] END OF FILE */
