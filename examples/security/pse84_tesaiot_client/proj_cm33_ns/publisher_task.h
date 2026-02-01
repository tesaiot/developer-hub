/******************************************************************************
* File Name:   publisher_task.h
*
* Description: MQTT Publisher Task - handles all outgoing messages to TESAIoT Platform
*
* This task processes the publisher_task_q queue and publishes MQTT messages.
* Key features:
*   - Async publishing decoupled from OPTIGA operations
*   - Retry logic with configurable max_attempts and retry_delay
*   - Memory management with free_after_publish flag
*   - Thread-safe queue-based inter-task communication
*
* Message types:
*   PUBLISHER_INIT  - Initialize publisher
*   PUBLISHER_DEINIT - Cleanup publisher
*   PUBLISH_MQTT_MSG - Publish payload to specified topic
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
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

#ifndef PUBLISHER_TASK_H_
#define PUBLISHER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for Button Task. */
#define PUBLISHER_TASK_PRIORITY               (2U)
#define PUBLISHER_TASK_STACK_SIZE             (1024U * 2U)  /* 2048 words (8KB) - Same as Infineon template */

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Publisher Task. */
typedef enum
{
    PUBLISHER_INIT,
    PUBLISHER_DEINIT,
    PUBLISH_MQTT_MSG
} publisher_cmd_t;

/* Struct to be passed via the publisher task queue */
typedef struct
{
    publisher_cmd_t cmd;
    char *data;
    size_t payload_len;
    char topic[96];
    size_t topic_len;  // Pre-calculated topic length to avoid strlen() hang
    bool free_after_publish;
    uint8_t attempt;
    uint8_t max_attempts;
    TickType_t retry_delay_ticks;
} publisher_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/
extern TaskHandle_t publisher_task_handle;
extern QueueHandle_t publisher_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void publisher_task(void *pvParameters);

#endif /* PUBLISHER_TASK_H_ */

/* [] END OF FILE */
