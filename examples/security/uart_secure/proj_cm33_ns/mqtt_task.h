/******************************************************************************
* File Name: mqtt_task.h
*
* Description: MQTT Client Task - manages connection lifecycle to TESAIoT Platform
*
* This task handles the MQTT connection state machine:
*   - Wi-Fi connection management
*   - TLS mutual authentication with OPTIGA Trust M certificates
*   - MQTT broker connection (TESAIoT Platform)
*   - Reconnection on disconnection
*   - Error handling for subscribe/publish failures
*
* Certificate selection:
*   Uses tesaiot_select_mqtt_certificate() for Smart Auto-Fallback
*   - Primary: Device Certificate (OID 0xE0E1) - TESAIoT issued
*   - Fallback: Factory Certificate (OID 0xE0E0) - Infineon provisioned
*
* Thread coordination:
*   - Spawns subscriber_task and publisher_task
*   - Coordinates via mqtt_task_q for error notifications
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

#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "cy_mqtt_api.h"


/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for MQTT Client Task. */
// Priority test result: Priority 4 FAILED - caused MQTT connection hang
// MQTT task priority 4 -> monopolizes CPU -> WiFi/TLS stack tasks (priority 2-3) starved
// TLS handshake needs cooperation between tasks -> Deadlock at "root_ca loaded"
// CONCLUSION: Task Priority Inversion is NOT the root cause of Error 0x0806000B
//
// Solution: Revert to priority 2, test ONLY dummy init publish
#define MQTT_CLIENT_TASK_PRIORITY (2U) // Keep same as subscriber/publisher for round-robin
#define MQTT_CLIENT_TASK_STACK_SIZE (1024U * 2U)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the MQTT Client Task. */
typedef enum
{
 HANDLE_MQTT_SUBSCRIBE_FAILURE,
 HANDLE_MQTT_PUBLISH_FAILURE,
 HANDLE_DISCONNECTION
} mqtt_task_cmd_t;

/*******************************************************************************
 * Extern variables
 ******************************************************************************/
extern cy_mqtt_t mqtt_connection;
extern QueueHandle_t mqtt_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void mqtt_client_task(void *pvParameters);
bool mqtt_request_start(void);
bool mqtt_is_started(void);
bool mqtt_is_connected(void);

/**
 * Force MQTT reconnection to clear internal state (including publish slots).
 * Use this as a workaround for slot exhaustion issues.
 * Returns true if reconnect successful, false otherwise.
 */
bool mqtt_force_reconnect(void);

/**
 * Request MQTT task to stop and cleanup.
 * This will disconnect from broker and terminate subscriber/publisher tasks.
 * Use before CSR workflow to avoid conflicts.
 * Returns true if stop request successful, false otherwise.
 */
bool mqtt_request_stop(void);

/**
 * Request MQTT start in lightweight mode (no subscriber/publisher tasks).
 * Use for MQTT connection test - direct subscribe/publish without FreeRTOS tasks.
 * After connection, use mqtt_connection directly for cy_mqtt_subscribe/publish.
 * Returns true if start request successful, false otherwise.
 */
bool mqtt_request_start_lightweight(void);

/**
 * Stop lightweight MQTT connection.
 * Disconnects and cleans up without trying to terminate subscriber/publisher tasks.
 * Returns true if stop successful, false otherwise.
 */
bool mqtt_stop_lightweight(void);

#endif /* MQTT_TASK_H_ */

/* [] END OF FILE */
