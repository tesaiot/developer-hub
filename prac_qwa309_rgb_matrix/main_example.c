/*******************************************************************************
 * @file    main_example.c
 * @brief   Practise entry wrapper — QWA309: DFR0522 8x16 RGB dot-matrix.
 *
 * Drives the DFRobot DFR0522 RGB matrix (I2C 0x10) over the shared 3.3V header
 * I2C bus (DISPLAY_I2C_CONTROLLER_HW) that the framework already initializes,
 * with an on-screen control UI. No separate SCB bring-up needed.
 *
 * BUILD (tools_3.8):  make build BOARD=TESAIOT_DEV_KIT
 ******************************************************************************/
#include "app_interface.h"
#include "rgb_matrix_ui.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;   /* rgb_matrix_ui_create() composes on lv_screen_active(). */
    rgb_matrix_ui_create();
}
