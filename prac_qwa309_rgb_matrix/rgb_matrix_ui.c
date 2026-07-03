/*******************************************************************************
* File Name        : rgb_matrix_ui.c
*
* Description      : LVGL controls for the DFRobot DFR0522 RGB matrix.
*******************************************************************************/

#include "rgb_matrix_ui.h"

#include "cybsp.h"
#include "cy_scb_i2c.h"
#include "display_i2c_config.h"
#include "lv_port_indev.h"
#include "lvgl.h"
#include "rgb_panel.h"

#include <stdint.h>
#include <stdio.h>

#define PANEL_BUTTON_MAX_COUNT    (19U)
#define DEVICE_CHECK_TIMEOUT_MS   (5U)

typedef enum
{
    PANEL_ACTION_CHECK = 0,
    PANEL_ACTION_CLEAR,
    PANEL_ACTION_PIXEL,
    PANEL_ACTION_FILL,
    PANEL_ACTION_RGB_PIXELS,
    PANEL_ACTION_ALL_COLORS,
    PANEL_ACTION_CORNERS,
} panel_action_t;

typedef struct
{
    const char *label;
    const char *action_name;
    panel_action_t action;
    rgb_panel_color_t color;
    uint32_t button_color;
} panel_button_config_t;

static lv_obj_t *panel_status_label;
static lv_obj_t *panel_buttons[PANEL_BUTTON_MAX_COUNT];
static uint32_t panel_button_count;

static const panel_button_config_t check_button_config =
    { "Check 0x10", "Device 0x10", PANEL_ACTION_CHECK, RGB_PANEL_COLOR_OFF, 0x0F766EU };
static const panel_button_config_t clear_button_config =
    { "Clear", "Clear", PANEL_ACTION_CLEAR, RGB_PANEL_COLOR_OFF, 0x334155U };

static const panel_button_config_t pixel_button_configs[] =
{
    { "Red",    "Pixel Red",    PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_RED,    0xB91C1CU },
    { "Green",  "Pixel Green",  PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_GREEN,  0x15803DU },
    { "Blue",   "Pixel Blue",   PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_BLUE,   0x1D4ED8U },
    { "Yellow", "Pixel Yellow", PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_YELLOW, 0xA16207U },
    { "Purple", "Pixel Purple", PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_PURPLE, 0x7E22CEU },
    { "Cyan",   "Pixel Cyan",   PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_CYAN,   0x0E7490U },
    { "White",  "Pixel White",  PANEL_ACTION_PIXEL, RGB_PANEL_COLOR_WHITE,  0x64748BU },
};

static const panel_button_config_t fill_button_configs[] =
{
    { "Red",    "Fill Red",    PANEL_ACTION_FILL, RGB_PANEL_COLOR_RED,    0xB91C1CU },
    { "Green",  "Fill Green",  PANEL_ACTION_FILL, RGB_PANEL_COLOR_GREEN,  0x15803DU },
    { "Blue",   "Fill Blue",   PANEL_ACTION_FILL, RGB_PANEL_COLOR_BLUE,   0x1D4ED8U },
    { "Yellow", "Fill Yellow", PANEL_ACTION_FILL, RGB_PANEL_COLOR_YELLOW, 0xA16207U },
    { "Purple", "Fill Purple", PANEL_ACTION_FILL, RGB_PANEL_COLOR_PURPLE, 0x7E22CEU },
    { "Cyan",   "Fill Cyan",   PANEL_ACTION_FILL, RGB_PANEL_COLOR_CYAN,   0x0E7490U },
    { "White",  "Fill White",  PANEL_ACTION_FILL, RGB_PANEL_COLOR_WHITE,  0x64748BU },
};

static const panel_button_config_t pattern_button_configs[] =
{
    { "RGB Pixels", "RGB Pixels", PANEL_ACTION_RGB_PIXELS, RGB_PANEL_COLOR_OFF, 0x475569U },
    { "All Colors", "All Colors", PANEL_ACTION_ALL_COLORS, RGB_PANEL_COLOR_OFF, 0x475569U },
    { "Corners",    "Corners",    PANEL_ACTION_CORNERS,    RGB_PANEL_COLOR_OFF, 0x475569U },
};

static void panel_button_event_cb(lv_event_t *event);
static void create_panel_button(lv_obj_t *parent, const panel_button_config_t *config);
static lv_obj_t *create_button_row(lv_obj_t *parent, int32_t height, int32_t gap);
static void create_section_label(lv_obj_t *parent, const char *text);
static cy_en_scb_i2c_status_t run_panel_action(const panel_button_config_t *config);
static cy_en_scb_i2c_status_t check_panel_device(void);
static const char *i2c_status_to_text(cy_en_scb_i2c_status_t status);

void rgb_matrix_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *header;
    lv_obj_t *title;
    lv_obj_t *button_row;

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101418), 0);
    lv_obj_set_style_pad_all(screen, 14, 0);
    lv_obj_set_style_pad_gap(screen, 7, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    panel_button_count = 0U;

    header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 44);
    lv_obj_set_style_pad_gap(header, 8, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    title = lv_label_create(header);
    lv_label_set_text(title, "DFRobot DFR0522 RGB Matrix");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    panel_status_label = lv_label_create(header);
    lv_label_set_text(panel_status_label, "Device 0x10: Not checked");
    lv_obj_set_flex_grow(panel_status_label, 1);
    lv_obj_set_style_text_align(panel_status_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(panel_status_label, lv_color_hex(0xA7B0BD), 0);

    create_panel_button(header, &check_button_config);
    create_panel_button(header, &clear_button_config);

    create_section_label(screen, "Single LED (center pixel)");
    button_row = create_button_row(screen, 48, 6);
    for (uint32_t i = 0U;
         i < (sizeof(pixel_button_configs) / sizeof(pixel_button_configs[0]));
         i++)
    {
        create_panel_button(button_row, &pixel_button_configs[i]);
    }

    create_section_label(screen, "Whole panel (primary and mixed colors)");
    button_row = create_button_row(screen, 48, 6);
    for (uint32_t i = 0U;
         i < (sizeof(fill_button_configs) / sizeof(fill_button_configs[0]));
         i++)
    {
        create_panel_button(button_row, &fill_button_configs[i]);
    }

    create_section_label(screen, "Multi-color patterns");
    button_row = create_button_row(screen, 50, 10);
    for (uint32_t i = 0U;
         i < (sizeof(pattern_button_configs) / sizeof(pattern_button_configs[0]));
         i++)
    {
        create_panel_button(button_row, &pattern_button_configs[i]);
    }
}

static lv_obj_t *create_button_row(lv_obj_t *parent, int32_t height, int32_t gap)
{
    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, height);
    lv_obj_set_style_pad_gap(row, gap, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    return row;
}

static void create_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xCBD5E1), 0);
}

static void create_panel_button(lv_obj_t *parent,
                                const panel_button_config_t *config)
{
    lv_obj_t *button;
    lv_obj_t *label;
    int32_t width = 92;

    if (panel_button_count >= PANEL_BUTTON_MAX_COUNT)
    {
        return;
    }

    if (config->action == PANEL_ACTION_CHECK)
    {
        width = 110;
    }
    else if (config->action >= PANEL_ACTION_RGB_PIXELS)
    {
        width = 145;
    }

    button = lv_button_create(parent);
    panel_buttons[panel_button_count++] = button;
    lv_obj_set_size(button, width, 40);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(config->button_color), 0);
    lv_obj_add_event_cb(button,
                        panel_button_event_cb,
                        LV_EVENT_CLICKED,
                        (void *)config);

    label = lv_label_create(button);
    lv_label_set_text(label, config->label);
    lv_obj_center(label);
}

static void panel_button_event_cb(lv_event_t *event)
{
    const panel_button_config_t *config =
        (const panel_button_config_t *)lv_event_get_user_data(event);
    cy_en_scb_i2c_status_t status;
    char status_text[96];

    for (uint32_t i = 0U; i < panel_button_count; i++)
    {
        lv_obj_add_state(panel_buttons[i], LV_STATE_DISABLED);
    }
    lv_label_set_text(panel_status_label, "Sending...");

    status = run_panel_action(config);
    (void)snprintf(status_text,
                   sizeof(status_text),
                   "%s: %s",
                   config->action_name,
                   i2c_status_to_text(status));
    lv_label_set_text(panel_status_label, status_text);
    lv_obj_set_style_text_color(panel_status_label,
                                (status == CY_SCB_I2C_SUCCESS) ?
                                    lv_color_hex(0x4ADE80) : lv_color_hex(0xF87171),
                                0);

    for (uint32_t i = 0U; i < panel_button_count; i++)
    {
        lv_obj_remove_state(panel_buttons[i], LV_STATE_DISABLED);
    }
}

static cy_en_scb_i2c_status_t run_panel_action(const panel_button_config_t *config)
{
    cy_en_scb_i2c_status_t status;

    if (config->action == PANEL_ACTION_CHECK)
    {
        return check_panel_device();
    }

    if (config->action == PANEL_ACTION_CLEAR)
    {
        return rgb_panel_clear(DISPLAY_I2C_CONTROLLER_HW,
                               &disp_touch_i2c_controller_context);
    }

    if (config->action == PANEL_ACTION_FILL)
    {
        return rgb_panel_fill(DISPLAY_I2C_CONTROLLER_HW,
                              &disp_touch_i2c_controller_context,
                              config->color);
    }

    status = rgb_panel_clear(DISPLAY_I2C_CONTROLLER_HW,
                             &disp_touch_i2c_controller_context);
    if (status != CY_SCB_I2C_SUCCESS)
    {
        return status;
    }
    Cy_SysLib_Delay(100U);

    if (config->action == PANEL_ACTION_PIXEL)
    {
        return rgb_panel_pixel(DISPLAY_I2C_CONTROLLER_HW,
                               &disp_touch_i2c_controller_context,
                               7U,
                               3U,
                               config->color);
    }

    if (config->action == PANEL_ACTION_RGB_PIXELS)
    {
        const rgb_panel_color_t colors[] =
        {
            RGB_PANEL_COLOR_RED,
            RGB_PANEL_COLOR_GREEN,
            RGB_PANEL_COLOR_BLUE,
        };

        for (uint32_t i = 0U; i < 3U; i++)
        {
            status = rgb_panel_pixel(DISPLAY_I2C_CONTROLLER_HW,
                                     &disp_touch_i2c_controller_context,
                                     (uint8_t)(6U + i),
                                     3U,
                                     colors[i]);
            if (status != CY_SCB_I2C_SUCCESS)
            {
                return status;
            }
        }
        return status;
    }

    if (config->action == PANEL_ACTION_ALL_COLORS)
    {
        for (uint32_t i = 0U; i < 7U; i++)
        {
            status = rgb_panel_pixel(DISPLAY_I2C_CONTROLLER_HW,
                                     &disp_touch_i2c_controller_context,
                                     (uint8_t)(4U + i),
                                     3U,
                                     (rgb_panel_color_t)(i + 1U));
            if (status != CY_SCB_I2C_SUCCESS)
            {
                return status;
            }
        }
        return status;
    }

    if (config->action == PANEL_ACTION_CORNERS)
    {
        static const uint8_t x[] = { 0U, 15U, 0U, 15U };
        static const uint8_t y[] = { 0U, 0U, 7U, 7U };
        static const rgb_panel_color_t colors[] =
        {
            RGB_PANEL_COLOR_RED,
            RGB_PANEL_COLOR_GREEN,
            RGB_PANEL_COLOR_BLUE,
            RGB_PANEL_COLOR_WHITE,
        };

        for (uint32_t i = 0U; i < 4U; i++)
        {
            status = rgb_panel_pixel(DISPLAY_I2C_CONTROLLER_HW,
                                     &disp_touch_i2c_controller_context,
                                     x[i],
                                     y[i],
                                     colors[i]);
            if (status != CY_SCB_I2C_SUCCESS)
            {
                return status;
            }
        }
        return status;
    }

    return CY_SCB_I2C_BAD_PARAM;
}

static cy_en_scb_i2c_status_t check_panel_device(void)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(DISPLAY_I2C_CONTROLLER_HW,
                                        RGB_PANEL_I2C_ADDRESS,
                                        CY_SCB_I2C_WRITE_XFER,
                                        DEVICE_CHECK_TIMEOUT_MS,
                                        &disp_touch_i2c_controller_context);
    (void)Cy_SCB_I2C_MasterSendStop(DISPLAY_I2C_CONTROLLER_HW,
                                    DEVICE_CHECK_TIMEOUT_MS,
                                    &disp_touch_i2c_controller_context);
    return status;
}

static const char *i2c_status_to_text(cy_en_scb_i2c_status_t status)
{
    switch (status)
    {
        case CY_SCB_I2C_SUCCESS:
            return "ACK / SUCCESS";
        case CY_SCB_I2C_BAD_PARAM:
            return "BAD PARAMETER";
        case CY_SCB_I2C_MASTER_NOT_READY:
            return "BUS NOT READY";
        case CY_SCB_I2C_MASTER_MANUAL_TIMEOUT:
            return "TIMEOUT";
        case CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK:
            return "ADDRESS NACK";
        case CY_SCB_I2C_MASTER_MANUAL_NAK:
            return "DATA NACK";
        case CY_SCB_I2C_MASTER_MANUAL_ARB_LOST:
            return "ARBITRATION LOST";
        case CY_SCB_I2C_MASTER_MANUAL_BUS_ERR:
            return "BUS ERROR";
        default:
            return "UNKNOWN ERROR";
    }
}

/* [] END OF FILE */
