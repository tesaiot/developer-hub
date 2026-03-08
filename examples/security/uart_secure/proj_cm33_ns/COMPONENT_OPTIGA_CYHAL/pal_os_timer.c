/******************************************************************************
* File Name:   pal_os_timer.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file and shall be changed in case
*              base board is changed. this file shall implement only functions
*              which can return timestamps
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
#include "include/pal/pal_os_timer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
 
/*******************************************************************************
 * Function Definitions
 ******************************************************************************/

uint32_t pal_os_timer_get_time_in_microseconds(void)
{
    // !!!OPTIGA_LIB_PORTING_REQUIRED
    // This API is needed to support optiga cmd scheduler.
    static uint32_t count = 0;
    // The implementation must ensure that every invocation of this API returns a unique value.
    return (count++);
}

/**
* Get the current time in milliseconds<br>
*
*
* \retval  uint32_t time in milliseconds
*/
uint32_t pal_os_timer_get_time_in_milliseconds(void)
{
    return xTaskGetTickCount();
}

/**
* Waits or delays until the given milliseconds time
* 
* \param[in] milliseconds Delay value in milliseconds
*
*/
void pal_os_timer_delay_in_milliseconds(uint16_t milliseconds)
{
    const TickType_t xDelay = milliseconds / portTICK_PERIOD_MS;
    vTaskDelay( xDelay );
}

