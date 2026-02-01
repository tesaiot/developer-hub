/******************************************************************************
* File Name: subscriber_task.c
*
* Description: This file contains the task that initializes the user LED GPIO,
* subscribes to the topic 'MQTT_SUB_TOPIC', and actuates the user LED
* based on the notifications received from the MQTT subscriber
* callback.
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
* - Processing of certificate, manifest, and fragment messages from TESAIoT Platform
* - Certificate write to OPTIGA Trust M OID 0xE0E1 with verification
* - Protected Update manifest and fragment handling
* - Certificate lifecycle state reset after successful certificate renewal
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
#include "string.h"
#include "FreeRTOS.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include "cy_json_parser.h" // JSON parser for Protected Update bundle
#include "tesaiot_optiga.h" // Certificate lifecycle management functions
#include "optiga_trust_helpers.h" // For extern volatile optiga_lib_status
#include "tesaiot_config.h" // Debug labels and configuration
#include "semphr.h"
#include "event_groups.h"
#include "mbedtls/base64.h" // Base64 decode for public key
#include "mbedtls/sha256.h" // SHA256 for debug digest calculation
#include "include/optiga_util.h" // OPTIGA Trust M utilities
#include "include/common/optiga_lib_common.h" // OPTIGA library common definitions
#include "tesaiot_optiga_core.h" // Persistent OPTIGA instance manager (Best Practice)

/******************************************************************************
* Macros
******************************************************************************/
/* Note: LABEL_SUBSCRIBER moved to tesaiot_config.h */

/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES (3U)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS (1000U)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT (1U)

/* Queue length of a message queue that is used to communicate with the
 * subscriber task.
 */
#define SUBSCRIBER_TASK_QUEUE_LENGTH (20U) // Increased to 20 to buffer all messages during Trust M write

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

SemaphoreHandle_t device_cert_semaphore;
EventGroupHandle_t data_received_event_group;
SemaphoreHandle_t g_csr_publish_done_semaphore;  /* CSR publish done signal - subscriber waits before subscribe */
volatile bool g_csr_workflow_active = false;  /* Flag: true when CSR workflow running - subscriber waits for semaphore */
volatile bool g_protected_update_active = false;  /* Flag: true when Protected Update executing - blocks Publisher task */
volatile bool g_protected_update_just_completed = false;  /* Flag: true after PU success - prevents Publisher task and ignores stray cmd=2 */

/* Shared status variable for certificate write result (used by vCSRWorkflow task) */
volatile optiga_lib_status_t device_cert_write_status = OPTIGA_LIB_SUCCESS;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q = NULL;

extern uint8_t *dev_cert_raw;
extern size_t dev_cert_raw_len;
extern uint8_t *ecc_key_final_fragment_array;
extern uint8_t *manifest_ecc_key;
extern size_t ecc_key_final_fragment_array_length;
extern size_t manifest_ecc_key_length;

extern uint8_t *pubkey;
extern size_t pubkey_length;

/* Configure the subscription information structure. */
static cy_mqtt_subscribe_info_t subscribe_info =
{
 .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
 .topic = MQTT_SUB_TOPIC,
 .topic_len = (sizeof(MQTT_SUB_TOPIC) - 1)
};

/******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Static variables for OPTIGA Trust M async operations */
static volatile optiga_lib_status_t trust_m_async_status = OPTIGA_LIB_BUSY;
static SemaphoreHandle_t trust_m_write_semaphore = NULL;

/* Debug counters for callback invocations (NEVER use printf in callback!) */
static volatile uint32_t callback_count_total = 0;
static volatile uint32_t callback_count_cert = 0;
static volatile uint32_t callback_count_manifest = 0;
static volatile uint32_t callback_count_fragment = 0;
static volatile uint32_t callback_count_pubkey = 0;
static volatile uint32_t callback_count_unknown = 0;

/* Smart Auto-Fallback response tracking variables */
SemaphoreHandle_t check_certificate_response_semaphore = NULL;
SemaphoreHandle_t upload_certificate_response_semaphore = NULL;
volatile bool platform_has_certificate = false;
volatile bool certificate_upload_success = false;
volatile bool check_certificate_response_received = false;
volatile bool upload_certificate_response_received = false;

/* Smart Auto-Fallback Unified sync_certificate variables */
SemaphoreHandle_t sync_certificate_response_semaphore = NULL;
volatile bool sync_certificate_response_received = false;
volatile bool certificate_sync_success = false;

/******************************************************************************
 * Function Name: optiga_trust_m_callback
 ******************************************************************************
 * Summary:
 * Callback function for OPTIGA Trust M async operations.
 * Signals semaphore when operation completes to unblock waiting task.
 *
 * Parameters:
 * void *context : Context pointer (unused)
 * optiga_lib_status_t return_status : Operation result status
 *
 * Return:
 * void
 *
 ******************************************************************************/
static void optiga_trust_m_callback(void *context, optiga_lib_status_t return_status)
{
 (void)context;
 trust_m_async_status = return_status;

 if (trust_m_write_semaphore != NULL) {
 BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 xSemaphoreGiveFromISR(trust_m_write_semaphore, &xHigherPriorityTaskWoken);
 portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
 }
}

/******************************************************************************
 * Function Name: subscribe_to_topic
 ******************************************************************************
 * Summary:
 * Function that subscribes to the MQTT topic specified by the macro
 * 'MQTT_SUB_TOPIC'. This operation is retried a maximum of
 * 'MAX_SUBSCRIBE_RETRIES' times with interval of
 * 'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 * void
 *
 * Return:
 * void
 *
 ******************************************************************************/
static void subscribe_to_topic(void)
{
 /* Status variable */
 cy_rslt_t result = CY_RSLT_SUCCESS;

 /* Command to the MQTT client task */
 mqtt_task_cmd_t mqtt_task_cmd;

 /* Subscribe with the configured parameters. */
 for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
 {
 printf("[Sub] >>> cy_mqtt_subscribe() retry=%lu <<<\n", (unsigned long)retry_count);
 fflush(stdout);
 result = cy_mqtt_subscribe(mqtt_connection, &subscribe_info, SUBSCRIPTION_COUNT);
 printf("[Sub] <<< cy_mqtt_subscribe() returned 0x%08lX <<<\n", (unsigned long)result);
 fflush(stdout);
 if (result == CY_RSLT_SUCCESS)
 {
 printf("\nMQTT client subscribed to the topic '%.*s' successfully.\n",
 subscribe_info.topic_len, subscribe_info.topic);
 break;
 }

 vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
 }

 if (CY_RSLT_SUCCESS != result)
 {
 printf("\nMQTT Subscribe failed with error 0x%0X after %d retries...\n\n",
 (int)result, MAX_SUBSCRIBE_RETRIES);

 /* Notify the MQTT client task about the subscription failure */
 mqtt_task_cmd = HANDLE_MQTT_SUBSCRIBE_FAILURE;
 xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
 }
}

/******************************************************************************
 * Function Name: unsubscribe_from_topic
 ******************************************************************************
 * Summary:
 * Function that unsubscribes from the topic specified by the macro
 * 'MQTT_SUB_TOPIC'.
 *
 * Parameters:
 * void
 *
 * Return:
 * void
 *
 ******************************************************************************/
static void unsubscribe_from_topic(void)
{
 cy_rslt_t result = cy_mqtt_unsubscribe(mqtt_connection,
 (cy_mqtt_unsubscribe_info_t *) &subscribe_info,
 SUBSCRIPTION_COUNT);

 if (CY_RSLT_SUCCESS != result)
 {
 printf("MQTT Unsubscribe operation failed with error 0x%0X!\n", (int)result);
 }
}

/******************************************************************************
 * Protected Update JSON Parsing - Context and Callback
 ******************************************************************************
 * Cypress JSON Parser uses callback-based parsing. The callback is invoked
 * for each key-value pair found in the JSON.
 ******************************************************************************/

/* Context structure to collect parsed JSON fields from Protected Update bundle */
typedef struct {
	char *signing_cert_b64;      /* X.509 Certificate DER (base64 encoded) - ~580 bytes decoded */
	uint16_t signing_cert_len;   /* Length of base64 string (~773 chars) */
	char *manifest_b64;
	uint16_t manifest_len;
	char *fragment_0_b64;
	uint16_t fragment_0_len;
	char *fragment_1_b64;
	uint16_t fragment_1_len;
	char *fragment_2_b64;
	uint16_t fragment_2_len;
	uint8_t fragment_count;
} protected_update_ctx_t;

/* JSON callback function for Protected Update bundle parsing */
static cy_rslt_t protected_update_json_callback(cy_JSON_object_t* json_obj, void *arg)
{
	protected_update_ctx_t *ctx = (protected_update_ctx_t *)arg;
	char *key = json_obj->object_string;
	uint8_t key_len = json_obj->object_string_length;
	char *val = json_obj->value;
	uint16_t val_len = json_obj->value_length;

	// Parse string fields (signing_certificate, manifest, fragment_0)
	// Field names aligned with TESAIoT Platform Protected Update API v2.11
	if (json_obj->value_type == JSON_STRING_TYPE) {
		// signing_certificate: X.509 Certificate DER (~580 bytes, base64 encoded ~773 chars)
		if (key_len == 19 && strncmp(key, "signing_certificate", 19) == 0) {
			ctx->signing_cert_b64 = val;
			ctx->signing_cert_len = val_len;
		}
		// manifest: COSE_Sign1 protected update manifest (base64 encoded)
		else if (key_len == 8 && strncmp(key, "manifest", 8) == 0) {
			ctx->manifest_b64 = val;
			ctx->manifest_len = val_len;
		}
		// fragment_0: Encrypted certificate payload (base64 encoded)
		else if (key_len == 10 && strncmp(key, "fragment_0", 10) == 0) {
			ctx->fragment_0_b64 = val;
			ctx->fragment_0_len = val_len;
		}
	}
	// Parse number fields (fragment_count)
	else if (json_obj->value_type == JSON_NUMBER_TYPE) {
		if (key_len == 14 && strncmp(key, "fragment_count", 14) == 0) {
			ctx->fragment_count = (uint8_t)json_obj->intval;
		}
	}

	return CY_RSLT_SUCCESS;
}

/******************************************************************************
 * Function Name: subscriber_task
 ******************************************************************************
 * Summary:
 * Task that sets up the user LED GPIO, subscribes to the specified MQTT topic,
 * and controls the user LED based on the received commands over the message 
 * queue. The task can also unsubscribe from the topic based on the commands
 * via the message queue.
 *
 * Parameters:
 * void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 * void
 *
 ******************************************************************************/
void subscriber_task(void *pvParameters)
{
 /* FIRST LINE DEBUG - if we don't see this, task crashed on entry (stack overflow?) */
 printf("[SUB-TASK] >>> ENTRY <<<\n");
 fflush(stdout);

 printf("\n");
 printf("================================================================================\n");
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " *** TASK ENTRY POINT - Task is now running! ***\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 printf("================================================================================\n");

 subscriber_data_t subscriber_q_data;

 /* To avoid compiler warnings */
 (void) pvParameters;

 /* Create queue FIRST before subscribing (callback may fire during subscribe!)
  * Delete old queue if exists (from previous MQTT session) to prevent heap exhaustion */
 if (subscriber_task_q != NULL) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("%s Deleting old queue (0x%08X) before creating new one\n", LABEL_SUBSCRIBER, (unsigned int)subscriber_task_q);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 vQueueDelete(subscriber_task_q);
 subscriber_task_q = NULL;
 }

 subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

 /* Check if queue creation succeeded */
 if (subscriber_task_q == NULL) {
 #if TESAIOT_DEBUG_ERROR_ENABLED
 printf("%s Failed to create subscriber queue! Heap may be exhausted.\n", LABEL_FATAL);
 #endif /* TESAIOT_DEBUG_ERROR_ENABLED */
 vTaskDelete(NULL); // Cannot continue without queue
 }

#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Queue created FIRST at 0x%08X (before subscribe)\n", (unsigned int)subscriber_task_q);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 /* CRITICAL: Create semaphore for certificate write completion signaling */
 device_cert_semaphore = xSemaphoreCreateBinary();
 if (device_cert_semaphore == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create device_cert_semaphore!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL); // Cannot proceed without semaphore
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Semaphore created at 0x%08X (for certificate write signaling)\n", (unsigned int)device_cert_semaphore);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 /* Create event group for Protected Update workflow signaling */
 if (data_received_event_group == NULL)
 {
 data_received_event_group = xEventGroupCreate();
 if (data_received_event_group == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create data_received_event_group!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL); // Cannot proceed without event group
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Event group created at 0x%08X (for Protected Update signaling)\n", (unsigned int)data_received_event_group);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 }

 /* Create semaphores for Smart Auto-Fallback response signaling */
 check_certificate_response_semaphore = xSemaphoreCreateBinary();
 if (check_certificate_response_semaphore == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create check_certificate_response_semaphore!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL);
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Smart Auto-Fallback check_certificate semaphore created at 0x%08X\n", (unsigned int)check_certificate_response_semaphore);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 upload_certificate_response_semaphore = xSemaphoreCreateBinary();
 if (upload_certificate_response_semaphore == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create upload_certificate_response_semaphore!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL);
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Smart Auto-Fallback upload_certificate semaphore created at 0x%08X\n", (unsigned int)upload_certificate_response_semaphore);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 /* Create semaphore for unified sync_certificate response */
 sync_certificate_response_semaphore = xSemaphoreCreateBinary();
 if (sync_certificate_response_semaphore == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create sync_certificate_response_semaphore!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL);
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Smart Auto-Fallback sync_certificate semaphore created at 0x%08X\n", (unsigned int)sync_certificate_response_semaphore);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 /* Create semaphore for CSR publish synchronization
  * Subscriber waits for CSR publish to complete before calling subscribe_to_topic()
  * to prevent MQTT event thread mutex deadlock during cy_mqtt_publish()
  */
 g_csr_publish_done_semaphore = xSemaphoreCreateBinary();
 if (g_csr_publish_done_semaphore == NULL)
 {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to create g_csr_publish_done_semaphore!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL);
 }
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " CSR publish sync semaphore created at 0x%08X\n", (unsigned int)g_csr_publish_done_semaphore);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 /* Initialize persistent OPTIGA Trust M instance (Best Practice)
 * This creates a single global instance that will be reused for all operations
 * to avoid instance state corruption and critical section deadlocks.
 * Root cause fixed: Multiple create/destroy cycles caused dangling pointer access.
 */
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " Initializing persistent OPTIGA Trust M instance...\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 if (!optiga_manager_init(optiga_trust_m_callback, NULL)) {
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " ERROR: Failed to initialize OPTIGA manager!\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 vTaskDelete(NULL); // Cannot proceed without OPTIGA instance
 }
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " OPTIGA manager initialized successfully\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 /* Wait for CSR publish to complete before subscribing (only in CSR workflow mode)
  * MQTT connection test uses direct publish and skips this wait.
  * CSR workflow sets g_csr_workflow_active=true to enable semaphore synchronization.
  */
 if (g_csr_workflow_active)
 {
     #if TESAIOT_DEBUG_VERBOSE_ENABLED
     printf(LABEL_SUBSCRIBER " CSR workflow active - waiting for CSR publish (semaphore)...\n");
     #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

     BaseType_t sem_result = xSemaphoreTake(g_csr_publish_done_semaphore, pdMS_TO_TICKS(30000));
     if (sem_result != pdTRUE)
     {
         #if TESAIOT_DEBUG_VERBOSE_ENABLED
         printf(LABEL_SUBSCRIBER " WARNING: CSR publish semaphore timeout (30s) - proceeding anyway\n");
         #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
     }
     else
     {
         #if TESAIOT_DEBUG_VERBOSE_ENABLED
         printf(LABEL_SUBSCRIBER " CSR publish completed - OK to subscribe now\n");
         #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
     }
 }
 else
 {
     #if TESAIOT_DEBUG_VERBOSE_ENABLED
     printf(LABEL_SUBSCRIBER " Direct mode - skipping CSR semaphore wait, subscribing immediately\n");
     #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 }

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " About to call subscribe_to_topic()...\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 /* Subscribe to the specified MQTT topic. */
 subscribe_to_topic();

 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf(LABEL_SUBSCRIBER " subscribe_to_topic() returned successfully!\n");
 printf(LABEL_SUBSCRIBER " Task initialization complete, entering main loop...\n");
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 printf("================================================================================\n");

 // No fflush(), no delays - just simple printf
 // fflush() blocks forever when UART buffer full
 // vTaskDelay() before printf causes MQTT timeout and disconnect
 while (true)
 {
 /* Wait for commands with portMAX_DELAY - no timeout needed */
 BaseType_t queue_result = xQueueReceive(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);

 if (queue_result != pdTRUE) {
 // This should never happen with portMAX_DELAY, but handle gracefully
 continue;
 }

 // Message received - printf IMMEDIATELY (no delays!)
 // Callback completed -> UART should be clear
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 printf("\n%s cmd=%d sz=%d\n", LABEL_SUBSCRIBER, subscriber_q_data.cmd, subscriber_q_data.data_size);
 printf("%s Callback stats: total=%lu cert=%lu manifest=%lu fragment=%lu pubkey=%lu unknown=%lu\n", LABEL_SUBSCRIBER,
 (unsigned long)callback_count_total,
 (unsigned long)callback_count_cert,
 (unsigned long)callback_count_manifest,
 (unsigned long)callback_count_fragment,
 (unsigned long)callback_count_pubkey,
 (unsigned long)callback_count_unknown);
 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 switch(subscriber_q_data.cmd)
 {
 case SUBSCRIBE_TO_TOPIC:
 {
 subscribe_to_topic();
 break;
 }

 case UNSUBSCRIBE_FROM_TOPIC:
 {
 unsubscribe_from_topic();
 break;
 }

 case UPDATE_DEVICE_CERTIFICATES:
 {
 	// IGNORE: After Protected Update completes, Server may send stray cmd=2 with empty data
 	// This prevents race condition where multiple tasks try to access OPTIGA simultaneously
 	if (g_protected_update_just_completed) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s Ignoring cmd=2 after Protected Update completed\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}

 	// IGNORE: Empty certificate data (sz=0) - nothing to write
 	if (subscriber_q_data.data_size == 0) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s Ignoring empty certificate (sz=0)\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}

 	// MINIMAL printf - no fflush() to prevent blocking
 	if (!subscriber_q_data.data) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: NULL cert data\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}

 	char *cert_data = (char*)pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!cert_data) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc cert fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}

 	memcpy(cert_data, subscriber_q_data.data, subscriber_q_data.data_size);
 	cert_data[subscriber_q_data.data_size] = '\0';
 	__DSB(); __DMB(); __ISB();

 	dev_cert_raw = (uint8_t *)cert_data;
 	dev_cert_raw_len = subscriber_q_data.data_size;

 	// Write certificate to OPTIGA Trust M
 	optiga_lib_status_t status = write_device_certificate_and_verify();
 	if (OPTIGA_LIB_SUCCESS == status) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s Cert OK\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		fflush(stdout);
#endif
 		tesaiot_reset_fallback_state();
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s Cert FAIL: 0x%04X\n", LABEL_SUBSCRIBER, status);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		fflush(stdout);
 	}

 	// Free certificate buffer after write operation
 	// write_device_certificate_and_verify() copies data to local buffer first,
 	// so safe to free global pointer after function returns
 	vPortFree(cert_data);
 	dev_cert_raw = NULL;
 	dev_cert_raw_len = 0;

 	// Store status for vCSRWorkflow task to check
 	device_cert_write_status = status;
 	__DSB(); __DMB(); __ISB(); // Memory barriers for cache coherency

 	// Signal completion (status already stored)
 	xSemaphoreGive(device_cert_semaphore);
 break;
 }

 case UPDATE_PROTECTED_UPDATE_BUNDLE:
 {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Processing Protected Update bundle\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 	// CRITICAL: Set flag EARLY to block Publisher task creation during ENTIRE workflow
 	// This includes Trust Anchor write, READBACK verification, and Protected Update
 	// Without this, Publisher task may be created during READBACK delay causing interference
 	g_protected_update_active = true;
#if 0  /* TRACE messages disabled to reduce debug output during Protected Update */
 	printf("%s [TRACE-1] Flag set\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
#endif

 	// State update: Processing JSON bundle
 	trustm_update_state(TRUSTM_STATE_PROCESSING_JSON_BUNDLE, NULL, NULL);
#if 0  /* TRACE messages disabled */
 	printf("%s [TRACE-2] State updated\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
#endif

 	if (!subscriber_q_data.data) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: NULL bundle data\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		trustm_update_state(TRUSTM_STATE_PROTECTED_UPDATE_FAILED, NULL, "NULL bundle data");
 		break;
 	}

 	// Copy JSON buffer before parsing to prevent dangling pointers.
 	// Cypress JSON Parser callbacks store pointers into original buffer.
 	// MQTT library may reuse/overwrite buffer for next packet,
 	// so we copy to ensure data remains valid during entire processing.
 	char *json_copy = (char*) pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!json_copy) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc JSON copy fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		break;
 	}
 	memcpy(json_copy, subscriber_q_data.data, subscriber_q_data.data_size);
 	json_copy[subscriber_q_data.data_size] = '\0'; // Null-terminate
 	__DSB(); __DMB(); __ISB(); // Memory barriers for cache coherency
#if 0  /* TRACE messages disabled */
 	printf("%s [TRACE-3] JSON buffer copied (%d bytes)\n", LABEL_SUBSCRIBER, subscriber_q_data.data_size);
 	fflush(stdout);
#endif

 	// Initialize context to collect parsed JSON fields
 	protected_update_ctx_t parse_ctx = {0};

 	// Register callback and parse JSON using Cypress JSON Parser
#if 0  /* TRACE messages disabled */
 	printf("%s [TRACE-4] Parsing JSON...\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
#endif
 	cy_JSON_parser_register_callback(protected_update_json_callback, &parse_ctx);
 	cy_rslt_t result = cy_JSON_parser(json_copy, subscriber_q_data.data_size);
#if 0  /* TRACE messages disabled */
 	printf("%s [TRACE-5] JSON parse result: 0x%08lX\n", LABEL_SUBSCRIBER, (unsigned long)result);
 	fflush(stdout);
#endif

 	if (result != CY_RSLT_SUCCESS) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: JSON parse failed: 0x%08lX\n", LABEL_SUBSCRIBER, (unsigned long)result);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		vPortFree(json_copy);
 		break;
 	}

 	// Validate required fields
 	if (!parse_ctx.signing_cert_b64 || !parse_ctx.manifest_b64 || !parse_ctx.fragment_0_b64) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Missing required fields\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		vPortFree(json_copy);
 		break;
 	}

#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Fragment count: %u\n", LABEL_SUBSCRIBER, parse_ctx.fragment_count);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 	// ========================================
 	// STEP 1: Process signing certificate (write to Trust M OID 0xE0E8)
 	// ========================================
 	printf("%s STEP 1: Processing signing certificate...\n", LABEL_SUBSCRIBER);
 	fflush(stdout);

#if 0  /* Signing certificate dump disabled to reduce debug output - causes UART buffer issues */
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Starting signing certificate decode...\n", LABEL_SUBSCRIBER);
 	printf("%s DEBUG: signing_cert_len=%u\n", LABEL_SUBSCRIBER, parse_ctx.signing_cert_len);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 	// Print ENTIRE signing certificate string to find invalid character
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Full signing certificate string:\n\"", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	for (int i = 0; i < parse_ctx.signing_cert_len; i++) {
 		printf("%c", parse_ctx.signing_cert_b64[i]);
 	}
 	printf("\"\n");

 	// Also print as hex dump for precise diagnosis
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Hex dump:\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	for (int i = 0; i < parse_ctx.signing_cert_len; i++) {
 		printf("%02X ", (uint8_t)parse_ctx.signing_cert_b64[i]);
 		if ((i + 1) % 16 == 0) printf("\n");
 	}
 	printf("\n");
#endif
#endif  /* End signing certificate dump disabled */

 	size_t pubkey_decoded_len = 0;
 	uint8_t *pubkey_der = NULL;
 	int decode_ret = mbedtls_base64_decode(NULL, 0, &pubkey_decoded_len,
 	 (uint8_t*)parse_ctx.signing_cert_b64,
 	 parse_ctx.signing_cert_len);
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Base64 decode size check: %d, len=%u\n", LABEL_SUBSCRIBER, decode_ret, (unsigned int)pubkey_decoded_len);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 	if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s DEBUG: Allocating %u bytes for signing certificate...\n", LABEL_SUBSCRIBER, (unsigned int)pubkey_decoded_len);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 		pubkey_der = (uint8_t*) pvPortMalloc(pubkey_decoded_len);
 		if (pubkey_der) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s DEBUG: pvPortMalloc OK, decoding...\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 			decode_ret = mbedtls_base64_decode(pubkey_der, pubkey_decoded_len,
 			 &pubkey_decoded_len,
 			 (uint8_t*)parse_ctx.signing_cert_b64,
 			 parse_ctx.signing_cert_len);
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s DEBUG: Decode complete: ret=%d, len=%u\n", LABEL_SUBSCRIBER, decode_ret, (unsigned int)pubkey_decoded_len);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 		} else {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s DEBUG: pvPortMalloc FAILED!\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 		}
 	}

 	if (decode_ret != 0 || !pubkey_der) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: signing certificate B64 decode fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		if (pubkey_der) vPortFree(pubkey_der);
 		vPortFree(json_copy);
 		break;
 	}

 	// Create semaphore for Trust M async operations (first time only)
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Creating semaphore...\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	if (trust_m_write_semaphore == NULL) {
 		trust_m_write_semaphore = xSemaphoreCreateBinary();
 		if (!trust_m_write_semaphore) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: semaphore fail\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag on early exit
 			vPortFree(pubkey_der);
 			vPortFree(json_copy);
 			break;
 		}
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s DEBUG: Semaphore created\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	} else {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s DEBUG: Semaphore already exists\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	}

 	// Write Trust Anchor to OID 0xE0E8
 	// Best Practice: Use persistent OPTIGA instance (no create/destroy)
 	printf("%s Acquiring OPTIGA instance...\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: Acquiring persistent OPTIGA instance...\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	trust_m_async_status = OPTIGA_LIB_BUSY;
 	optiga_util_t *me_util = optiga_manager_acquire();
 	printf("%s OPTIGA acquired: %s\n", LABEL_SUBSCRIBER, me_util ? "OK" : "FAILED");
 	fflush(stdout);
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s DEBUG: optiga_manager_acquire returned: %p\n", LABEL_SUBSCRIBER, me_util);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	if (!me_util) {
 		printf("%s ERROR: Failed to acquire OPTIGA instance\n", LABEL_SUBSCRIBER);
 		g_protected_update_active = false;  // Reset flag on early exit
 		vPortFree(pubkey_der);
 		vPortFree(json_copy);
 		break;
 	}

 	// ========================================
 	// STEP 1: Write Trust Anchor metadata AND data
 	// ========================================
 	// NOTE:
 	//
 	// ERROR 0x8029 ROOT CAUSE: Trust M requires OID 0xE0E8 to have
 	// metadata type 0x11 (Trust Anchor) for manifest verification!
 	//
 	// Previous version SKIPPED metadata write (wrongly assumed already set)
 	// Result: Metadata read failed (0x0204) -> Trust M refuses OID as Trust Anchor!
 	//
 	// SOLUTION: Write metadata type 0x11 BEFORE writing data
 	// -> If metadata write fails (LcsO = 0x07), log warning but continue
 	// -> Hopefully metadata was already set correctly

 	// State update: Writing Trust Anchor
 	trustm_update_state(TRUSTM_STATE_WRITING_TRUST_ANCHOR, NULL, NULL);

 	// STEP 1.1: Write Trust Anchor metadata (type 0x11)
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s [1.1] Writing Trust Anchor metadata to OID 0xE0E8...\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	uint8_t trust_anchor_metadata[] = {
 	 0x20, 0x06, // TLV: Length = 6 bytes
 	 0xD3, 0x01, 0x00, // Tag 0xD3 (Execute Access Condition), Length 1, Value 0x00 (Always)
 	 0xE8, 0x01, 0x11  // Tag 0xE8 (Data Object Type), Length 1, Value 0x11 (Trust Anchor)
 	};

 	optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
 	optiga_lib_status_t metadata_status = optiga_util_write_metadata(
 	 me_util, 0xE0E8, trust_anchor_metadata, sizeof(trust_anchor_metadata)
 	);

 	if (OPTIGA_LIB_SUCCESS == metadata_status) {
 	 // Wait for metadata write completion by polling optiga_lib_status
 	 TickType_t start_ticks = xTaskGetTickCount();
 	 TickType_t timeout_ticks = pdMS_TO_TICKS(1000);

 	 while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
 	 vTaskDelay(pdMS_TO_TICKS(10));
 	 if (optiga_lib_status != OPTIGA_LIB_BUSY) {
 	 break;
 	 }
 	 }
 	 trust_m_async_status = optiga_lib_status;  // Copy result for checking

 	 if (OPTIGA_LIB_SUCCESS == trust_m_async_status) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 	 printf("%s [1.1] Trust Anchor metadata write OK\n", LABEL_SUBSCRIBER);
 	 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

 	 /* Wait 500ms for Trust M to commit metadata to NVM */
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 	 printf("%s [1.1] Waiting 500ms for metadata NVM commit...\n", LABEL_SUBSCRIBER);
 	 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	 vTaskDelay(pdMS_TO_TICKS(500));
 	 } else {
 	 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 	 printf("%s [1.1] WARNING: Metadata write failed (0x%04X) - continuing with data write\n", LABEL_SUBSCRIBER,
 	 trust_m_async_status);
 	 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	 }
 	} else {
 	 #if TESAIOT_DEBUG_VERBOSE_ENABLED
 	 printf("%s [1.1] WARNING: Metadata write request failed (0x%04X) - continuing with data write\n", LABEL_SUBSCRIBER,
 	 metadata_status);
 	 #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	}

 	// STEP 1.2: Write Trust Anchor data (public key DER)
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s [1.2] Writing Trust Anchor data (%u bytes) to OID 0xE0E8...\n", LABEL_SUBSCRIBER, (unsigned int)pubkey_decoded_len);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif
 	optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
 	optiga_lib_status_t write_status = optiga_util_write_data(
 		me_util, 0xE0E8, OPTIGA_UTIL_ERASE_AND_WRITE, 0, pubkey_der, pubkey_decoded_len
 	);

 	if (OPTIGA_LIB_SUCCESS == write_status) {
 		// Wait for Trust M operation by polling optiga_lib_status
 		TickType_t start_ticks = xTaskGetTickCount();
 		TickType_t timeout_ticks = pdMS_TO_TICKS(1000);
 		bool operation_complete = false;

 		while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
 			vTaskDelay(pdMS_TO_TICKS(10));
 			if (optiga_lib_status != OPTIGA_LIB_BUSY) {
 				operation_complete = true;
 				break;
 			}
 		}
 		trust_m_async_status = optiga_lib_status;  // Copy result for checking

 		if (operation_complete && (OPTIGA_LIB_SUCCESS == trust_m_async_status)) {
#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s [1.2] TrustAnchor data write OK\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
#endif

				// Store Platform certificate for Protected Update
				// When provision() later calls write_trust_anchor(), it will use this
				// certificate instead of the hardcoded Infineon certificate
				extern uint8_t *external_trust_anchor;
				extern size_t external_trust_anchor_len;
				external_trust_anchor = pubkey_der;
				external_trust_anchor_len = pubkey_decoded_len;
				#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
				#if TESAIOT_DEBUG_VERBOSE_ENABLED
				printf("%s Stored Platform certificate for Protected Update (%u bytes)\n", LABEL_SUBSCRIBER, (unsigned int)pubkey_decoded_len);
				#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
				#endif

			// DEBUG: Public key hex dump (disabled to reduce serial load)
			#if 0  // DISABLED: HEX dump causes serial buffer overflow and task blocking
			printf("%s DEBUG: Public key hex (%u bytes)\n", LABEL_SUBSCRIBER, (unsigned int)pubkey_decoded_len);
			for (size_t i = 0; i < pubkey_decoded_len; i++) {
				printf("%02x", pubkey_der[i]);
			}
			printf("\n");
			#endif

			// ========================================
			// READBACK VERIFICATION: Verify OID 0xE0E8 data write was successful
			// ========================================
			/* Wait 500ms for Trust M to commit data to NVM before readback */
			#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
			#if TESAIOT_DEBUG_VERBOSE_ENABLED
			printf("%s READBACK: Waiting 500ms for data NVM commit...\n", LABEL_SUBSCRIBER);
			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			#endif
			vTaskDelay(pdMS_TO_TICKS(500));

			#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
			#if TESAIOT_DEBUG_VERBOSE_ENABLED
			printf("%s READBACK: Reading back OID 0xE0E8 to verify...\n", LABEL_SUBSCRIBER);
			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			#endif

			/* STEP 1: Read metadata to confirm type 0x11 (Trust Anchor) */
			uint8_t readback_metadata[64] = {0};
			uint16_t readback_metadata_len = sizeof(readback_metadata);

			optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
			optiga_lib_status_t read_metadata_status = optiga_util_read_metadata(
				me_util, 0xE0E8, readback_metadata, &readback_metadata_len
			);

			if (OPTIGA_LIB_SUCCESS == read_metadata_status) {
				// Wait for metadata read completion by polling
				TickType_t start_ticks = xTaskGetTickCount();
				TickType_t timeout_ticks = pdMS_TO_TICKS(1000);
				bool operation_complete = false;

				while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
					vTaskDelay(pdMS_TO_TICKS(10));
					if (optiga_lib_status != OPTIGA_LIB_BUSY) {
						operation_complete = true;
						break;
					}
				}
				trust_m_async_status = optiga_lib_status;  // Copy result

				if (operation_complete && (OPTIGA_LIB_SUCCESS == trust_m_async_status) && readback_metadata_len >= 2) {
					/* FULL Metadata TLV Parser for OID 0xE0E8 */
					printf("%s READBACK: OID 0xE0E8 Metadata (%u bytes):\n", LABEL_SUBSCRIBER, readback_metadata_len);

					/* Print full hex dump */
					printf("%s   HEX: ", LABEL_SUBSCRIBER);
					for (uint8_t i = 0; i < readback_metadata_len; i++) {
						printf("%02X ", readback_metadata[i]);
					}
					printf("\n");

					/* Parse TLV structure */
					if (readback_metadata[0] == 0x20) {
						uint8_t total_len = readback_metadata[1];
						printf("%s   Total TLV Length: %u bytes\n", LABEL_SUBSCRIBER, total_len);

						/* Parse individual tags */
						uint8_t idx = 2;
						bool found_type_tag = false;
						while (idx < readback_metadata_len && idx < (2 + total_len)) {
							uint8_t tag = readback_metadata[idx];
							uint8_t tag_len = (idx + 1 < readback_metadata_len) ? readback_metadata[idx + 1] : 0;

							printf("%s   Tag 0x%02X (len=%u): ", LABEL_SUBSCRIBER, tag, tag_len);

							switch (tag) {
								case 0xC0: /* LcsO - Lifecycle State */
									if (tag_len >= 1 && idx + 2 < readback_metadata_len) {
										uint8_t lcso = readback_metadata[idx + 2];
										printf("LcsO = 0x%02X ", lcso);
										if (lcso == 0x01) printf("(Creation)");
										else if (lcso == 0x03) printf("(Initialization)");
										else if (lcso == 0x07) printf("(Operational)");
										else if (lcso == 0x0F) printf("(Terminated)");
									}
									break;
								case 0xC4: /* Max Size */
									if (tag_len >= 2 && idx + 3 < readback_metadata_len) {
										uint16_t max_size = (readback_metadata[idx + 2] << 8) | readback_metadata[idx + 3];
										printf("MaxSize = %u bytes", max_size);
									}
									break;
								case 0xC5: /* Used Size */
									if (tag_len >= 2 && idx + 3 < readback_metadata_len) {
										uint16_t used_size = (readback_metadata[idx + 2] << 8) | readback_metadata[idx + 3];
										printf("UsedSize = %u bytes", used_size);
									}
									break;
								case 0xD0: /* Change Access Condition */
									printf("ChangeAccess = ");
									for (uint8_t j = 0; j < tag_len && idx + 2 + j < readback_metadata_len; j++) {
										printf("%02X ", readback_metadata[idx + 2 + j]);
									}
									break;
								case 0xD1: /* Read Access Condition */
									printf("ReadAccess = ");
									for (uint8_t j = 0; j < tag_len && idx + 2 + j < readback_metadata_len; j++) {
										printf("%02X ", readback_metadata[idx + 2 + j]);
									}
									break;
								case 0xD3: /* Execute Access Condition */
									printf("ExecuteAccess = ");
									for (uint8_t j = 0; j < tag_len && idx + 2 + j < readback_metadata_len; j++) {
										printf("%02X ", readback_metadata[idx + 2 + j]);
									}
									break;
								case 0xE8: /* Data Object Type */
									found_type_tag = true;
									if (tag_len >= 1 && idx + 2 < readback_metadata_len) {
										uint8_t obj_type = readback_metadata[idx + 2];
										printf("Type = 0x%02X ", obj_type);
										if (obj_type == 0x00) printf("(BSTR)");
										else if (obj_type == 0x01) printf("(UPCTR)");
										else if (obj_type == 0x11) printf("(TRUST_ANCHOR) *** REQUIRED ***");
										else if (obj_type == 0x12) printf("(DEVCERT)");
										else if (obj_type == 0x21) printf("(PTFBIND)");
										else if (obj_type == 0x22) printf("(UPDATSEC)");
										else if (obj_type == 0x23) printf("(AUTHREF)");
									}
									break;
								default:
									printf("Unknown tag, data: ");
									for (uint8_t j = 0; j < tag_len && idx + 2 + j < readback_metadata_len; j++) {
										printf("%02X ", readback_metadata[idx + 2 + j]);
									}
									break;
							}
							printf("\n");
							idx += 2 + tag_len;
						}

						/* Summary */
						if (!found_type_tag) {
							printf("%s   *** CRITICAL: No Type tag (0xE8) found! ***\n", LABEL_SUBSCRIBER);
							printf("%s   *** OID 0xE0E8 is NOT configured as Trust Anchor (type 0x11) ***\n", LABEL_SUBSCRIBER);
							printf("%s   *** Protected Update WILL FAIL with Error 0x8029 ***\n", LABEL_SUBSCRIBER);
						}
					}
				} else {
					#if TESAIOT_DEBUG_VERBOSE_ENABLED
					printf("%s WARNING: Metadata read failed (status=0x%04X, len=%u)\n", LABEL_SUBSCRIBER, trust_m_async_status, readback_metadata_len);
					#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
				}
			} else {
				#if TESAIOT_DEBUG_VERBOSE_ENABLED
				printf("%s ERROR: Metadata read request failed (0x%04X)\n", LABEL_SUBSCRIBER, read_metadata_status);
				#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			}

			// STEP 2: Read data back from OID 0xE0E8
			uint8_t readback_cert[800] = {0}; // X.509 Certificate DER = ~580 bytes
			uint16_t readback_cert_len = sizeof(readback_cert);

			optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
			optiga_lib_status_t read_data_status = optiga_util_read_data(
				me_util, 0xE0E8, 0, readback_cert, &readback_cert_len
			);

			if (OPTIGA_LIB_SUCCESS == read_data_status) {
				// Wait for data read completion by polling
				TickType_t start_ticks = xTaskGetTickCount();
				TickType_t timeout_ticks = pdMS_TO_TICKS(1000);
				bool operation_complete = false;

				while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
					vTaskDelay(pdMS_TO_TICKS(10));
					if (optiga_lib_status != OPTIGA_LIB_BUSY) {
						operation_complete = true;
						break;
					}
				}
				trust_m_async_status = optiga_lib_status;  // Copy result

				if (operation_complete && (OPTIGA_LIB_SUCCESS == trust_m_async_status)) {
					#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
					#if TESAIOT_DEBUG_VERBOSE_ENABLED
					printf("%s READBACK: Successfully read %u bytes from OID 0xE0E8\n", LABEL_SUBSCRIBER, readback_cert_len);
					#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
					#endif

					// STEP 3: Compare with original public key
					if (readback_cert_len == pubkey_decoded_len) {
						bool match = true;
						for (size_t i = 0; i < pubkey_decoded_len; i++) {
							if (readback_cert[i] != pubkey_der[i]) {
								match = false;
								#if TESAIOT_DEBUG_VERBOSE_ENABLED
								printf("%s MISMATCH at byte %zu: wrote 0x%02X, read 0x%02X\n", LABEL_SUBSCRIBER,
									i, pubkey_der[i], readback_cert[i]);
								#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
								break;
							}
						}

						if (match) {
							#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
							#if TESAIOT_DEBUG_VERBOSE_ENABLED
							printf("%s READBACK VERIFIED: Public key matches 100%%!\n", LABEL_SUBSCRIBER);
							#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
							#endif
						} else {
							#if TESAIOT_DEBUG_VERBOSE_ENABLED
							printf("%s READBACK FAILED: Public key corrupted in Trust M!\n", LABEL_SUBSCRIBER);
							#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
						}
					} else {
						#if TESAIOT_DEBUG_VERBOSE_ENABLED
						printf("%s LENGTH MISMATCH: Wrote %zu bytes, read %u bytes\n", LABEL_SUBSCRIBER, pubkey_decoded_len, readback_cert_len);
						#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
					}

					// READBACK hex dump (disabled to reduce serial load)
					#if 0  // DISABLED: HEX dump causes serial buffer overflow
					printf("%s READBACK: Public key hex (%u bytes)\n", LABEL_SUBSCRIBER, readback_cert_len);
					for (size_t i = 0; i < readback_cert_len; i++) {
						printf("%02x", readback_cert[i]);
					}
					printf("\n");
					#endif
				} else {
					#if TESAIOT_DEBUG_VERBOSE_ENABLED
					printf("%s ERROR: Data read failed (0x%04X)\n", LABEL_SUBSCRIBER, trust_m_async_status);
					#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
				}
			} else {
				#if TESAIOT_DEBUG_VERBOSE_ENABLED
				printf("%s ERROR: Data read request failed (0x%04X)\n", LABEL_SUBSCRIBER, read_data_status);
				#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			}

			// ========================================
			// NOTE: Force Trust M NVM commit before manifest verification
			// ========================================
			// PROBLEM: Error 0x8029 occurs because Trust M hasn't committed Trust Anchor
			// to NVM before manifest verification reads it.
			//
			// Timeline:
			// 1. optiga_util_write_data(0xE0E8) -> writes to cache -> returns success
			// 2. Immediate readback -> reads from cache -> matches 100%
			// 3. optiga_util_protected_update_start() -> reads OID 0xE0E8 from NVM
			// -> But NVM not updated yet -> sees old data (zeros)
			// -> Signature verification fails -> Error 0x8029!
			//
			// SOLUTION:
			// 1. Add 500ms delay to allow background NVM commit
			// 2. Re-read OID 0xE0E8 to force synchronization point
			// 3. This ensures Trust M has committed before manifest verification
			//
			// NVM COMMIT WAIT - Release mutex first to avoid deadlock
			optiga_manager_release();
			vTaskDelay(pdMS_TO_TICKS(500));  // 500ms is enough for NVM commit

 		} else {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s TrustAnchor-FAIL:0x%04X\n", LABEL_SUBSCRIBER, trust_m_async_status);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag on early exit
 			optiga_manager_release(); // Release (not destroy) persistent instance
 			vPortFree(pubkey_der);
 			vPortFree(json_copy);
 			break;
 		}
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s TrustAnchor-write-fail:0x%04X\n", LABEL_SUBSCRIBER, write_status);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		optiga_manager_release(); // Release (not destroy) persistent instance
 		vPortFree(pubkey_der);
 		vPortFree(json_copy);
 		break;
 	}

 	// NOTE: optiga_manager_release() already called before NVM wait
 	pubkey = pubkey_der;
 	pubkey_length = pubkey_decoded_len;

 	// ========================================
 	// STEP 2: Process manifest (decode and store)
 	// ========================================
 	printf("%s STEP 2: Processing manifest...\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
 	size_t manifest_decoded_len = 0;
 	uint8_t *manifest_buf = NULL;
 	decode_ret = mbedtls_base64_decode(NULL, 0, &manifest_decoded_len,
 	 (uint8_t*)parse_ctx.manifest_b64,
 	 parse_ctx.manifest_len);

 	if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
 		manifest_buf = (uint8_t*) pvPortMalloc(manifest_decoded_len);
 		if (manifest_buf) {
 			decode_ret = mbedtls_base64_decode(manifest_buf, manifest_decoded_len,
 			 &manifest_decoded_len,
 			 (uint8_t*)parse_ctx.manifest_b64,
 			 parse_ctx.manifest_len);
 		}
 	}

 	if (decode_ret != 0 || !manifest_buf) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: manifest B64 decode fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		if (manifest_buf) vPortFree(manifest_buf);
 		vPortFree(json_copy);
 		break;
 	}

 	__DSB(); __DMB(); __ISB();
 	manifest_ecc_key = manifest_buf;
 	manifest_ecc_key_length = manifest_decoded_len;
 	printf("%s Manifest OK (%lu bytes)\n", LABEL_SUBSCRIBER, (unsigned long)manifest_decoded_len);

 	/* DEBUG: Parse manifest CBOR structure to verify Trust Anchor OID */
 	printf("%s Manifest Analysis:\n", LABEL_SUBSCRIBER);
 	printf("%s   First 16 bytes: ", LABEL_SUBSCRIBER);
 	for (size_t i = 0; i < 16 && i < manifest_decoded_len; i++) {
 		printf("%02X ", manifest_buf[i]);
 	}
 	printf("\n");

 	/* CBOR COSE_Sign1 structure: bytes[8:9] contain Trust Anchor OID */
 	if (manifest_decoded_len >= 10) {
 		uint16_t manifest_ta_oid = (manifest_buf[8] << 8) | manifest_buf[9];
 		printf("%s   Trust Anchor OID in manifest: 0x%04X ", LABEL_SUBSCRIBER, manifest_ta_oid);
 		if (manifest_ta_oid == 0xE0E8) {
 			printf("(MATCHES device config!)\n");
 		} else {
 			printf("*** MISMATCH! Device uses 0xE0E8 ***\n");
 		}
 	}

 	/* Show signature algorithm (byte 4) */
 	if (manifest_decoded_len >= 5) {
 		uint8_t sign_algo = manifest_buf[4];
 		printf("%s   Signature Algorithm: 0x%02X ", LABEL_SUBSCRIBER, sign_algo);
 		if (sign_algo == 0x26) printf("(ES256 - ECDSA P-256)\n");
 		else if (sign_algo == 0x27) printf("(ES384)\n");
 		else printf("(Unknown)\n");
 	}

#if 0  /* Manifest hex dump disabled to reduce debug output - causes UART buffer issues */
 	/* DEBUG: Full manifest hex dump for Server comparison */
 	printf("%s ========== MANIFEST HEX DUMP (%lu bytes) ==========\n", LABEL_SUBSCRIBER, (unsigned long)manifest_decoded_len);
 	for (size_t i = 0; i < manifest_decoded_len; i++) {
 		printf("%02X", manifest_buf[i]);
 		if ((i + 1) % 32 == 0) printf("\n");
 		else if ((i + 1) % 8 == 0) printf(" ");
 	}
 	if (manifest_decoded_len % 32 != 0) printf("\n");
 	printf("%s ========== END MANIFEST HEX DUMP ==========\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
#endif

 	xEventGroupSetBits(data_received_event_group, MANIFEST_RECEIVED_BIT);

 	// ========================================
 	// STEP 3: Process fragments (decode and concatenate)
 	// ========================================
 	printf("%s STEP 3: Processing fragments...\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
 	size_t fragment_0_decoded_len = 0;
 	size_t fragment_1_decoded_len = 0;
 	size_t fragment_2_decoded_len = 0;
 	uint8_t *fragment_0_buf = NULL;
 	uint8_t *fragment_1_buf = NULL;
 	uint8_t *fragment_2_buf = NULL;

 	// Decode fragment_0 (always present)
 	decode_ret = mbedtls_base64_decode(NULL, 0, &fragment_0_decoded_len,
 	 (uint8_t*)parse_ctx.fragment_0_b64,
 	 parse_ctx.fragment_0_len);
 	if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
 		fragment_0_buf = (uint8_t*) pvPortMalloc(fragment_0_decoded_len);
 		if (fragment_0_buf) {
 			decode_ret = mbedtls_base64_decode(fragment_0_buf, fragment_0_decoded_len,
 			 &fragment_0_decoded_len,
 			 (uint8_t*)parse_ctx.fragment_0_b64,
 			 parse_ctx.fragment_0_len);
 		}
 	}

 	if (decode_ret != 0 || !fragment_0_buf) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: fragment_0 B64 decode fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		if (fragment_0_buf) vPortFree(fragment_0_buf);
 		vPortFree(json_copy);
 		break;
 	}

 	// Decode fragment_1 if exists
 	if (parse_ctx.fragment_1_b64 && parse_ctx.fragment_1_len > 0) {
 		decode_ret = mbedtls_base64_decode(NULL, 0, &fragment_1_decoded_len,
 		 (uint8_t*)parse_ctx.fragment_1_b64,
 		 parse_ctx.fragment_1_len);
 		if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
 			fragment_1_buf = (uint8_t*) pvPortMalloc(fragment_1_decoded_len);
 			if (fragment_1_buf) {
 				decode_ret = mbedtls_base64_decode(fragment_1_buf, fragment_1_decoded_len,
 				 &fragment_1_decoded_len,
 				 (uint8_t*)parse_ctx.fragment_1_b64,
 				 parse_ctx.fragment_1_len);
 			}
 		}

 		if (decode_ret != 0 || !fragment_1_buf) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: fragment_1 B64 decode fail\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag on early exit
 			vPortFree(fragment_0_buf);
 			if (fragment_1_buf) vPortFree(fragment_1_buf);
 			vPortFree(json_copy);
 			break;
 		}
 	}

 	// Decode fragment_2 if exists (rare)
 	if (parse_ctx.fragment_2_b64 && parse_ctx.fragment_2_len > 0) {
 		decode_ret = mbedtls_base64_decode(NULL, 0, &fragment_2_decoded_len,
 		 (uint8_t*)parse_ctx.fragment_2_b64,
 		 parse_ctx.fragment_2_len);
 		if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
 			fragment_2_buf = (uint8_t*) pvPortMalloc(fragment_2_decoded_len);
 			if (fragment_2_buf) {
 				decode_ret = mbedtls_base64_decode(fragment_2_buf, fragment_2_decoded_len,
 				 &fragment_2_decoded_len,
 				 (uint8_t*)parse_ctx.fragment_2_b64,
 				 parse_ctx.fragment_2_len);
 			}
 		}

 		if (decode_ret != 0 || !fragment_2_buf) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: fragment_2 B64 decode fail\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag on early exit
 			vPortFree(fragment_0_buf);
 			if (fragment_1_buf) vPortFree(fragment_1_buf);
 			if (fragment_2_buf) vPortFree(fragment_2_buf);
 			vPortFree(json_copy);
 			break;
 		}
 	}

 	// Concatenate fragments into single buffer
 	size_t total_fragment_len = fragment_0_decoded_len + fragment_1_decoded_len + fragment_2_decoded_len;
 	uint8_t *fragments_combined = (uint8_t*) pvPortMalloc(total_fragment_len);
 	if (!fragments_combined) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc combined fragments fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag on early exit
 		vPortFree(fragment_0_buf);
 		if (fragment_1_buf) vPortFree(fragment_1_buf);
 		if (fragment_2_buf) vPortFree(fragment_2_buf);
 		vPortFree(json_copy);
 		break;
 	}

 	// Copy fragment_0 (always present)
 	memcpy(fragments_combined, fragment_0_buf, fragment_0_decoded_len);

 	// Copy fragment_1 if exists
 	if (fragment_1_buf) {
 		memcpy(fragments_combined + fragment_0_decoded_len, fragment_1_buf, fragment_1_decoded_len);
 	}

 	// Copy fragment_2 if exists
 	if (fragment_2_buf) {
 		memcpy(fragments_combined + fragment_0_decoded_len + fragment_1_decoded_len,
 		 fragment_2_buf, fragment_2_decoded_len);
 	}

 	__DSB(); __DMB(); __ISB();
 	ecc_key_final_fragment_array = fragments_combined;
 	ecc_key_final_fragment_array_length = total_fragment_len;

 	vPortFree(fragment_0_buf);
 	if (fragment_1_buf) vPortFree(fragment_1_buf);
 	if (fragment_2_buf) vPortFree(fragment_2_buf);

 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Fragments OK (%lu bytes total)\n", LABEL_SUBSCRIBER, (unsigned long)total_fragment_len);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif

		// DEBUG: Fragment hex dump (disabled to reduce serial load)
		#if 0  // DISABLED: HEX dump causes serial buffer overflow
		printf("%s DEBUG: Fragment hex (%lu bytes)\n", LABEL_SUBSCRIBER, (unsigned long)total_fragment_len);
		for (size_t i = 0; i < total_fragment_len; i++) {
			printf("%02x", fragments_combined[i]);
		}
		printf("\n");
		#endif

 	xEventGroupSetBits(data_received_event_group, FRAGMENT_RECEIVED_BIT);

 	// NOTE: PROTECTED_UPDATE_COMPLETE_BIT is set AFTER STEP 4.2 success (see below)
 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Protected Update bundle complete\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif

 	// ========================================
 	// STEP 3.5: Configure Target OID metadata with MUD field
 	// ========================================
 	// CRITICAL: Target OID 0xE0E1 MUST have MUD field pointing to Trust Anchor 0xE0E8
 	// Without MUD, OPTIGA Trust M cannot verify manifest signature → Error 0x8007
 	// Reference: tesaiot_protected_update_isolated.c Step 3
 	{
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s [3.5] Configuring Target OID 0xE0E1 metadata with MUD → 0xE0E8...\n", LABEL_SUBSCRIBER);
 		fflush(stdout);
 		#endif

 		// Acquire OPTIGA instance for metadata write
 		optiga_util_t *me_mud = optiga_manager_acquire();
 		if (!me_mud) {
 			printf("%s ERROR: Failed to acquire OPTIGA for MUD config\n", LABEL_SUBSCRIBER);
 			g_protected_update_active = false;
 			vPortFree(json_copy);
 			if (subscriber_q_data.need_free && subscriber_q_data.data) {
 				vPortFree(subscriber_q_data.data);
 			}
 			break;
 		}

 		// MUD metadata: Change access = Integrity protected using OID 0xE0E8
 		// Format: 0xD0 (Change tag), 0x03 (length), 0x21 (MUD subfield), 0xE0, 0xE8 (Trust Anchor OID)
 		// Also include version counter reset to 0x0000 for manifest version 1
 		uint8_t target_mud_metadata[] = {
 			0x20, 0x09,                     // Container + length 9
 			0xC1, 0x02, 0x00, 0x00,         // Version counter = 0x0000 (required for manifest version 1)
 			0xD0, 0x03, 0x21, 0xE0, 0xE8    // Change access = Integrity protected using 0xE0E8
 		};

 		optiga_lib_status = OPTIGA_LIB_BUSY;
 		optiga_lib_status_t mud_status = optiga_util_write_metadata(
 			me_mud,
 			0xE0E1,  // Target OID (Device Certificate slot)
 			target_mud_metadata,
 			sizeof(target_mud_metadata)
 		);

 		if (mud_status != OPTIGA_LIB_SUCCESS) {
 			printf("%s ERROR: Target OID MUD write failed: 0x%04X\n", LABEL_SUBSCRIBER, mud_status);
 			optiga_manager_release();
 			g_protected_update_active = false;
 			vPortFree(json_copy);
 			if (subscriber_q_data.need_free && subscriber_q_data.data) {
 				vPortFree(subscriber_q_data.data);
 			}
 			break;
 		}

 		// Wait for async operation
 		uint32_t mud_timeout_ms = 5000, mud_elapsed_ms = 0;
 		while (optiga_lib_status == OPTIGA_LIB_BUSY && mud_elapsed_ms < mud_timeout_ms) {
 			vTaskDelay(pdMS_TO_TICKS(10));
 			mud_elapsed_ms += 10;
 		}

 		if (optiga_lib_status != OPTIGA_LIB_SUCCESS) {
 			printf("%s ERROR: Target OID MUD write callback failed: 0x%04X\n", LABEL_SUBSCRIBER, optiga_lib_status);
 			optiga_manager_release();
 			g_protected_update_active = false;
 			vPortFree(json_copy);
 			if (subscriber_q_data.need_free && subscriber_q_data.data) {
 				vPortFree(subscriber_q_data.data);
 			}
 			break;
 		}

 		optiga_manager_release();
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s [3.5] Target OID 0xE0E1 MUD configured (→ 0xE0E8)\n", LABEL_SUBSCRIBER);
 		fflush(stdout);
 		#endif

 		// Wait for NVM commit
 		vTaskDelay(pdMS_TO_TICKS(200));
 	}

 	// ========================================
 	// STEP 4: Execute OPTIGA Protected Update
 	// ========================================
 	// NOTE: g_protected_update_active was set to true at the START of this case
 	// to prevent Publisher task creation during Trust Anchor write and READBACK
 	printf("%s ====================================================================\n", LABEL_SUBSCRIBER);
 	printf("%s STEP 4: Executing OPTIGA Trust M Protected Update\n", LABEL_SUBSCRIBER);
 	printf("%s ====================================================================\n", LABEL_SUBSCRIBER);
 	fflush(stdout);

 	// Acquire OPTIGA instance
 	optiga_util_t *me_protupd = optiga_manager_acquire();
 	if (!me_protupd) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Failed to acquire OPTIGA instance\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag before exit
 		vPortFree(json_copy);
 		if (subscriber_q_data.need_free && subscriber_q_data.data) {
 			vPortFree(subscriber_q_data.data);
 		}
 		break;
 	}

 	// Step 4.1: Start Protected Update with manifest
	// CRITICAL: Use single printf with fflush to avoid interleaved output
	printf("%s [4.1] Manifest(%u) -> calling protected_update_start...\n", LABEL_SUBSCRIBER, (unsigned int)manifest_ecc_key_length);
	fflush(stdout);

	// State update: Verifying manifest signature
	trustm_update_state(TRUSTM_STATE_VERIFYING_MANIFEST, NULL, NULL);

 	optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling

 	optiga_lib_status_t protupd_status = optiga_util_protected_update_start(
 		me_protupd,
 		1, // manifest_version = 1 (Infineon SDK standard - all v5.3.0/v5.4.0 examples use version 1)
 		manifest_ecc_key,
 		manifest_ecc_key_length
 	);

	// DEBUG: If we reach here, function returned
	printf("%s [4.1] optiga_util_protected_update_start() returned: 0x%04X\n", LABEL_SUBSCRIBER, protupd_status);
	fflush(stdout);

 	if (OPTIGA_LIB_SUCCESS != protupd_status) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: optiga_util_protected_update_start() failed (0x%04X)\n", LABEL_SUBSCRIBER, protupd_status);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag before exit
 		optiga_manager_release();
 		vPortFree(json_copy);
 		if (subscriber_q_data.need_free && subscriber_q_data.data) {
 			vPortFree(subscriber_q_data.data);
 		}
 		break;
 	}

 	// Wait for manifest verification by polling (5 second timeout)
	// NOTE: Using 100ms delay (same as isolated test) to ensure proper async handling
 	TickType_t start_ticks = xTaskGetTickCount();
 	TickType_t timeout_ticks = pdMS_TO_TICKS(5000);
 	bool manifest_verified = false;

	printf("%s [4.1] Waiting for callback (optiga_lib_status=0x%04X)...\n", LABEL_SUBSCRIBER, optiga_lib_status);
	fflush(stdout);

 	while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
 		vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay like isolated test
 		if (optiga_lib_status != OPTIGA_LIB_BUSY) {
 			manifest_verified = true;
 			break;
 		}
 	}
 	trust_m_async_status = optiga_lib_status;  // Copy result

 	if (manifest_verified) {
 		if (OPTIGA_LIB_SUCCESS != trust_m_async_status) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: Manifest verification FAILED (0x%04X)\n", LABEL_SUBSCRIBER, trust_m_async_status);
 			printf("%s This means Trust Anchor signature verification failed.\n", LABEL_SUBSCRIBER);
 			printf("%s Check: 1) Trust Anchor in OID 0xE0E8 matches Platform signing key\n", LABEL_SUBSCRIBER);
 			printf("%s 2) Manifest signature is valid\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag before exit
 			optiga_manager_release();
 			vPortFree(json_copy);
 			if (subscriber_q_data.need_free && subscriber_q_data.data) {
 				vPortFree(subscriber_q_data.data);
 			}
 			break;
 		}
 		#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s [4.1] Manifest verification OK (Trust Anchor signature valid)\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		#endif
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Manifest verification timeout (5s)\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag before exit
 		optiga_manager_release();
 		vPortFree(json_copy);
 		if (subscriber_q_data.need_free && subscriber_q_data.data) {
 			vPortFree(subscriber_q_data.data);
 		}
 		break;
 	}

 	// Step 4.2: Send fragments to write certificate
 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s [4.2] Sending fragments (%u bytes) to Trust M for OID 0xE0E1 write...\n", LABEL_SUBSCRIBER, (unsigned int)ecc_key_final_fragment_array_length);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif
 	optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
 	protupd_status = optiga_util_protected_update_final(
 		me_protupd,
 		ecc_key_final_fragment_array,
 		ecc_key_final_fragment_array_length
 	);

 	if (OPTIGA_LIB_SUCCESS != protupd_status) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: optiga_util_protected_update_final() failed (0x%04X)\n", LABEL_SUBSCRIBER, protupd_status);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag before exit
 		optiga_manager_release();
 		vPortFree(json_copy);
 		if (subscriber_q_data.need_free && subscriber_q_data.data) {
 			vPortFree(subscriber_q_data.data);
 		}
 		break;
 	}

 	// Wait for fragment write by polling (5 second timeout)
	// NOTE: Using 100ms delay (same as isolated test)
 	start_ticks = xTaskGetTickCount();
 	timeout_ticks = pdMS_TO_TICKS(5000);
 	bool fragment_written = false;

 	while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
 		vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay like isolated test
 		if (optiga_lib_status != OPTIGA_LIB_BUSY) {
 			fragment_written = true;
 			break;
 		}
 	}
 	trust_m_async_status = optiga_lib_status;  // Copy result

 	if (fragment_written) {
 		if (OPTIGA_LIB_SUCCESS != trust_m_async_status) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: Fragment write FAILED (0x%04X)\n", LABEL_SUBSCRIBER, trust_m_async_status);
 			printf("%s Certificate payload could not be written to OID 0xE0E1\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			g_protected_update_active = false;  // Reset flag before exit
 			optiga_manager_release();
 			vPortFree(json_copy);
 			if (subscriber_q_data.need_free && subscriber_q_data.data) {
 				vPortFree(subscriber_q_data.data);
 			}
 			break;
 		}
 		#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s [4.2] Fragment write OK\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		#endif
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Fragment write timeout (5s)\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		g_protected_update_active = false;  // Reset flag before exit
 		optiga_manager_release();
 		vPortFree(json_copy);
 		if (subscriber_q_data.need_free && subscriber_q_data.data) {
 			vPortFree(subscriber_q_data.data);
 		}
 		break;
 	}

 	// Protected Update completed successfully
 	// Set "just completed" flag FIRST - this prevents:
 	// 1. MQTT-Task from creating Publisher task
 	// 2. Subscriber from processing stray cmd=2 from Server
 	g_protected_update_just_completed = true;
 	g_protected_update_active = false;  // Reset flag - but Publisher task creation blocked by just_completed

 	// Signal workflow completion via event group - workflow waits on this bit
 	xEventGroupSetBits(data_received_event_group, PROTECTED_UPDATE_COMPLETE_BIT);
 	trustm_update_state(TRUSTM_STATE_PROTECTED_UPDATE_SUCCESS, NULL, NULL);

 	// Always print success message (unconditional)
 	printf("%s ====================================================================\n", LABEL_SUBSCRIBER);
 	printf("%s PROTECTED UPDATE COMPLETED SUCCESSFULLY!\n", LABEL_SUBSCRIBER);
 	printf("%s Certificate written to OID 0xE0E1 via Protected Update mechanism\n", LABEL_SUBSCRIBER);
 	printf("%s ====================================================================\n", LABEL_SUBSCRIBER);
 	fflush(stdout);
 	optiga_manager_release();

 	// Cleanup JSON copy buffer
 	vPortFree(json_copy);

 	// Free original MQTT data buffer if allocated in callback
 	if (subscriber_q_data.need_free && subscriber_q_data.data) {
 		vPortFree(subscriber_q_data.data);
 	}
 	break;
 }

 /******************************************************************************
 * Smart Auto-Fallback Response Handlers
 ******************************************************************************/
 case CHECK_CERTIFICATE_RESPONSE:
 {
 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Received check_certificate_response\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif

 	// Parse JSON response to extract has_certificate field
 	if (!subscriber_q_data.data || subscriber_q_data.data_size == 0) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Empty check_certificate response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		check_certificate_response_received = true;
 		platform_has_certificate = false;
 		xSemaphoreGive(check_certificate_response_semaphore);
 		break;
 	}

 	// Copy JSON data to null-terminated buffer
 	char *json_copy = (char *)pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!json_copy) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc failed for check_certificate JSON\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		check_certificate_response_received = true;
 		platform_has_certificate = false;
 		xSemaphoreGive(check_certificate_response_semaphore);
 		break;
 	}
 	memcpy(json_copy, subscriber_q_data.data, subscriber_q_data.data_size);
 	json_copy[subscriber_q_data.data_size] = '\0';

 	// Simple JSON parsing - look for "has_certificate":true or "has_certificate":false
 	char *has_cert_field = strstr(json_copy, "\"has_certificate\"");
 	if (has_cert_field) {
 		char *true_value = strstr(has_cert_field, "true");
 		char *false_value = strstr(has_cert_field, "false");

 		if (true_value && (!false_value || true_value < false_value)) {
 			platform_has_certificate = true;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Platform HAS certificate\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		} else {
 			platform_has_certificate = false;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Platform does NOT have certificate\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		}
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: has_certificate field not found in response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		platform_has_certificate = false;
 	}

 	vPortFree(json_copy);
 	check_certificate_response_received = true;
 	xSemaphoreGive(check_certificate_response_semaphore);

 	// Free original MQTT data buffer if allocated in callback
 	if (subscriber_q_data.need_free && subscriber_q_data.data) {
 		vPortFree(subscriber_q_data.data);
 	}
 	break;
 }

 case UPLOAD_CERTIFICATE_RESPONSE:
 {
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Received upload_certificate_response\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 	// Parse JSON response to extract status field
 	if (!subscriber_q_data.data || subscriber_q_data.data_size == 0) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Empty upload_certificate response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		upload_certificate_response_received = true;
 		certificate_upload_success = false;
 		xSemaphoreGive(upload_certificate_response_semaphore);
 		break;
 	}

 	// Copy JSON data to null-terminated buffer
 	char *json_copy = (char *)pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!json_copy) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc failed for upload_certificate JSON\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		upload_certificate_response_received = true;
 		certificate_upload_success = false;
 		xSemaphoreGive(upload_certificate_response_semaphore);
 		break;
 	}
 	memcpy(json_copy, subscriber_q_data.data, subscriber_q_data.data_size);
 	json_copy[subscriber_q_data.data_size] = '\0';

 	// Simple JSON parsing - look for "status":"success"
 	char *status_field = strstr(json_copy, "\"status\"");
 	if (status_field) {
 		char *success_value = strstr(status_field, "\"success\"");

 		if (success_value) {
 			certificate_upload_success = true;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Certificate upload SUCCESS\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		} else {
 			certificate_upload_success = false;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Certificate upload FAILED\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		}
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: status field not found in response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		certificate_upload_success = false;
 	}

 	vPortFree(json_copy);
 	upload_certificate_response_received = true;
 	xSemaphoreGive(upload_certificate_response_semaphore);
 	break;
 }

 case SYNC_CERTIFICATE_RESPONSE: // Unified sync_certificate response
 {
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Received sync_certificate_response\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

 	// Parse JSON response to extract status field
 	if (!subscriber_q_data.data || subscriber_q_data.data_size == 0) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Empty sync_certificate response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		sync_certificate_response_received = true;
 		certificate_sync_success = false;
 		xSemaphoreGive(sync_certificate_response_semaphore);
 		break;
 	}

 	// Copy JSON data to null-terminated buffer
 	char *json_copy = (char *)pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!json_copy) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc failed for sync_certificate JSON\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		sync_certificate_response_received = true;
 		certificate_sync_success = false;
 		xSemaphoreGive(sync_certificate_response_semaphore);
 		break;
 	}
 	memcpy(json_copy, subscriber_q_data.data, subscriber_q_data.data_size);
 	json_copy[subscriber_q_data.data_size] = '\0';

 	// Simple JSON parsing - look for "status":"success"
 	char *status_field = strstr(json_copy, "\"status\"");
 	if (status_field) {
 		char *success_value = strstr(status_field, "\"success\"");

 		if (success_value) {
 			certificate_sync_success = true;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Certificate sync SUCCESS\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		} else {
 			certificate_sync_success = false;
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s Certificate sync FAILED\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		}
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: status field not found in sync response\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		certificate_sync_success = false;
 	}

 	vPortFree(json_copy);
 	sync_certificate_response_received = true;
 	xSemaphoreGive(sync_certificate_response_semaphore);
 	break;
 }

 /******************************************************************************
 * DEPRECATED HANDLERS - Replaced by UPDATE_PROTECTED_UPDATE_BUNDLE (2025-10-22)
 *
 * These handlers are no longer used. The Platform now sends all Protected Update
 * components (pubkey, manifest, fragments) in a single JSON message via the
 * UPDATE_PROTECTED_UPDATE_BUNDLE handler.
 *
 * Kept here for reference only. Will be removed in future version.
 ******************************************************************************/
 #if 0
 case UPDATE_DEVICE_MANIFEST:
 {
 	uint8_t *manifest_buf = (uint8_t*) pvPortMalloc(subscriber_q_data.data_size);
 	if (!manifest_buf) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc manifest fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}
 	memcpy(manifest_buf, subscriber_q_data.data, subscriber_q_data.data_size);
 	__DSB(); __DMB(); __ISB();

 	manifest_ecc_key = manifest_buf;
 	manifest_ecc_key_length = subscriber_q_data.data_size;
 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Manifest OK\n", LABEL_SUBSCRIBER);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif

 	xEventGroupSetBits(data_received_event_group, MANIFEST_RECEIVED_BIT);
 	break;
 }
 case UPDATE_DEVICE_FRAGMENT:
 {
 	uint8_t *fragment_buf = (uint8_t*) pvPortMalloc(subscriber_q_data.data_size);
 	if (!fragment_buf) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc fragment fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}
 	memcpy(fragment_buf, subscriber_q_data.data, subscriber_q_data.data_size);
 	__DSB(); __DMB(); __ISB();

 	ecc_key_final_fragment_array = fragment_buf;
 	ecc_key_final_fragment_array_length = subscriber_q_data.data_size;
 	#if TESAIOT_DEBUG_SUBSCRIBER_ENABLED
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s Fragment OK (%lu bytes)\n", LABEL_SUBSCRIBER, (unsigned long)subscriber_q_data.data_size);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	#endif

		// DEBUG: Fragment hex dump (disabled to reduce serial load)
		#if 0  // DISABLED: HEX dump causes serial buffer overflow
		printf("%s DEBUG: Fragment hex (%lu bytes)\n", LABEL_SUBSCRIBER, (unsigned long)subscriber_q_data.data_size);
		for (size_t i = 0; i < subscriber_q_data.data_size; i++) {
			printf("%02x", fragment_buf[i]);
		}
		printf("\n");
		#endif

 	xEventGroupSetBits(data_received_event_group, FRAGMENT_RECEIVED_BIT);
 	break;
 }
 case UPDATE_DEVICE_PUBLIC_KEY:
 {
 	// MINIMAL printf to avoid UART overflow
 	if (!subscriber_q_data.data) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: NULL pubkey\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}

 	uint8_t *pubkey_buf = (uint8_t*) pvPortMalloc(subscriber_q_data.data_size + 1);
 	if (!pubkey_buf) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: malloc pubkey fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		break;
 	}
 	memcpy(pubkey_buf, subscriber_q_data.data, subscriber_q_data.data_size);
 	pubkey_buf[subscriber_q_data.data_size] = '\0';
 	__DSB(); __DMB(); __ISB();

 	// Decode Base64 to DER
 	size_t decoded_len = 0;
 	uint8_t *pubkey_der = NULL;
 	int decode_ret = mbedtls_base64_decode(NULL, 0, &decoded_len, pubkey_buf, subscriber_q_data.data_size);
 	if (decode_ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
 		pubkey_der = (uint8_t*) pvPortMalloc(decoded_len);
 		if (pubkey_der) {
 			decode_ret = mbedtls_base64_decode(pubkey_der, decoded_len, &decoded_len,
 			 pubkey_buf, subscriber_q_data.data_size);
 		}
 	}

 	if (decode_ret != 0 || !pubkey_der) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: B64 decode fail\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		vPortFree(pubkey_buf);
 		if (pubkey_der) vPortFree(pubkey_der);
 		break;
 	}

 	// Create semaphore for Trust M async operations (first time only)
 	if (trust_m_write_semaphore == NULL) {
 		trust_m_write_semaphore = xSemaphoreCreateBinary();
 		if (!trust_m_write_semaphore) {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s ERROR: semaphore fail\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			vPortFree(pubkey_buf);
 			vPortFree(pubkey_der);
 			break;
 		}
 	}

 	// Best Practice: Use persistent OPTIGA instance (no create/destroy)
 	trust_m_async_status = OPTIGA_LIB_BUSY;
 	optiga_util_t *me_util = optiga_manager_acquire();
 	if (!me_util) {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s ERROR: Failed to acquire OPTIGA instance\n", LABEL_SUBSCRIBER);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		vPortFree(pubkey_buf);
 		vPortFree(pubkey_der);
 		break;
 	}

 	// Write Trust Anchor to OID 0xE0E8
 	optiga_lib_status = OPTIGA_LIB_BUSY;  // Set global status for polling
 	optiga_lib_status_t write_status = optiga_util_write_data(
 		me_util, 0xE0E8, OPTIGA_UTIL_ERASE_AND_WRITE, 0, pubkey_der, decoded_len
 	);

 	if (OPTIGA_LIB_SUCCESS == write_status) {
 		// Wait for Trust M operation by polling optiga_lib_status
 		// vTaskDelay() forces subscriber task to sleep, releasing CPU for MQTT task
 		// to receive and process incoming packets (manifest, fragments)
 		TickType_t start_ticks = xTaskGetTickCount();
 		TickType_t timeout_ticks = pdMS_TO_TICKS(1000);
 		bool operation_complete = false;

 		while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
 			// Note: vTaskDelay FIRST to ensure MQTT task gets CPU time
 			vTaskDelay(pdMS_TO_TICKS(10)); // Force sleep to allow MQTT packet reception

 			if (optiga_lib_status != OPTIGA_LIB_BUSY) {
 				operation_complete = true;
 				break;
 			}
 		}
 		trust_m_async_status = optiga_lib_status;  // Copy result

 		if (operation_complete) {
 			if (OPTIGA_LIB_SUCCESS == trust_m_async_status) {
 				#if TESAIOT_DEBUG_VERBOSE_ENABLED
 				printf("%s TrustAnchor-OK\n", LABEL_SUBSCRIBER);
 				#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			} else {
 				#if TESAIOT_DEBUG_VERBOSE_ENABLED
 				printf("%s TrustAnchor-FAIL:0x%04X\n", LABEL_SUBSCRIBER, trust_m_async_status);
 				#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 			}
 		} else {
 			#if TESAIOT_DEBUG_VERBOSE_ENABLED
 			printf("%s TrustAnchor-timeout\n", LABEL_SUBSCRIBER);
 			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 		}
 	} else {
 		#if TESAIOT_DEBUG_VERBOSE_ENABLED
 		printf("%s TrustAnchor-write-fail:0x%04X\n", LABEL_SUBSCRIBER, write_status);
 		#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	}

 	// Release (not destroy) persistent OPTIGA instance
 	optiga_manager_release();
 	pubkey = pubkey_der;
 	pubkey_length = decoded_len;
 	vPortFree(pubkey_buf);
 	break;
 }
 #endif // End of deprecated handlers

 default:
 	#if TESAIOT_DEBUG_VERBOSE_ENABLED
 	printf("%s WARN: Unknown cmd %d\n", LABEL_SUBSCRIBER, (int)subscriber_q_data.cmd);
 	#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
 	break;
 }
 } // End of message received scope
 } // End of while(true) loop

/******************************************************************************
 * Function Name: mqtt_subscription_callback
 ******************************************************************************
 * Summary:
 * Callback to handle incoming MQTT messages. This callback prints the 
 * contents of the incoming message and informs the subscriber task, via a 
 * message queue, to turn on / turn off the device based on the received 
 * message.
 *
 * Parameters:
 * cy_mqtt_publish_info_t *received_msg_info : Information structure of the 
 * received MQTT message
 *
 * Return:
 * void
 *
 ******************************************************************************/
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info)
{
 /* Received MQTT message */
 const char *received_msg = received_msg_info->payload;
 int received_msg_len = received_msg_info->payload_len;
 const char *received_topic = received_msg_info->topic;
 int received_topic_len = received_msg_info->topic_len;

 /* Data to be sent to the subscriber task queue. */
 subscriber_data_t subscriber_q_data;
 bool valid_topic = false;

 // Increment total callback counter (atomic operation, safe in ISR/callback)
 callback_count_total++;

 // Match topic - NO printf to avoid UART overflow!
 // Check new single-message protected_update topic FIRST
 if ((strlen(MQTT_SUB_TOPIC_COMMAND_PROTECTED_UPDATE) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_PROTECTED_UPDATE, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPDATE_PROTECTED_UPDATE_BUNDLE;
 // Note: Copy data to prevent buffer reuse by MQTT library
 char *data_copy = (char*)pvPortMalloc(received_msg_len + 1);
 if (data_copy) {
 memcpy(data_copy, received_msg, received_msg_len);
 data_copy[received_msg_len] = '\0';
 subscriber_q_data.data = data_copy;
 subscriber_q_data.data_size = received_msg_len;
 subscriber_q_data.need_free = true;
 valid_topic = true;
 }
 // No separate counter - use callback_count_total
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_CERT) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_CERT, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPDATE_DEVICE_CERTIFICATES;
 // Note: Copy data to prevent buffer reuse by MQTT library
 char *data_copy = (char*)pvPortMalloc(received_msg_len + 1);
 if (data_copy) {
 memcpy(data_copy, received_msg, received_msg_len);
 data_copy[received_msg_len] = '\0';
 subscriber_q_data.data = data_copy;
 subscriber_q_data.data_size = received_msg_len;
 subscriber_q_data.need_free = true;
 valid_topic = true;
 callback_count_cert++;
 }
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_CHECK_CERT_RESPONSE) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_CHECK_CERT_RESPONSE, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = CHECK_CERTIFICATE_RESPONSE;
 // Note: Copy data to prevent buffer reuse by MQTT library
 char *data_copy = (char*)pvPortMalloc(received_msg_len + 1);
 if (data_copy) {
 memcpy(data_copy, received_msg, received_msg_len);
 data_copy[received_msg_len] = '\0';
 subscriber_q_data.data = data_copy;
 subscriber_q_data.data_size = received_msg_len;
 subscriber_q_data.need_free = true;
 valid_topic = true;
 }
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_UPLOAD_CERT_RESPONSE) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_UPLOAD_CERT_RESPONSE, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPLOAD_CERTIFICATE_RESPONSE;
 // Note: Copy data to prevent buffer reuse by MQTT library
 char *data_copy = (char*)pvPortMalloc(received_msg_len + 1);
 if (data_copy) {
 memcpy(data_copy, received_msg, received_msg_len);
 data_copy[received_msg_len] = '\0';
 subscriber_q_data.data = data_copy;
 subscriber_q_data.data_size = received_msg_len;
 subscriber_q_data.need_free = true;
 valid_topic = true;
 }
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_SYNC_CERT_RESPONSE) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_SYNC_CERT_RESPONSE, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = SYNC_CERTIFICATE_RESPONSE; // Unified sync response
 // Note: Copy data to prevent buffer reuse by MQTT library
 char *data_copy = (char*)pvPortMalloc(received_msg_len + 1);
 if (data_copy) {
 memcpy(data_copy, received_msg, received_msg_len);
 data_copy[received_msg_len] = '\0';
 subscriber_q_data.data = data_copy;
 subscriber_q_data.data_size = received_msg_len;
 subscriber_q_data.need_free = true;
 valid_topic = true;
 }
 }
 #if 0 // DEPRECATED: Old 4-message protocol replaced by single JSON message (2025-10-22)
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_MANIFEST) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_MANIFEST, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPDATE_DEVICE_MANIFEST;
 subscriber_q_data.data = received_msg;
 subscriber_q_data.data_size = received_msg_len;
 valid_topic = true;
 callback_count_manifest++;
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_FRAGMENT) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_FRAGMENT, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPDATE_DEVICE_FRAGMENT;
 subscriber_q_data.data = received_msg;
 subscriber_q_data.data_size = received_msg_len;
 valid_topic = true;
 callback_count_fragment++;
 }
 else if ((strlen(MQTT_SUB_TOPIC_COMMAND_PUB_KEY) == received_topic_len) &&
 (strncmp(MQTT_SUB_TOPIC_COMMAND_PUB_KEY, received_topic, received_topic_len) == 0))
 {
 subscriber_q_data.cmd = UPDATE_DEVICE_PUBLIC_KEY;
 subscriber_q_data.data = received_msg;
 subscriber_q_data.data_size = received_msg_len;
 valid_topic = true;
 callback_count_pubkey++;
 }
 #endif // End of deprecated topic routing
 else
 {
 callback_count_unknown++;
 }

 // Note: NO printf() in callback to avoid UART buffer overflow blocking!
 // If UART buffer full -> printf blocks -> callback blocks -> MQTT library cannot receive new packets!
 // Note: xQueueSend MUST NOT BLOCK - use timeout=0!
 // Blocking even 100ms prevents MQTT library from processing next packets!

 if (!valid_topic || subscriber_task_q == NULL) {
 return; // Silently drop invalid messages
 }

 // Queue message - subscriber task will process and print
 // MUST use timeout=0 (no blocking!) - callback MUST return immediately
 // Queue size 20 is large enough to buffer all messages during Trust M write
 // If queue full -> drop message (should never happen with queue size 20)
 xQueueSend(subscriber_task_q, &subscriber_q_data, 0);
}


/* [] END OF FILE */
