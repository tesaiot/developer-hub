/******************************************************************************
* File Name:   subscriber_task.h
*
* Description: MQTT Subscriber Task - handles incoming messages from TESAIoT Platform
*
* This task processes all MQTT subscriptions and routes messages to appropriate
* handlers based on command type:
*
*   SUBSCRIBE_TO_TOPIC       - Subscribe to MQTT topics
*   UNSUBSCRIBE_FROM_TOPIC   - Unsubscribe from topics
*   UPDATE_DEVICE_CERTIFICATES - Write certificate to OPTIGA (CSR workflow)
*   UPDATE_PROTECTED_UPDATE_BUNDLE - Execute Protected Update with JSON bundle
*   CHECK_CERTIFICATE_RESPONSE - Platform response for cert existence check
*   UPLOAD_CERTIFICATE_RESPONSE - Platform response for cert upload
*   SYNC_CERTIFICATE_RESPONSE - Platform response for unified sync
*
* Uses FreeRTOS primitives:
*   subscriber_task_q - Queue for inter-task communication
*   device_cert_semaphore - Signals certificate write completion
*   data_received_event_group - Tracks Protected Update data arrival
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

#ifndef SUBSCRIBER_TASK_H_
#define SUBSCRIBER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "cy_mqtt_api.h"
#include "semphr.h"
#include "include/common/optiga_lib_common.h"  // For optiga_lib_status_t type

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for Subscriber Task. */
#define SUBSCRIBER_TASK_PRIORITY           (2U)  // Same as network tasks; Timer Task (3) must run for OPTIGA callbacks
#define SUBSCRIBER_TASK_STACK_SIZE         (1024U)  /* 1024 words (4KB) - Multi-Topic Protocol with JSON parsing */


#define MANIFEST_RECEIVED_BIT 					(1 << 0)
#define FRAGMENT_RECEIVED_BIT 					(1 << 1)
#define PROTECTED_UPDATE_COMPLETE_BIT			(1 << 2)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Subscriber Task. */
typedef enum
{
    SUBSCRIBE_TO_TOPIC,
    UNSUBSCRIBE_FROM_TOPIC,
    UPDATE_DEVICE_CERTIFICATES,
	UPDATE_PROTECTED_UPDATE_BUNDLE,  // Single JSON message with all Protected Update components (pubkey, manifest, fragments)

	// Smart Auto-Fallback responses
	CHECK_CERTIFICATE_RESPONSE,      // Platform response for certificate existence check
	UPLOAD_CERTIFICATE_RESPONSE,     // Platform response for certificate upload
	SYNC_CERTIFICATE_RESPONSE,       // Platform response for unified certificate sync

	// DEPRECATED (2025-10-22): Old 4-message protocol replaced by UPDATE_PROTECTED_UPDATE_BUNDLE
	UPDATE_DEVICE_MANIFEST,          // DEPRECATED - use UPDATE_PROTECTED_UPDATE_BUNDLE
	UPDATE_DEVICE_FRAGMENT,          // DEPRECATED - use UPDATE_PROTECTED_UPDATE_BUNDLE
	UPDATE_DEVICE_PUBLIC_KEY         // DEPRECATED - use UPDATE_PROTECTED_UPDATE_BUNDLE
} subscriber_cmd_t;

/* Struct to be passed via the subscriber task queue */
typedef struct{
    subscriber_cmd_t cmd;
    char* data;  // Non-const to allow freeing after use
    int data_size;
    bool need_free;  // Flag to indicate if data was allocated and needs freeing
} subscriber_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/
extern TaskHandle_t subscriber_task_handle;
extern QueueHandle_t subscriber_task_q;
extern uint32_t current_device_state;
extern SemaphoreHandle_t device_cert_semaphore;
extern volatile optiga_lib_status_t device_cert_write_status;
extern EventGroupHandle_t data_received_event_group;
extern SemaphoreHandle_t g_csr_publish_done_semaphore;  /* Signals when CSR publish completes - subscriber waits before subscribe */
extern volatile bool g_csr_workflow_active;  /* Flag: true when CSR workflow is running, false for direct MQTT test */
extern volatile bool g_protected_update_active;  /* Flag: true when Protected Update is executing, prevents Publisher task creation */
extern volatile bool g_protected_update_just_completed;  /* Flag: true after PU success - prevents Publisher task and ignores stray cmd=2 */
/*******************************************************************************
* Function Prototypes
********************************************************************************/
void subscriber_task(void *pvParameters);
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info);

#endif /* SUBSCRIBER_TASK_H_ */

/* [] END OF FILE */
