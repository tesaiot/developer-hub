/**
* \copyright
* MIT License
*
* Copyright (c) 2022 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
* \endcopyright
*
* \author Infineon Technologies AG
*
* \file pal_i2c.c
*
* \brief   This file implements the platform abstraction layer(pal) APIs for I2C.
*
* \ingroup  grPAL
*
* @{
*/

#include "include/pal/pal_i2c.h"
#include "pal_psoc_i2c_mapping.h"
#include "cy_pdl.h"
#include "mtb_hal.h"
#include "cybsp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

//ToDo: Need to check which file needds to be added #include "cyhal_scb_common.h"

#define PAL_I2C_MASTER_MAX_BITRATE  (400U)
#define PAL_I2C_MASTER_INTR_PRIO    (3U)
/// @cond hidden

_STATIC_H volatile uint32_t g_entry_count = 0;
_STATIC_H const pal_i2c_t * gp_pal_i2c_current_ctx;
_STATIC_H uint8_t g_pal_i2c_init_flag = 0;
_STATIC_H TaskHandle_t i2c_taskhandle = NULL;
_STATIC_H SemaphoreHandle_t xIicSemaphoreHandle;


static mtb_hal_i2c_t CYBSP_I2C_CONTROLLER_0_hal_obj __attribute__((unused));
cy_stc_scb_i2c_context_t CYBSP_I2C_CONTROLLER_context;

//lint --e{715} suppress "This is implemented for overall completion of API"
static pal_status_t pal_i2c_acquire(const void * p_i2c_context)
{

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ( xSemaphoreTakeFromISR(xIicSemaphoreHandle, &xHigherPriorityTaskWoken) == pdTRUE )
        return PAL_STATUS_SUCCESS;
    else
        return PAL_STATUS_FAILURE;
}

// I2C release bus function
//lint --e{715} suppress the unused p_i2c_context variable lint, since this is kept for future enhancements
static void pal_i2c_release(const void* p_i2c_context)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xIicSemaphoreHandle, &xHigherPriorityTaskWoken);
}


static void i2c_task(void *pvParameters)
{
  upper_layer_callback_t upper_layer_handler;
  uint32_t event = 0;

  while(1)
  {
    xTaskNotifyWait(0, 0xffffffff, &event, portMAX_DELAY);

    upper_layer_handler = (upper_layer_callback_t)gp_pal_i2c_current_ctx->upper_layer_event_handler;

    if (0UL != (MTB_HAL_I2C_TARGET_ERR_EVENT & event))
    {
        /* In case of error abort transfer */
        upper_layer_handler(gp_pal_i2c_current_ctx->p_upper_layer_ctx, PAL_I2C_EVENT_ERROR);
    }
    /* Check write complete event */
    else if (0UL != (MTB_HAL_I2C_TARGET_WR_CMPLT_EVENT & event))
    {
        /* Perform the required functions */
        upper_layer_handler(gp_pal_i2c_current_ctx->p_upper_layer_ctx, PAL_I2C_EVENT_SUCCESS);
    }
    /* Check read complete event */
    else if (0UL != (MTB_HAL_I2C_TARGET_RD_CMPLT_EVENT & event))
    {
        /* Perform the required functions */
        upper_layer_handler(gp_pal_i2c_current_ctx->p_upper_layer_ctx, PAL_I2C_EVENT_SUCCESS);
    }

    pal_i2c_release(gp_pal_i2c_current_ctx->p_upper_layer_ctx);

  }
}

/* Defining master callback handler */
void i2c_master_event_handler(void *callback_arg, mtb_hal_i2c_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken= pdFALSE;

    xTaskNotifyFromISR(i2c_taskhandle, event, eSetBits, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

pal_status_t pal_i2c_init(const pal_i2c_t * p_i2c_context)
{
    cy_rslt_t cy_hal_status = CY_RSLT_SUCCESS;

    mtb_hal_i2c_cfg_t i2c_master_config = {false,
    		0,
			400000,
			0,
			true
    };


    do
    {
        if (i2c_taskhandle == NULL)
        {
            xIicSemaphoreHandle = xSemaphoreCreateBinary();

            if (xTaskCreate(i2c_task, "i2c_task", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, &i2c_taskhandle) != pdPASS)
            {
                break;
            }

            pal_i2c_release((void * )p_i2c_context);

            return PAL_STATUS_SUCCESS;
        }

        if (g_pal_i2c_init_flag == 0)
        {
            // NOTE: Skip hardware init if context is NULL (semaphore-only init)
            // Hardware init requires valid p_i2c_context to access p_i2c_hw_config
            if (p_i2c_context == NULL)
            {
                return PAL_STATUS_SUCCESS;
            }

        	cy_hal_status = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
        	            &CYBSP_I2C_CONTROLLER_config,
        	            &CYBSP_I2C_CONTROLLER_context);

        	if (CY_RSLT_SUCCESS !=  cy_hal_status)
        	{
        	  	return cy_hal_status;
        	}


        	Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);
            // Init I2C driver
            cy_hal_status = mtb_hal_i2c_setup(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel,
                        ((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->dev_config,
						&CYBSP_I2C_CONTROLLER_context,
                        NULL);

            if (CY_RSLT_SUCCESS != cy_hal_status)
            {
                break;
            }

            // Configure the I2C resource to be master
            cy_hal_status = mtb_hal_i2c_configure(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel, &i2c_master_config);
            // Register i2c master callback
            mtb_hal_i2c_register_callback(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel,
                          (mtb_hal_i2c_event_callback_t) i2c_master_event_handler,
                          NULL);

            // Enable interrupts for I2C master
            mtb_hal_i2c_enable_event(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel,
                         (mtb_hal_i2c_event_t)(MTB_HAL_I2C_TARGET_WR_CMPLT_EVENT \
                         | MTB_HAL_I2C_TARGET_RD_CMPLT_EVENT  \
                         | MTB_HAL_I2C_TARGET_ERR_EVENT ),    \
                         true);
            g_pal_i2c_init_flag = 1;
        }
        else
        {
            cy_hal_status = (pal_status_t)PAL_STATUS_SUCCESS;
        }
    } while (FALSE);

    return (pal_status_t)cy_hal_status;
}

pal_status_t pal_i2c_deinit(const pal_i2c_t * p_i2c_context)
{
    if (i2c_taskhandle != NULL && (p_i2c_context == NULL))
    {
        vTaskDelete(i2c_taskhandle);
        i2c_taskhandle = NULL;
        vSemaphoreDelete(xIicSemaphoreHandle);
    }
    return (PAL_STATUS_SUCCESS);
}

pal_status_t pal_i2c_write(const pal_i2c_t * p_i2c_context, uint8_t * p_data, uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;

    //Acquire the I2C bus before read/write
    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context))
    {
        gp_pal_i2c_current_ctx = p_i2c_context;

        status = mtb_hal_i2c_controller_write(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel,
                p_i2c_context->slave_address,
                p_data,
                length,
                0,
                true);
        //Invoke the low level i2c master driver API to write to the bus
        if (CY_RSLT_SUCCESS != status)
        {
            //If I2C Master fails to invoke the write operation, invoke upper layer event handler with error.
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);
            
            //Release I2C Bus
            pal_i2c_release((void * )p_i2c_context);
        }
        else
        {
        	((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
        	                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;

            pal_i2c_release((void * )p_i2c_context);
        }
    }
    else
    {
        status = PAL_STATUS_I2C_BUSY;
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                        (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
    return (status);
}

pal_status_t pal_i2c_read(const pal_i2c_t * p_i2c_context, uint8_t * p_data, uint16_t length)
{
    pal_status_t status = PAL_STATUS_FAILURE;

    //Acquire the I2C bus before read/write
    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context))
    {
        gp_pal_i2c_current_ctx = p_i2c_context;

        //Invoke the low level i2c master driver API to read from the bus
        if (CY_RSLT_SUCCESS != mtb_hal_i2c_controller_read(((pal_psoc_i2c_t *)(p_i2c_context->p_i2c_hw_config))->i2c_controller_channel,
                                                                p_i2c_context->slave_address,
                                                                p_data,
                                                                length,
                                                                0,
                                                                true))
        {
            //If I2C Master fails to invoke the read operation, invoke upper layer event handler with error.
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);

            //Release I2C Bus
            pal_i2c_release((void * )p_i2c_context);
        }
        else
        {
        	((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
        	                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;
            pal_i2c_release((void * )p_i2c_context);
        }
    }
    else
    {
        status = PAL_STATUS_I2C_BUSY;
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                        (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
    return (status);
}

pal_status_t pal_i2c_set_bitrate(const pal_i2c_t * p_i2c_context, uint16_t bitrate)
{
	return PAL_STATUS_SUCCESS;
}

/**
* @}
*/
