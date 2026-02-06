/******************************************************************************
* File Name:   pal_ifx_i2c_config.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. This file implements 
*              platform abstraction layer configurations for ifx i2c protocol.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Header file includes
 ******************************************************************************/
#include "include/pal/pal_gpio.h"
#include "include/pal/pal_i2c.h"
#include "include/ifx_i2c/ifx_i2c_config.h"
#include "include/pal/pal_ifx_i2c_config.h"
#include "pal_psoc_i2c_mapping.h"
#include "pal_psoc_gpio_mapping.h"
#include "optiga_lib_config.h"
//#include "cy_pdl.h"
#include "mtb_hal.h"
#include "cybsp.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
// i2c driver related
mtb_hal_i2c_t i2c_master_obj;
cy_stc_scb_i2c_context_t i2c_master_context;

#ifdef OPTIGA_TRUSTM_VDD
pal_psoc_gpio_t optiga_vdd_config =
{
    .port_num = OPTIGA_TRUSTM_VDD,
    .init_state = true
};
#endif

#ifdef OPTIGA_TRUSTM_RST
pal_psoc_gpio_t optiga_reset_config =
{
    .port_num = OPTIGA_TRUSTM_RST,
    .init_state = true
};
#endif

pal_psoc_i2c_t optiga_i2c_master_config =
{
    .i2c_controller_channel = &i2c_master_obj,
    .dev_config = &CYBSP_I2C_CONTROLLER_hal_config,
    .context = &i2c_master_context,
    .i2c_clk = {0}
};

/**
* \brief PAL vdd pin configuration for OPTIGA. 
 */
pal_gpio_t optiga_vdd_0 =
{
#ifdef OPTIGA_TRUSTM_VDD
    // Platform specific GPIO context for the pin used to toggle Vdd.
    (void * )&optiga_vdd_config
#else
    NULL
#endif
};

/**
 * \brief PAL reset pin configuration for OPTIGA.
 */
pal_gpio_t optiga_reset_0 =
{
#ifdef OPTIGA_TRUSTM_RST
    // Platform specific GPIO context for the pin used to toggle Reset.
    (void * )&optiga_reset_config
#else
    NULL
#endif
};

/**
 * \brief PAL I2C configuration for OPTIGA.
 */
pal_i2c_t optiga_pal_i2c_context_0 =
{
    /// Pointer to I2C master platform specific context
    (void*)&optiga_i2c_master_config,
    /// Upper layer context
    NULL,
    /// Callback event handler
    NULL,
    /// Slave address
    0x30
};

