/*******************************************************************************
* File Name        : pot_monitor_ui.c
*
* Description      : LVGL dashboard and SAR ADC sampling for four potentiometers.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "pot_monitor_ui.h"

#include "cy_autanalog.h"
#include "cy_autanalog_ac.h"
#include "cy_autanalog_sar.h"
#include "cy_gpio.h"
#include "cycfg_peripherals.h"
#include "cybsp.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define POT_COUNT               (4U)
#define POT_ADC_INDEX           (0U)
#define POT_ADC_FULL_SCALE      (4095U)
#define POT_ADC_VREF_MV         (1800U)
#define POT_ADC_READY_MASK      (CY_AUTANALOG_SAR_CHAN_MASK_GPIO0 | \
                                 CY_AUTANALOG_SAR_CHAN_MASK_GPIO1 | \
                                 CY_AUTANALOG_SAR_CHAN_MASK_GPIO2 | \
                                 CY_AUTANALOG_SAR_CHAN_MASK_GPIO3)
#define POT_REFRESH_PERIOD_MS   (100U)

/*******************************************************************************
* Data Types
*******************************************************************************/
typedef struct
{
    const char *name;
    const char *pin;
    uint8_t adc_channel;
    uint32_t accent;
    lv_obj_t *value_label;
    lv_obj_t *percent_label;
    lv_obj_t *raw_label;
    lv_obj_t *bar;
} pot_channel_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
static pot_channel_t pot_channels[POT_COUNT] =
{
    { "VR1", "P15.5  ADC5", 1U, 0x14B8A6, NULL, NULL, NULL, NULL },
    { "VR2", "P15.4  ADC4", 0U, 0x22C55E, NULL, NULL, NULL, NULL },
    { "VR3", "P15.6  ADC6", 2U, 0xF59E0B, NULL, NULL, NULL, NULL },
    { "VR4", "P15.7  ADC7", 3U, 0xF43F5E, NULL, NULL, NULL, NULL },
};

static lv_obj_t *adc_status_label;
static lv_obj_t *sample_rate_label;
static bool adc_ready;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static bool pot_adc_init(void);
static void pot_adc_init_pin(GPIO_PRT_Type *port, uint32_t pin);
static void pot_timer_cb(lv_timer_t *timer);
static void update_channel(pot_channel_t *channel);
static void create_header(lv_obj_t *screen);
static void create_summary(lv_obj_t *screen);
static void create_channel_grid(lv_obj_t *screen);
static void create_channel_card(lv_obj_t *parent, pot_channel_t *channel);
static void style_label(lv_obj_t *label, uint32_t color, const lv_font_t *font);

/*******************************************************************************
* Function Name: pot_monitor_ui_create
********************************************************************************
* Summary:
*  Creates the LVGL potentiometer monitor screen and starts periodic ADC refresh.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void pot_monitor_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    adc_ready = pot_adc_init();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 14, 0);
    lv_obj_set_style_pad_gap(screen, 10, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    create_header(screen);
    create_summary(screen);
    create_channel_grid(screen);

    if (adc_ready)
    {
        lv_timer_t *timer = lv_timer_create(pot_timer_cb, POT_REFRESH_PERIOD_MS, NULL);
        lv_timer_ready(timer);
    }
    else
    {
        lv_label_set_text(adc_status_label, "ADC init failed");
    }
}

/*******************************************************************************
* Function Name: pot_adc_init
********************************************************************************/
static bool pot_adc_init(void)
{
    uint32_t init_status;

    pot_adc_init_pin(P15_4_PORT, P15_4_PIN);
    pot_adc_init_pin(P15_5_PORT, P15_5_PIN);
    pot_adc_init_pin(P15_6_PORT, P15_6_PIN);
    pot_adc_init_pin(P15_7_PORT, P15_7_PIN);

    init_status = Cy_AutAnalog_Init(&autonomous_analog_init);
    if (CY_AUTANALOG_SUCCESS != init_status)
    {
        return false;
    }

    Cy_AutAnalog_Enable();
    Cy_AutAnalog_StartAutonomousControl();

    return true;
}

/*******************************************************************************
* Function Name: pot_adc_init_pin
********************************************************************************/
static void pot_adc_init_pin(GPIO_PRT_Type *port, uint32_t pin)
{
    Cy_GPIO_Pin_FastInit(port, pin, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
}

/*******************************************************************************
* Function Name: create_header
********************************************************************************/
static void create_header(lv_obj_t *screen)
{
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_t *title;

    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 54);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    title = lv_label_create(header);
    lv_label_set_text(title, "Pot Monitor");
    style_label(title, 0xF8FAFC, &lv_font_montserrat_24);

    adc_status_label = lv_label_create(header);
    lv_label_set_text(adc_status_label, adc_ready ? "Starting ADC" : "ADC offline");
    lv_obj_set_flex_grow(adc_status_label, 1);
    lv_obj_set_style_text_align(adc_status_label, LV_TEXT_ALIGN_RIGHT, 0);
    style_label(adc_status_label, adc_ready ? 0x5EEAD4 : 0xFCA5A5, &lv_font_montserrat_16);
}

/*******************************************************************************
* Function Name: create_summary
********************************************************************************/
static void create_summary(lv_obj_t *screen)
{
    lv_obj_t *summary = lv_obj_create(screen);
    lv_obj_t *range_label;

    lv_obj_remove_style_all(summary);
    lv_obj_set_width(summary, lv_pct(100));
    lv_obj_set_height(summary, 42);
    lv_obj_set_style_bg_color(summary, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(summary, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(summary, 6, 0);
    lv_obj_set_style_pad_hor(summary, 14, 0);
    lv_obj_set_style_pad_ver(summary, 8, 0);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(summary,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    range_label = lv_label_create(summary);
    lv_label_set_text(range_label, "0.000-1.800 V");
    style_label(range_label, 0xCBD5E1, &lv_font_montserrat_14);

    sample_rate_label = lv_label_create(summary);
    lv_label_set_text(sample_rate_label, "100 ms");
    lv_obj_set_flex_grow(sample_rate_label, 1);
    lv_obj_set_style_text_align(sample_rate_label, LV_TEXT_ALIGN_RIGHT, 0);
    style_label(sample_rate_label, 0x94A3B8, &lv_font_montserrat_14);
}

/*******************************************************************************
* Function Name: create_channel_grid
********************************************************************************/
static void create_channel_grid(lv_obj_t *screen)
{
    lv_obj_t *grid = lv_obj_create(screen);
    lv_obj_t *row;

    lv_obj_remove_style_all(grid);
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_style_pad_gap(grid, 10, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_COLUMN);

    for (uint8_t row_index = 0U; row_index < 2U; row_index++)
    {
        row = lv_obj_create(grid);
        lv_obj_remove_style_all(row);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_flex_grow(row, 1);
        lv_obj_set_style_pad_gap(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

        create_channel_card(row, &pot_channels[row_index * 2U]);
        create_channel_card(row, &pot_channels[(row_index * 2U) + 1U]);
    }
}

/*******************************************************************************
* Function Name: create_channel_card
********************************************************************************/
static void create_channel_card(lv_obj_t *parent, pot_channel_t *channel)
{
    lv_obj_t *card;
    lv_obj_t *top_row;
    lv_obj_t *name_label;
    lv_obj_t *pin_label;
    lv_obj_t *bottom_row;

    card = lv_obj_create(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, lv_pct(100));
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(channel->accent), 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    top_row = lv_obj_create(card);
    lv_obj_remove_style_all(top_row);
    lv_obj_set_width(top_row, lv_pct(100));
    lv_obj_set_height(top_row, 30);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    name_label = lv_label_create(top_row);
    lv_label_set_text(name_label, channel->name);
    style_label(name_label, channel->accent, &lv_font_montserrat_16);

    pin_label = lv_label_create(top_row);
    lv_label_set_text(pin_label, channel->pin);
    lv_obj_set_flex_grow(pin_label, 1);
    lv_obj_set_style_text_align(pin_label, LV_TEXT_ALIGN_RIGHT, 0);
    style_label(pin_label, 0x94A3B8, &lv_font_montserrat_14);

    channel->value_label = lv_label_create(card);
    lv_label_set_text(channel->value_label, "---.--- V");
    style_label(channel->value_label, 0xF8FAFC, &lv_font_montserrat_24);

    channel->bar = lv_bar_create(card);
    lv_obj_set_width(channel->bar, lv_pct(100));
    lv_obj_set_height(channel->bar, 18);
    lv_obj_set_style_radius(channel->bar, 4, 0);
    lv_obj_set_style_bg_color(channel->bar, lv_color_hex(0x1F2937), LV_PART_MAIN);
    lv_obj_set_style_bg_color(channel->bar, lv_color_hex(channel->accent), LV_PART_INDICATOR);
    lv_bar_set_range(channel->bar, 0, 1000);
    lv_bar_set_value(channel->bar, 0, LV_ANIM_OFF);

    bottom_row = lv_obj_create(card);
    lv_obj_remove_style_all(bottom_row);
    lv_obj_set_width(bottom_row, lv_pct(100));
    lv_obj_set_height(bottom_row, 26);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_row,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    channel->percent_label = lv_label_create(bottom_row);
    lv_label_set_text(channel->percent_label, "--.- %");
    style_label(channel->percent_label, 0xCBD5E1, &lv_font_montserrat_16);

    channel->raw_label = lv_label_create(bottom_row);
    lv_label_set_text(channel->raw_label, "raw ----");
    lv_obj_set_flex_grow(channel->raw_label, 1);
    lv_obj_set_style_text_align(channel->raw_label, LV_TEXT_ALIGN_RIGHT, 0);
    style_label(channel->raw_label, 0x94A3B8, &lv_font_montserrat_14);
}

/*******************************************************************************
* Function Name: pot_timer_cb
********************************************************************************/
static void pot_timer_cb(lv_timer_t *timer)
{
    uint8_t status_mask;
    (void)timer;

    status_mask = Cy_AutAnalog_SAR_GetHSchanResultStatus(POT_ADC_INDEX);

    for (uint8_t i = 0U; i < POT_COUNT; i++)
    {
        update_channel(&pot_channels[i]);
    }

    if ((status_mask & POT_ADC_READY_MASK) == POT_ADC_READY_MASK)
    {
        lv_label_set_text(adc_status_label, "Live");
        lv_obj_set_style_text_color(adc_status_label, lv_color_hex(0x5EEAD4), 0);
    }
    else
    {
        lv_label_set_text(adc_status_label, "ADC settling");
        lv_obj_set_style_text_color(adc_status_label, lv_color_hex(0xFBBF24), 0);
    }

    lv_label_set_text_fmt(sample_rate_label,
                          "%lu ms",
                          (unsigned long)POT_REFRESH_PERIOD_MS);
}

/*******************************************************************************
* Function Name: update_channel
********************************************************************************/
static void update_channel(pot_channel_t *channel)
{
    uint16_t raw;
    uint32_t millivolts;
    uint32_t percent_tenths;
    uint32_t bar_value;

    raw = (uint16_t)Cy_AutAnalog_SAR_ReadResult(POT_ADC_INDEX,
                                                CY_AUTANALOG_SAR_INPUT_GPIO,
                                                channel->adc_channel);
    raw &= 0x0FFFU;

    millivolts = ((uint32_t)raw * POT_ADC_VREF_MV) / POT_ADC_FULL_SCALE;
    percent_tenths = ((uint32_t)raw * 1000U) / POT_ADC_FULL_SCALE;
    bar_value = ((uint32_t)raw * 1000U) / POT_ADC_FULL_SCALE;

    lv_label_set_text_fmt(channel->value_label,
                          "%lu.%03lu V",
                          (unsigned long)(millivolts / 1000U),
                          (unsigned long)(millivolts % 1000U));
    lv_label_set_text_fmt(channel->percent_label,
                          "%lu.%lu %%",
                          (unsigned long)(percent_tenths / 10U),
                          (unsigned long)(percent_tenths % 10U));
    lv_label_set_text_fmt(channel->raw_label,
                          "raw %04u",
                          (unsigned int)raw);
    lv_bar_set_value(channel->bar, (int32_t)bar_value, LV_ANIM_OFF);
}

/*******************************************************************************
* Function Name: style_label
********************************************************************************/
static void style_label(lv_obj_t *label, uint32_t color, const lv_font_t *font)
{
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);
}

/* [] END OF FILE */
