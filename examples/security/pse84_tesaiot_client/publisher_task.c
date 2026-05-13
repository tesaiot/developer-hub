/******************************************************************************
* File Name: publisher_task.c
*
* Description: This file contains the task that sets up the user button GPIO 
* for the publisher and publishes MQTT messages on the topic
* 'MQTT_PUB_TOPIC' to control a device that is actuated by the
* subscriber task. The file also contains the ISR that notifies
* the publisher task about the new device state to be published.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation. All rights reserved.
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
* integrated circuit products. Any reproduction, modification, translation,
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

/******************************************************************************
* TESAIoT Platform Integration
*******************************************************************************
* SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This file has been modified by the TESAIoT Platform Developer Team to add:
* - Queue-based publishing of CSR, status, and Protected Update requests
* - Large payload handling for certificate and manifest data
* - Printf size check to prevent hang on large payloads (>100 bytes)
* - Integration with TESAIoT Platform MQTT message routing
*
* Original work Copyright 2024-2025 Cypress Semiconductor Corporation (Infineon)
* Modifications Copyright 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This file is part of the TESAIoT AIoT Foundation Platform, developed in
* collaboration with Infineon Technologies AG for PSoC Edge E84 + OPTIGA Trust M.
*
* Contact: Wiroon Sriborrirux <sriborrirux@gmail.com>
*******************************************************************************/

#include "cybsp.h"
#include "FreeRTOS.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "retarget_io_init.h"
#include "tesaiot_config.h" // Debug labels and configuration
/******************************************************************************
* Macros
******************************************************************************/
/* Note: LABEL_PUBLISHER moved to tesaiot_config.h */

/* Interrupt priority for User Button Input. */
#define BTN1_INTERRUPT_PRIORITY (7U)

/* The maximum number of times each PUBLISH in this example will be retried. */
#define PUBLISH_RETRY_LIMIT (10U)

/* A PUBLISH message is retried if no response is received within this 
 * time (in milliseconds).
 */
#define PUBLISH_RETRY_MS (1000U)

/* Queue length of a message queue that is used to communicate with the
 * publisher task.
 * REDUCED from 3→1 to save RAM (~290 bytes). Sufficient for button events.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH (1U)

#define DEBOUNCE_TIME_MS (2U)

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

volatile bool button_debouncing = false;
volatile uint32_t button_debounce_timestamp = 0;

/* Handle of the queue holding the commands for the publisher task */
// Initialize to NULL to detect leaked queue from previous run
QueueHandle_t publisher_task_q = NULL;

/* Structure to store publish message information. */
cy_mqtt_publish_info_t publish_info =
{
 .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
 .topic = MQTT_PUB_TOPIC,
 .topic_len = (sizeof(MQTT_PUB_TOPIC) - 1),
 .retain = false,
 .dup = false
};

/* Interrupt config structure */
cy_stc_sysint_t intrCfg =
{
 .intrSrc = CYBSP_USER_BTN_IRQ,
 .intrPriority = BTN1_INTERRUPT_PRIORITY
};
/*******************************************************************************
* Function Name: button_interrupt_handler
********************************************************************************
* Summary:
* GPIO interrupt handler.
*
* Parameters:
* None
*
* Return:
* None
*
*******************************************************************************/
static void button_interrupt_handler(void)
{
 if (Cy_GPIO_GetInterruptStatus(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN))
 {
 Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
 NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);

 if (!(Cy_GPIO_Read(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN)))
 {
 if (!button_debouncing)
 {
 /* Set the debouncing flag */
 button_debouncing = true;

 /* Record the current timestamp */
 button_debounce_timestamp = (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
 }

 if (button_debouncing && (((xTaskGetTickCount() * portTICK_PERIOD_MS)) - button_debounce_timestamp >= DEBOUNCE_TIME_MS * portTICK_PERIOD_MS))
 {
 button_debouncing = false;

 BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 publisher_data_t publisher_q_data;

 /* Assign the publish command to be sent to the publisher task. */
 publisher_q_data.cmd = PUBLISH_MQTT_MSG;

 /* Send the command and data to publisher task over the queue */
 xQueueSendFromISR(publisher_task_q, &publisher_q_data, &xHigherPriorityTaskWoken);
 portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
 }
 }
 }

 /* CYBSP_USER_BTN1 (SW2) and CYBSP_USER_BTN2 (SW4) share the same port and
 * hence they share the same NVIC IRQ line. Since both the buttons are
 * configured for falling edge interrupt in the BSP, pressing any button
 * will trigger the execution of this ISR. Therefore, we must clear the
 * interrupt flag of the user button (CYBSP_USER_BTN2) to avoid issues in
 * case if user presses BTN2 by mistake.
 */
 Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
 NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);
}

/*******************************************************************************
* Function Name: user_button_init
********************************************************************************
*
* Summary:
* Initialize the button with Interrupt
*
* Parameters:
* None
*
* Return:
* None
*
*******************************************************************************/
void user_button_init(void)
{
 cy_en_sysint_status_t btn_interrupt_init_status ;

 /* CYBSP_USER_BTN1 (SW2) and CYBSP_USER_BTN2 (SW4) share the same port and
 * hence they share the same NVIC IRQ line. Since both are configured in the BSP
 * via the Device Configurator, the interrupt flags for both the buttons are set
 * right after they get initialized through the call to cybsp_init(). The flags
 * must be cleared otherwise the interrupt line will be constantly asserted.
 */
 Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT,CYBSP_USER_BTN1_PIN);
 Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT,CYBSP_USER_BTN2_PIN);
 NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
 NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

 /* Initialize the interrupt and register interrupt callback */
 btn_interrupt_init_status = Cy_SysInt_Init(&intrCfg, &button_interrupt_handler);

 /* Button interrupt initialization failed. Stop program execution. */
 if(CY_SYSINT_SUCCESS != btn_interrupt_init_status)
 {
 handle_app_error();
 }

 /* Enable the interrupt in the NVIC */
 NVIC_EnableIRQ(intrCfg.intrSrc);
}
/******************************************************************************
 * Function Name: publisher_init
 ******************************************************************************
 * Summary:
 * Function that initializes and sets-up the user button GPIO pin along with 
 * its interrupt.
 * 
 * Parameters:
 * void
 *
 * Return:
 * void
 *
 ******************************************************************************/
static void publisher_init(void)
{
 /* Initialize the user button GPIO */
 user_button_init();

 printf("\nPress the user button (SW2) to publish \"%s\"/\"%s\" on the topic '%s'...\n", 
 MQTT_DEVICE_ON_MESSAGE, MQTT_DEVICE_OFF_MESSAGE, publish_info.topic);
}

/******************************************************************************
 * Function Name: publisher_deinit
 ******************************************************************************
 * Summary:
 * Disables the user button interrupt
 *
 * Parameters:
 * void
 *
 * Return:
 * void
 *
 ******************************************************************************/
static void publisher_deinit(void)
{
 NVIC_DisableIRQ(intrCfg.intrSrc);
}

/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 * Task that sets up the user button GPIO for the publisher and publishes
 * MQTT messages to the broker. The user button init and deinit operations,
 * and the MQTT publish operation is performed based on commands sent by other
 * tasks and callbacks over a message queue.
 *
 * Parameters:
 * void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 * void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters)
{
 /* Status variable */
 cy_rslt_t result;

 publisher_data_t publisher_q_data;

 /* Command to the MQTT client task */
 mqtt_task_cmd_t mqtt_task_cmd;

 CY_UNUSED_PARAMETER(pvParameters);

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Task started\n", LABEL_PUBLISHER);
 #endif

 /* Initialize and set-up the user button GPIO */
 publisher_init();

 /* Delete old queue if exists (from previous MQTT session) */
 if (publisher_task_q != NULL) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Deleting old queue before creating new one\n", LABEL_PUBLISHER);
 #endif
 vQueueDelete(publisher_task_q);
 publisher_task_q = NULL;
 }

 /* Create a message queue to communicate with other tasks and callbacks */
 publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

 if (publisher_task_q == NULL) {
 printf("%s Queue creation failed - task self-deleting\n", LABEL_PUBLISHER);
 vTaskDelete(NULL);
 return;
 }

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Initialization complete\n", LABEL_PUBLISHER);
 #endif

 while (true)
 {
 /* Wait for commands from other tasks and callbacks. */
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_PUBLISHER " Waiting for queue message...\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Q-get cmd=%d len=%u\n", LABEL_PUBLISHER,
 publisher_q_data.cmd,
 (unsigned int)publisher_q_data.payload_len);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);
#endif
 switch(publisher_q_data.cmd)
 {
 case PUBLISHER_INIT:
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_PUBLISHER " Processing PUBLISHER_INIT command\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 /* Initialize and set-up the user button GPIO. */
 publisher_init();
 break;
 }

 case PUBLISHER_DEINIT:
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_PUBLISHER " Processing PUBLISHER_DEINIT command\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 /* Deinit the user button GPIO and corresponding interrupt. */
 publisher_deinit();
 break;
 }

 case PUBLISH_MQTT_MSG:
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s MQTT-MSG case\n", LABEL_PUBLISHER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);
#endif
 // Determine QoS based on topic suffix
 // "/ping" and "/csr" topics use QoS 0 (fire-and-forget, avoids PUBACK wait hang)
 // All other topics use QoS 1 (guaranteed delivery)
 cy_mqtt_qos_t qos = CY_MQTT_QOS1; // Default: QoS 1
 if (strstr(publisher_q_data.topic, "/ping") != NULL ||
     strstr(publisher_q_data.topic, "/csr") != NULL) {
 qos = CY_MQTT_QOS0; // QoS 0 for ping/CSR messages (avoid cy_mqtt_publish hang)
 }

 /* Create local publish_info struct to avoid global variable issues */
 cy_mqtt_publish_info_t local_publish_info = {
 .qos = qos, // QoS 0 for /ping, QoS 1 for others
 .topic = publisher_q_data.topic,
 .topic_len = publisher_q_data.topic_len,
 .payload = publisher_q_data.data,
 .payload_len = publisher_q_data.payload_len,
 .retain = false,
 .dup = false
 };

#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s topic_len=%u\n", LABEL_PUBLISHER, (unsigned int)local_publish_info.topic_len);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);
#endif

 // Do not print large payloads (>100 bytes) as %s string - printf will hang
 // For CSR workflow, 791-byte JSON payload would block printf forever
 if (local_publish_info.payload_len > 100) {
 printf("\nPublisher: Publishing %u bytes on '%s' (payload too large to display)\n",
 (unsigned int)local_publish_info.payload_len, local_publish_info.topic);
 } else {
 printf("\nPublisher: Publishing '%s' (%u bytes) on '%s'\n",
 (char *) local_publish_info.payload, (unsigned int)local_publish_info.payload_len, local_publish_info.topic);
 }
 fflush(stdout);

#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 /* Debug: Verify payload pointer and contents before cy_mqtt_publish */
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s payload ptr=%p len=%u\n", LABEL_PUBLISHER,
 local_publish_info.payload,
 (unsigned int)local_publish_info.payload_len);
 printf("%s Payload hex: ", LABEL_PUBLISHER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 for (size_t i = 0; i < local_publish_info.payload_len && i < 32; i++) {
 printf("%02X ", ((unsigned char*)local_publish_info.payload)[i]);
 }
 printf("\n");

 /* Verify all struct fields before publish */
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s BEFORE cy_mqtt_publish:\n", LABEL_PUBLISHER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 printf(" qos=%d topic=%s topic_len=%u\n",
 (int)local_publish_info.qos,
 local_publish_info.topic,
 (unsigned int)local_publish_info.topic_len);
 printf(" payload=%p payload_len=%u\n",
 local_publish_info.payload,
 (unsigned int)local_publish_info.payload_len);
 printf(" retain=%d dup=%d\n",
 (int)local_publish_info.retain,
 (int)local_publish_info.dup);
 printf(" mqtt_connection=%p\n", mqtt_connection);
 fflush(stdout);
#endif

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s >>> Calling cy_mqtt_publish() now...\n", LABEL_PUBLISHER);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);
 vTaskDelay(pdMS_TO_TICKS(100)); // Give UART time to transmit

 // DEBUG: Verify payload content is in memory before sending
 // Server suggested: check if payload data is actually accessible
 printf("%s PAYLOAD-VERIFY: ptr=%p len=%u\n", LABEL_PUBLISHER,
        local_publish_info.payload, (unsigned int)local_publish_info.payload_len);
 if (local_publish_info.payload != NULL && local_publish_info.payload_len > 0)
 {
     const unsigned char *pdata = (const unsigned char *)local_publish_info.payload;
     printf("%s PAYLOAD-VERIFY: First 32 bytes: ", LABEL_PUBLISHER);
     for (int i = 0; i < 32 && i < (int)local_publish_info.payload_len; i++)
     {
         printf("%02X ", pdata[i]);
     }
     printf("\n");
     printf("%s PAYLOAD-VERIFY: Last 32 bytes: ", LABEL_PUBLISHER);
     int start = (int)local_publish_info.payload_len - 32;
     if (start < 0) start = 0;
     for (int i = start; i < (int)local_publish_info.payload_len; i++)
     {
         printf("%02X ", pdata[i]);
     }
     printf("\n");
     fflush(stdout);
 }

 /* Memory barriers for dual-core cache coherency (M55/M33) */
 __DSB();
 __DMB();
 __ISB();

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Calling cy_mqtt_publish()...\n", LABEL_PUBLISHER);
 fflush(stdout);
 #endif

 /* Allow TCP ACKs to be processed before publish */
 vTaskDelay(pdMS_TO_TICKS(500));

 UBaseType_t original_priority = uxTaskPriorityGet(NULL);
 result = cy_mqtt_publish(mqtt_connection, &local_publish_info);

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s cy_mqtt_publish returned: 0x%08X\n", LABEL_PUBLISHER, (unsigned int)result);
 fflush(stdout);
 #endif

 /* Signal subscriber task that publish is complete */
 if (g_csr_publish_done_semaphore != NULL)
 {
 xSemaphoreGive(g_csr_publish_done_semaphore);
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Signaled publish done semaphore\n", LABEL_PUBLISHER);
 fflush(stdout);
 #endif
 }

 vTaskPrioritySet(NULL, original_priority);

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s <<< cy_mqtt_publish() returned: 0x%08X\n", LABEL_PUBLISHER, (unsigned int)result);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout);

 /* Allow TCP stack time to transmit large payloads */
 vTaskDelay(pdMS_TO_TICKS(2000));

 if (result != CY_RSLT_SUCCESS)
 {
 printf(" Publisher: MQTT Publish failed with error 0x%0X.\n\n", (int)result);

 /* Communicate the publish failure with the the MQTT
 * client task.
 */
 mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
 xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
 }
 else
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 printf(" Publisher: MQTT Publish SUCCESS.\n");
 fflush(stdout); // Ensure message printed before context switch
#endif
 }

 // Free payload after publish if requested
 // Payload was allocated with pvPortMalloc() in publish_request_protected_update()
 // Must free with vPortFree() to prevent memory leak
 if (publisher_q_data.free_after_publish && publisher_q_data.data)
 {
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Freeing payload (%u bytes)\n", LABEL_PUBLISHER, (unsigned int)publisher_q_data.payload_len);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 fflush(stdout); // Ensure message printed before context switch
#endif
 vPortFree(publisher_q_data.data);
 publisher_q_data.data = NULL; // Safety - prevent double free
 }

 break;
 }
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 default:
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_PUBLISHER " Unknown command: %d\n", publisher_q_data.cmd);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 break;
#endif
 }
 }
#if TESAIOT_DEBUG_PUBLISHER_ENABLED
 else
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_PUBLISHER " xQueueReceive returned pdFALSE (should never happen with portMAX_DELAY)\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 }
#endif
 }
}



/* [] END OF FILE */
