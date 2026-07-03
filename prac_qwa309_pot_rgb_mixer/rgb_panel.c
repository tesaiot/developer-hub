/*******************************************************************************
* File Name        : rgb_panel.c
*
* Description      : Minimal DFRobot DFR0522 RGB panel I2C driver.
*******************************************************************************/

#include "rgb_panel.h"

#include <string.h>

#define RGB_PANEL_COMMAND_REGISTER  (0x02U)
#define RGB_PANEL_CLEAR_FUNCTION    (0x01U)
#define RGB_PANEL_FILL_FUNCTION     (0x09U)
#define RGB_PANEL_PIXEL_FUNCTION    (0x08U)
#define RGB_PANEL_WIDTH             (16U)
#define RGB_PANEL_HEIGHT            (8U)
#define RGB_PANEL_BYTE_TIMEOUT_MS   (5U)

static cy_en_scb_i2c_status_t rgb_panel_write(CySCB_Type *base,
                                              cy_stc_scb_i2c_context_t *context,
                                              uint8_t function,
                                              rgb_panel_color_t color,
                                              uint8_t x,
                                              uint8_t y)
{
    uint8_t tx_buffer[RGB_PANEL_TX_SIZE];
    cy_en_scb_i2c_status_t status;
    cy_en_scb_i2c_status_t stop_status;

    memset(tx_buffer, 0, sizeof(tx_buffer));
    tx_buffer[0] = RGB_PANEL_COMMAND_REGISTER;
    tx_buffer[1] = function;
    tx_buffer[2] = (uint8_t)color;
    tx_buffer[3] = x;
    tx_buffer[4] = y;

    status = Cy_SCB_I2C_MasterSendStart(base,
                                        RGB_PANEL_I2C_ADDRESS,
                                        CY_SCB_I2C_WRITE_XFER,
                                        RGB_PANEL_BYTE_TIMEOUT_MS,
                                        context);

    if (status == CY_SCB_I2C_SUCCESS)
    {
        for (uint32_t i = 0U; i < RGB_PANEL_TX_SIZE; i++)
        {
            status = Cy_SCB_I2C_MasterWriteByte(base,
                                                tx_buffer[i],
                                                RGB_PANEL_BYTE_TIMEOUT_MS,
                                                context);
            if (status != CY_SCB_I2C_SUCCESS)
            {
                break;
            }
        }
    }

    stop_status = Cy_SCB_I2C_MasterSendStop(base,
                                            RGB_PANEL_BYTE_TIMEOUT_MS,
                                            context);
    if ((status == CY_SCB_I2C_SUCCESS) &&
        (stop_status != CY_SCB_I2C_SUCCESS))
    {
        status = stop_status;
    }

    return status;
}

cy_en_scb_i2c_status_t rgb_panel_clear(CySCB_Type *base,
                                       cy_stc_scb_i2c_context_t *context)
{
    return rgb_panel_write(base,
                           context,
                           RGB_PANEL_CLEAR_FUNCTION,
                           RGB_PANEL_COLOR_OFF,
                           0U,
                           0U);
}

cy_en_scb_i2c_status_t rgb_panel_fill(CySCB_Type *base,
                                      cy_stc_scb_i2c_context_t *context,
                                      rgb_panel_color_t color)
{
    if ((color < RGB_PANEL_COLOR_RED) || (color > RGB_PANEL_COLOR_WHITE))
    {
        return CY_SCB_I2C_BAD_PARAM;
    }

    return rgb_panel_write(base,
                           context,
                           RGB_PANEL_FILL_FUNCTION,
                           color,
                           0U,
                           0U);
}

cy_en_scb_i2c_status_t rgb_panel_pixel(CySCB_Type *base,
                                       cy_stc_scb_i2c_context_t *context,
                                       uint8_t x,
                                       uint8_t y,
                                       rgb_panel_color_t color)
{
    if ((x >= RGB_PANEL_WIDTH) || (y >= RGB_PANEL_HEIGHT) ||
        (color > RGB_PANEL_COLOR_WHITE))
    {
        return CY_SCB_I2C_BAD_PARAM;
    }

    return rgb_panel_write(base,
                           context,
                           RGB_PANEL_PIXEL_FUNCTION,
                           color,
                           x,
                           y);
}

/* [] END OF FILE */
