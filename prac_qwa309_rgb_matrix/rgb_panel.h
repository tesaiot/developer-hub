/*******************************************************************************
* File Name        : rgb_panel.h
*
* Description      : Minimal DFRobot DFR0522 RGB panel I2C driver.
*******************************************************************************/

#ifndef RGB_PANEL_H
#define RGB_PANEL_H

#include "cy_scb_i2c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RGB_PANEL_I2C_ADDRESS    (0x10U)
#define RGB_PANEL_PAYLOAD_SIZE   (50U)
#define RGB_PANEL_TX_SIZE        (RGB_PANEL_PAYLOAD_SIZE + 1U)

typedef enum
{
    RGB_PANEL_COLOR_OFF = 0U,
    RGB_PANEL_COLOR_RED = 1U,
    RGB_PANEL_COLOR_GREEN = 2U,
    RGB_PANEL_COLOR_YELLOW = 3U,
    RGB_PANEL_COLOR_BLUE = 4U,
    RGB_PANEL_COLOR_PURPLE = 5U,
    RGB_PANEL_COLOR_CYAN = 6U,
    RGB_PANEL_COLOR_WHITE = 7U,
} rgb_panel_color_t;

cy_en_scb_i2c_status_t rgb_panel_clear(CySCB_Type *base,
                                       cy_stc_scb_i2c_context_t *context);

cy_en_scb_i2c_status_t rgb_panel_fill(CySCB_Type *base,
                                      cy_stc_scb_i2c_context_t *context,
                                      rgb_panel_color_t color);

cy_en_scb_i2c_status_t rgb_panel_pixel(CySCB_Type *base,
                                       cy_stc_scb_i2c_context_t *context,
                                       uint8_t x,
                                       uint8_t y,
                                       rgb_panel_color_t color);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RGB_PANEL_H */

/* [] END OF FILE */
