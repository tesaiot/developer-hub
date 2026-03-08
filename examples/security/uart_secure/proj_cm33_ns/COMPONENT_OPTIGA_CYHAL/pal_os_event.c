/******************************************************************************
* File Name:   pal_os_event.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. The functions here are
*              very important and shall be implemted with computation
*              In case a hsot library is always busy (hang) it is likely
*              that function here are not corectly implemented
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
#include "pal/pal_os_event.h"
#include "pal/pal.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "mtb_hal.h"

/// @cond hidden
/*******************************************************************************
 * Macros
 ******************************************************************************/
#define PAL_OS_EVENT_INTR_PRIO    (4U)
#define CYHAL_TIMER_SCALING 10

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
/* Timer object used */
mtb_hal_timer_t pal_os_event_timer_obj;

/* PAL OS Event handler */
pal_os_event_t pal_os_event_ctx;

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
/* An internal timer initialisation function */
int32_t pal_os_event_init(pal_os_event_t* p_pal_os_event_ctx, register_callback callback, void * callback_args);

void pal_os_event_trigger_registered_callback(void) { }

static void pal_os_event_timer_callback(TimerHandle_t xTimer)
{
    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT(xTimer);

    pal_os_event_t *p_pal_os_event = (pal_os_event_t *)pvTimerGetTimerID( xTimer );
    p_pal_os_event->callback_registered(p_pal_os_event->callback_ctx);
}

void pal_os_event_start(pal_os_event_t * p_pal_os_event, register_callback callback, void * callback_args)
{
    if (FALSE == p_pal_os_event->is_event_triggered)
    {
        p_pal_os_event->is_event_triggered = TRUE;
        pal_os_event_register_callback_oneshot(p_pal_os_event,callback,callback_args, 1000);
    }
}

void pal_os_event_stop(pal_os_event_t * p_pal_os_event)
{
    //lint --e{714} suppress "The API pal_os_event_stop is not exposed in header file but used as extern in 
    //optiga_cmd.c"
    p_pal_os_event->is_event_triggered = FALSE;
}

pal_os_event_t * pal_os_event_create(register_callback callback, void * callback_args)
{
    if (!pal_os_event_init(&pal_os_event_ctx, callback, callback_args))
        return NULL;

    if ((callback != NULL) && (callback_args != NULL))
    {
        pal_os_event_start(&pal_os_event_ctx, callback, callback_args);
    }

    return (&pal_os_event_ctx);
}

//lint --e{818,715} suppress "As there is no implementation, pal_os_event is not used"
void pal_os_event_destroy(pal_os_event_t * p_pal_os_event)
{
    if (p_pal_os_event != NULL)
    {
        pal_os_event_stop(p_pal_os_event);
        xTimerDelete(p_pal_os_event->os_timer, 1000);
    }
}

void pal_os_event_register_callback_oneshot(pal_os_event_t * p_pal_os_event,
                                             register_callback callback,
                                             void * callback_args,
                                             uint32_t time_us)
{
    if (p_pal_os_event != NULL)
    {
        p_pal_os_event->callback_registered = callback;
        p_pal_os_event->callback_ctx = callback_args;

        if (time_us < 1000)
        {
        time_us = 1000;
        }

        xTimerChangePeriod((TimerHandle_t)p_pal_os_event->os_timer, pdMS_TO_TICKS(time_us / 1000), 0);
    }
}

int32_t pal_os_event_init(pal_os_event_t* p_pal_os_event_ctx, register_callback callback, void * callback_args)
{
    TimerHandle_t xTimer;

    if (p_pal_os_event_ctx != NULL)
    {
        p_pal_os_event_ctx->callback_registered = callback;
        p_pal_os_event_ctx->callback_ctx = callback_args;
        p_pal_os_event_ctx->is_event_triggered = false;

        xTimer = xTimerCreate(NULL, 1, pdFALSE, ( void * ) p_pal_os_event_ctx, pal_os_event_timer_callback);
        if (xTimer != NULL)
        {
            p_pal_os_event_ctx->os_timer = xTimer;
        }
        else
        {
            return 0;
        }
    }

    return 1;
}
