/******************************************************************************
* File Name:   pal_os_lock.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. The function here are not 
*              really used by the host library (mw) itself
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
#include "pal/pal_os_lock.h"

#include "FreeRTOS.h"
#include "semphr.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
SemaphoreHandle_t xLockSemaphoreHandle;

volatile uint8_t first_call_flag = 1;

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
void _lock_init(pal_os_lock_t * p_lock)
{
    xLockSemaphoreHandle = xSemaphoreCreateBinary();
    pal_os_lock_release(p_lock);
}

void pal_os_lock_create(pal_os_lock_t * p_lock, uint8_t lock_type)
{
    p_lock->type = lock_type;
    p_lock->lock = 0;
}

//lint --e{715} suppress "p_lock is not used here as it is placeholder for future."
//lint --e{818} suppress "Not declared as pointer as nothing needs to be updated in the pointer."
void pal_os_lock_destroy(pal_os_lock_t * p_lock)
{

}


pal_status_t pal_os_lock_acquire(pal_os_lock_t * p_lock)
{
    (void) p_lock;
    vPortEnterCritical();
    if (first_call_flag)
    {
        _lock_init(p_lock);
        first_call_flag = 0;
    }
    vPortExitCritical();

    if ( xSemaphoreTake(xLockSemaphoreHandle, portMAX_DELAY) == pdTRUE )
        return PAL_STATUS_SUCCESS;
    else {
        return PAL_STATUS_FAILURE;
    }
}

void pal_os_lock_release(pal_os_lock_t * p_lock)
{
    (void) p_lock;

    xSemaphoreGive(xLockSemaphoreHandle);
}

void pal_os_lock_enter_critical_section()
{
    vPortEnterCritical();
}

void pal_os_lock_exit_critical_section()
{
    vPortExitCritical();
}

