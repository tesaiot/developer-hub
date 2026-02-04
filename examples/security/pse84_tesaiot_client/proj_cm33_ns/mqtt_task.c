/******************************************************************************
* File Name:   mqtt_task.c
*
* Description: This file contains the task that handles initialization & 
*              connection of Wi-Fi and the MQTT client. The task then starts 
*              the subscriber and the publisher tasks. The task also implements
*              reconnection mechanisms to handle WiFi and MQTT disconnections.
*              The task also handles all the cleanup operations to gracefully 
*              terminate the Wi-Fi and MQTT connections in case of any failure.
*
* Related Document: See README.md
*
*
*******************************************************************************
* * Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
* *
* * This software, including source code, documentation and related
* * materials ("Software") is owned by Cypress Semiconductor Corporation
* * or one of its affiliates ("Cypress") and is protected by and subject to
* * worldwide patent protection (United States and foreign),
* * United States copyright laws and international treaty provisions.
* * Therefore, you may use this Software only as provided in the license
* * agreement accompanying the software package from which you
* * obtained this Software ("EULA").
* * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* * non-transferable license to copy, modify, and compile the Software
* * source code solely for use in connection with Cypress's
* * integrated circuit products.  Any reproduction, modification, translation,
* * compilation, or representation of this Software except as specified
* * above is prohibited without the express written permission of Cypress.
* *
* * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* * reserves the right to make changes to the Software without notice. Cypress
* * does not assume any liability arising out of the application or use of the
* * Software or any product or circuit described in the Software. Cypress does
* * not authorize its products for use in any products where a malfunction or
* * failure of the Cypress product may reasonably be expected to result in
* * significant property damage, injury or death ("High Risk Product"). By
* * including Cypress's product in a High Risk Product, the manufacturer
* * of such system or application assumes all risk of such use and in doing
* * so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/******************************************************************************
* TESAIoT Platform Integration
*******************************************************************************
* SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This file has been modified by the TESAIoT Platform Developer Team to add:
* - Integration with TESAIoT AIoT Foundation Platform MQTT broker
* - NTP time synchronization after Wi-Fi connection (for certificate validation)
* - Certificate Lifecycle Management (device cert selection and fallback)
* - Auto-renewal trigger after Factory Certificate fallback
* - OPTIGA Trust M certificate selection before MQTT connection
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

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* Task header files */
#include "mqtt_task.h"
#include "subscriber_task.h"
#include "publisher_task.h"

/* Configuration file for Wi-Fi and MQTT client */
#include "wifi_config.h"
#include "mqtt_client_config.h"

/* TESAIoT OPTIGA Trust M integration for certificate lifecycle management */
#include "tesaiot_optiga.h"

/* OPTIGA PSA Secure Element driver for dynamic key OID selection */
#include "optiga_psa_se.h"

/* TESAIoT platform services (MQTT, SNTP) */
#include "tesaiot_platform.h"

/* TESAIoT configuration and debug labels */
#include "tesaiot_config.h"

/* Middleware libraries */
#include "cy_retarget_io.h"
#include "cy_wcm.h"

#include "cy_mqtt_api.h"
#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"
#include "retarget_io_init.h"


/******************************************************************************
* Macros
******************************************************************************/
/* Queue length of a message queue that is used to communicate the status of 
 * various operations.
 */
#define MQTT_TASK_QUEUE_LENGTH           (3u)

/* Time in milliseconds to wait before creating the publisher task. */
#define TASK_CREATION_DELAY_MS           (2000u)

/* Flag Masks for tracking which cleanup functions must be called. */
#define WCM_INITIALIZED                             (1lu << 0)
#define WIFI_CONNECTED                              (1lu << 1)
#define LIBS_INITIALIZED                            (1lu << 2)
#define BUFFER_INITIALIZED                          (1lu << 3)
#define MQTT_INSTANCE_CREATED                       (1lu << 4)
#define MQTT_CONNECTION_SUCCESS                     (1lu << 5)
#define MQTT_MSG_RECEIVED                           (1lu << 6)
#define TIME_DIV_MS                                 (60000u)
#define APP_SDIO_INTERRUPT_PRIORITY                 (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY            (2U)
#define APP_SDIO_FREQUENCY_HZ                       (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK                     (64U)

/*String that describes the MQTT handle that is being created in order to uniquely identify it*/
#define MQTT_HANDLE_DESCRIPTOR                       "MQTThandleID"

/* Macro to check if the result of an operation was successful and set the 
 * corresponding bit in the status_flag based on 'init_mask' parameter. When 
 * it has failed, print the error message and return the result to the 
 * calling function.
 */
#define CHECK_RESULT(result, init_mask, error_message...)      \
                     do                                        \
                     {                                         \
                         if ((int)result == CY_RSLT_SUCCESS)   \
                         {                                     \
                             status_flag |= init_mask;         \
                         }                                     \
                         else                                  \
                         {                                     \
                             printf(error_message);            \
                             return result;                    \
                         }                                     \
                     } while(0)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT connection handle. */
cy_mqtt_t mqtt_connection;

/* Queue handle used to communicate results of various operations - MQTT 
 * Publish, MQTT Subscribe, MQTT connection, and Wi-Fi connection between tasks 
 * and callbacks.
 */
QueueHandle_t mqtt_task_q;

/* Flag to denote initialization status of various operations. */
uint32_t status_flag;

/* Pointer to the network buffer needed by the MQTT library for MQTT send and 
 * receive operations.
 */
uint8_t *mqtt_network_buffer = NULL;
static mtb_hal_sdio_t sdio_instance;
cy_stc_sd_host_context_t sdhc_host_context;
static cy_wcm_config_t wcm_config;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

/* SysPm callback parameter structure for SDHC */
static cy_stc_syspm_callback_params_t sdcardDSParams =
{
    .context   = &sdhc_host_context,
    .base      = CYBSP_WIFI_SDIO_HW
};

/* SysPm callback structure for SDHC*/
static cy_stc_syspm_callback_t sdhcDeepSleepCallbackHandler =
{
    .callback           = Cy_SD_Host_DeepSleepCallback,
    .skipMode           = SYSPM_SKIP_MODE,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &sdcardDSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};
#endif

static EventGroupHandle_t mqtt_control_events;
static bool mqtt_start_requested = false;
static bool mqtt_started = false;

/* Lightweight mode flag - when true, mqtt_task skips subscriber/publisher task creation.
 * Used by MQTT connection test for direct loopback without FreeRTOS tasks overhead.
 */
static bool g_mqtt_lightweight_mode = false;
static bool mqtt_stop_requested = false;

#define MQTT_CTRL_START_BIT               (1U)

/******************************************************************************
* Function Prototypes
*******************************************************************************/

#if GENERATE_UNIQUE_CLIENT_ID
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier);
#endif /* GENERATE_UNIQUE_CLIENT_ID */


bool mqtt_request_start(void)
{
    if (mqtt_control_events == NULL)
    {
        return false;
    }

    if (mqtt_start_requested)
    {
        return true;
    }

    mqtt_start_requested = true;
    (void)xEventGroupSetBits(mqtt_control_events, MQTT_CTRL_START_BIT);
    return true;
}

bool mqtt_is_started(void)
{
    return mqtt_started;
}

bool mqtt_is_connected(void)
{
    return (status_flag & MQTT_CONNECTION_SUCCESS) != 0U;
}

/**
 * Force MQTT reconnection to clear internal state (including publish slots).
 * This is a workaround for slot exhaustion (0x0806000B) where slots are not
 * being properly cleaned up by the MQTT library.
 */
bool mqtt_force_reconnect(void)
{
    printf("[MQTT-Reconnect] Starting forced reconnection to clear slots...\n");
    fflush(stdout);

    if (mqtt_connection == NULL)
    {
        printf("[MQTT-Reconnect] ERROR: mqtt_connection is NULL\n");
        return false;
    }

    /* Step 1: Disconnect */
    printf("[MQTT-Reconnect] Disconnecting...\n");
    cy_rslt_t result = cy_mqtt_disconnect(mqtt_connection);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("[MQTT-Reconnect] Disconnect returned 0x%08X (may be OK if already disconnected)\n",
               (unsigned int)result);
    }

    /* Clear connection flag */
    status_flag &= ~(MQTT_CONNECTION_SUCCESS);

    /* Wait a bit for cleanup */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Step 2: Reconnect */
    printf("[MQTT-Reconnect] Reconnecting with clean_session=true...\n");
    result = cy_mqtt_connect(mqtt_connection, &connection_info);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("[MQTT-Reconnect] ERROR: Reconnect failed with 0x%08X\n", (unsigned int)result);
        return false;
    }

    /* Set connection flag */
    status_flag |= MQTT_CONNECTION_SUCCESS;

    printf("[MQTT-Reconnect] Reconnection successful! Slots should be cleared.\n");
    fflush(stdout);

    /* Wait for stack to stabilize */
    vTaskDelay(pdMS_TO_TICKS(1000));

    return true;
}

/**
 * Request MQTT task to stop and cleanup.
 * This will disconnect from broker and terminate subscriber/publisher tasks.
 * Use before CSR workflow to avoid conflicts with existing MQTT session.
 *
 * Note: This does NOT disconnect Wi-Fi - only MQTT session is terminated.
 * The mqtt_request_start() can be called again to reconnect after CSR.
 */
bool mqtt_request_stop(void)
{
    printf("[MQTT-Stop] Stopping MQTT session for CSR workflow...\n");
    fflush(stdout);

    /* Step 1: Check if MQTT is actually running */
    if (!mqtt_started)
    {
        printf("[MQTT-Stop] MQTT not started, nothing to stop\n");
        return true;
    }

    /* Step 2: Terminate publisher task if running */
    if (publisher_task_handle != NULL)
    {
        printf("[MQTT-Stop] Terminating publisher task...\n");
        vTaskDelete(publisher_task_handle);
        publisher_task_handle = NULL;
    }

    /* Step 3: Terminate subscriber task if running */
    if (subscriber_task_handle != NULL)
    {
        printf("[MQTT-Stop] Terminating subscriber task...\n");
        vTaskDelete(subscriber_task_handle);
        subscriber_task_handle = NULL;
    }

    /* Step 4: Disconnect from MQTT broker */
    if (status_flag & MQTT_CONNECTION_SUCCESS)
    {
        printf("[MQTT-Stop] Disconnecting from broker...\n");
        cy_rslt_t result = cy_mqtt_disconnect(mqtt_connection);
        if (result != CY_RSLT_SUCCESS)
        {
            printf("[MQTT-Stop] Disconnect returned 0x%08X (may be OK)\n",
                   (unsigned int)result);
        }
        status_flag &= ~(MQTT_CONNECTION_SUCCESS);
    }

    /* Step 5: Delete MQTT instance */
    if (status_flag & MQTT_INSTANCE_CREATED)
    {
        printf("[MQTT-Stop] Deleting MQTT instance...\n");
        cy_rslt_t result = cy_mqtt_delete(mqtt_connection);
        if (result != CY_RSLT_SUCCESS)
        {
            printf("[MQTT-Stop] Delete returned 0x%08X (may be OK)\n",
                   (unsigned int)result);
        }
        status_flag &= ~(MQTT_INSTANCE_CREATED);
        mqtt_connection = NULL;
    }

    /* Step 6: Free network buffer */
    if (status_flag & BUFFER_INITIALIZED)
    {
        printf("[MQTT-Stop] Freeing network buffer...\n");
        vPortFree((void *)mqtt_network_buffer);
        mqtt_network_buffer = NULL;
        status_flag &= ~(BUFFER_INITIALIZED);
    }

    /* Step 7: Deinit MQTT library */
    if (status_flag & LIBS_INITIALIZED)
    {
        printf("[MQTT-Stop] Deinitializing MQTT library...\n");
        cy_mqtt_deinit();
        status_flag &= ~(LIBS_INITIALIZED);
    }

    /* Step 8: Reset state flags to allow fresh start */
    mqtt_started = false;
    mqtt_start_requested = false;

    /* Step 9: Reset workflow flags to prevent stale state from affecting next MQTT session */
    g_csr_workflow_active = false;
    g_protected_update_active = false;
    g_protected_update_just_completed = false;

    printf("[MQTT-Stop] MQTT session stopped successfully\n");
    printf("[MQTT-Stop] Wi-Fi remains connected for CSR workflow\n");
    fflush(stdout);

    /* Wait for resources to be released */
    vTaskDelay(pdMS_TO_TICKS(500));

    return true;
}

/**
 * Request MQTT start in lightweight mode.
 * Sets flag to skip subscriber/publisher task creation.
 * Used by MQTT connection test for direct loopback.
 */
bool mqtt_request_start_lightweight(void)
{
    printf("[MQTT-Lightweight] Requesting lightweight MQTT start (no sub/pub tasks)...\n");
    fflush(stdout);

    g_mqtt_lightweight_mode = true;
    return mqtt_request_start();
}

/**
 * Stop lightweight MQTT connection.
 * Signals mqtt_task() to cleanup and waits for completion.
 */
bool mqtt_stop_lightweight(void)
{
    if (!g_mqtt_lightweight_mode)
    {
        printf("[MQTT-Lightweight] Not in lightweight mode, nothing to stop.\n");
        return false;
    }

    printf("[MQTT-Lightweight] Requesting stop...\n");
    fflush(stdout);

    /* Signal mqtt_task() to cleanup */
    mqtt_stop_requested = true;

    /* Wait for mqtt_task() to complete cleanup (max 5 seconds) */
    int timeout_ms = 5000;
    while (mqtt_started && timeout_ms > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout_ms -= 100;
    }

    if (mqtt_started)
    {
        printf("[MQTT-Lightweight] WARNING: Timeout waiting for mqtt_task cleanup\n");
        return false;
    }

    printf("[MQTT-Lightweight] Stopped successfully. Wi-Fi remains connected.\n");
    fflush(stdout);

    return true;
}


/******************************************************************************
* Function Prototypes
*******************************************************************************/

#if GENERATE_UNIQUE_CLIENT_ID
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier);
#endif /* GENERATE_UNIQUE_CLIENT_ID */


/******************************************************************************
 * Function Name: cleanup
 ******************************************************************************
 * Summary:
 *  Function that invokes the deinit and cleanup functions for various
 *  operations based on the status_flag.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/

/**
 * Cleanup MQTT only - keep Wi-Fi connected.
 * Used by lightweight mode to disconnect MQTT without touching Wi-Fi.
 */
static void cleanup_mqtt_only(void)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    /* Disconnect the MQTT connection if it was established. */
    if (status_flag & MQTT_CONNECTION_SUCCESS)
    {
        status = cy_mqtt_disconnect(mqtt_connection);
        if (CY_RSLT_SUCCESS == status)
        {
            printf("Disconnected from the MQTT Broker...\n");
        }
        status_flag &= ~(MQTT_CONNECTION_SUCCESS);
    }

    /* Delete the MQTT instance if it was created. */
    if (status_flag & MQTT_INSTANCE_CREATED)
    {
        status = cy_mqtt_delete(mqtt_connection);
        if (CY_RSLT_SUCCESS == status)
        {
            printf("Removed MQTT connection info from stack...\n");
        }
        status_flag &= ~(MQTT_INSTANCE_CREATED);
        mqtt_connection = NULL;
    }

    /* Deallocate the network buffer. */
    if (status_flag & BUFFER_INITIALIZED)
    {
        vPortFree((void *)mqtt_network_buffer);
        mqtt_network_buffer = NULL;
        status_flag &= ~(BUFFER_INITIALIZED);
    }

    /* Skip cy_mqtt_deinit() to preserve TLS layer state between connections.
     * This fixes the TLS handshake failure (0x8060009) when running CSR workflow after MQTT test.
     * Full cleanup() will call cy_mqtt_deinit() when needed.
     */
    printf("[MQTT-Lightweight] Keeping MQTT library initialized for TLS state preservation\n");

    /* Reset MQTT state flags - keep Wi-Fi and LIBS_INITIALIZED flags intact */
    mqtt_started = false;
    mqtt_start_requested = false;
}

static void cleanup(void)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    /* Disconnect the MQTT connection if it was established. */
    if (status_flag & MQTT_CONNECTION_SUCCESS)
    {
        status = cy_mqtt_disconnect(mqtt_connection);

        if (CY_RSLT_SUCCESS == status)
        {
            printf("Disconnected from the MQTT Broker...\n");
        }
        else
        {
            printf("MQTT disconnect API failed unexpectedly.\n");
        }
        status_flag &= ~(MQTT_CONNECTION_SUCCESS);
    }
    /* Delete the MQTT instance if it was created. */
    if (status_flag & MQTT_INSTANCE_CREATED)
    {
        status = cy_mqtt_delete(mqtt_connection);

        if (CY_RSLT_SUCCESS == status)
        {
            printf("Removed MQTT connection info from stack...\n");
        }
        else
        {
            printf("MQTT delete API failed unexpectedly.\n");
        }
        status_flag &= ~(MQTT_INSTANCE_CREATED);
        mqtt_connection = NULL;
    }
    /* Deallocate the network buffer. */
    if (status_flag & BUFFER_INITIALIZED)
    {
        vPortFree((void *) mqtt_network_buffer);
        mqtt_network_buffer = NULL;
        status_flag &= ~(BUFFER_INITIALIZED);
    }
    /* Deinit the MQTT library. */
    if (status_flag & LIBS_INITIALIZED)
    {
        status = cy_mqtt_deinit();

        if (CY_RSLT_SUCCESS == status)
        {
            printf("Deinitialized MQTT stack...\n");
        }
        else
        {
            printf("MQTT deinit API failed unexpectedly.\n");
        }
        status_flag &= ~(LIBS_INITIALIZED);
    }
    /* Disconnect from Wi-Fi AP. */
    if (status_flag & WIFI_CONNECTED)
    {
        status = cy_wcm_disconnect_ap();

        if (CY_RSLT_SUCCESS == status)
        {
            printf("Disconnected from the Wi-Fi AP!\n");
        }
        else
        {
            printf("WCM disconnect AP failed unexpectedly.\n");
        }
        status_flag &= ~(WIFI_CONNECTED);
    }

    /* Reset MQTT state flags */
    mqtt_started = false;
    mqtt_start_requested = false;
    /* De-initialize the Wi-Fi Connection Manager. */
    if (status_flag & WCM_INITIALIZED)
    {
        status = cy_wcm_deinit();

        if (CY_RSLT_SUCCESS == status)
        {
            printf("Deinitialized Wifi connection...\n");
        }
        else
        {
            printf("WCM deinit API failed unexpectedly.\n");
        }
    }
}


/******************************************************************************
 * Function Name: wifi_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates connection to the Wi-Fi Access Point using the
 *  specified SSID and PASSWORD. The connection is retried a maximum of
 *  'MAX_WIFI_CONN_RETRIES' times with interval of 'WIFI_CONN_RETRY_INTERVAL_MS'
 *  milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful Wi-Fi connection, else an
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;

    /* Check if Wi-Fi connection is already established. */
    if (!(cy_wcm_is_connected_to_ap()))
    {
        /* Configure the connection parameters for the Wi-Fi interface. */
        memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
        memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
        memcpy(connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
        connect_param.ap_credentials.security = WIFI_SECURITY;

        printf("\nWi-Fi Connecting to '%s'\n", connect_param.ap_credentials.SSID);

        /* Connect to the Wi-Fi AP. */
        for (uint32_t retry_count = 0; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++)
        {
            result = cy_wcm_connect_ap(&connect_param, &ip_address);

            if (CY_RSLT_SUCCESS == result)
            {
                printf("\nSuccessfully connected to Wi-Fi network '%s'.\n", connect_param.ap_credentials.SSID);

                /* Set the appropriate bit in the status_flag to denote
                 * successful Wi-Fi connection, print the assigned IP address.
                 */
                status_flag |= WIFI_CONNECTED;
                if (ip_address.version == CY_WCM_IP_VER_V4)
                {
                    printf("IPv4 Address Assigned: %s\n\n", ip4addr_ntoa((const ip4_addr_t *) &ip_address.ip.v4));
                }
                else if (ip_address.version == CY_WCM_IP_VER_V6)
                {
                    printf("IPv6 Address Assigned: %s\n\n", ip6addr_ntoa((const ip6_addr_t *) &ip_address.ip.v6));
                }
                return result;
            }

            printf("Wi-Fi Connection failed. Error code:0x%0X. Retrying in %d ms. Retries left: %d\n",
                (int)result, WIFI_CONN_RETRY_INTERVAL_MS, (int)(MAX_WIFI_CONN_RETRIES - retry_count - 1));
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
        }

        printf("\nExceeded maximum Wi-Fi connection attempts!\n");
        printf("Wi-Fi connection failed after retrying for %d mins\n\n",
            (int)(WIFI_CONN_RETRY_INTERVAL_MS * MAX_WIFI_CONN_RETRIES) / TIME_DIV_MS);
    }
    return result;
}

/******************************************************************************
 * Function Name: mqtt_event_callback
 ******************************************************************************
 * Summary:
 *  Callback invoked by the MQTT library for events like MQTT disconnection,
 *  incoming MQTT subscription messages from the MQTT broker.
 *    1. In case of MQTT disconnection, the MQTT client task is communicated
 *       about the disconnection using a message queue.
 *    2. When an MQTT subscription message is received, the subscriber callback
 *       function implemented in subscriber_task.c is invoked to handle the
 *       incoming MQTT message.
 *
 * Parameters:
 *  cy_mqtt_t mqtt_handle : MQTT handle corresponding to the MQTT event (unused)
 *  cy_mqtt_event_t event : MQTT event information
 *  void *user_data : User data pointer passed during cy_mqtt_create() (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data)
{
    cy_mqtt_publish_info_t *received_msg;
    mqtt_task_cmd_t mqtt_task_cmd;

    CY_UNUSED_PARAMETER(mqtt_handle);
    CY_UNUSED_PARAMETER(user_data);

    switch(event.type)
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT:
        {
            /* Clear the status flag bit to indicate MQTT disconnection. */
            status_flag &= ~(MQTT_CONNECTION_SUCCESS);

            /* MQTT connection with the MQTT broker is broken as the client
             * is unable to communicate with the broker. Set the appropriate
             * command to be sent to the MQTT task.
             */
            /* Print disconnect reason for debugging */
            const char* reason_str = "UNKNOWN";
            switch (event.data.reason) {
                case CY_MQTT_DISCONN_TYPE_BROKER_DOWN:
                    reason_str = "BROKER_DOWN (keepalive timeout)";
                    break;
                case CY_MQTT_DISCONN_TYPE_NETWORK_DOWN:
                    reason_str = "NETWORK_DOWN (Wi-Fi lost)";
                    break;
                case CY_MQTT_DISCONN_TYPE_BAD_RESPONSE:
                    reason_str = "BAD_RESPONSE (invalid packet)";
                    break;
                case CY_MQTT_DISCONN_TYPE_SND_RCV_FAIL:
                    reason_str = "SND_RCV_FAIL (timeout/latency)";
                    break;
            }
            printf("\nUnexpectedly disconnected from MQTT broker!\n");
            printf("[MQTT-Event] Disconnect reason: %s (code=%d)\n", reason_str, (int)event.data.reason);
            mqtt_task_cmd = HANDLE_DISCONNECTION;

            /* Send the message to the MQTT client task to handle the
             * disconnection.
             */
            xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
            break;
        }

        case CY_MQTT_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECEIVE:
        {
            status_flag |= MQTT_MSG_RECEIVED;

            /* Incoming MQTT message has been received. Send this message to
             * the subscriber callback function to handle it.
             */

            received_msg = &(event.data.pub_msg.received_message);

            mqtt_subscription_callback(received_msg);
            break;
        }
        default :
        {
            /* Unknown MQTT event */
            printf("\nUnknown Event received from MQTT callback!\n");
            break;
        }
    }
}

/******************************************************************************
 * Function Name: mqtt_init
 ******************************************************************************
 * Summary:
 *  Function that initializes the MQTT library and creates an instance for the
 *  MQTT client. The network buffer needed by the MQTT library for MQTT send
 *  send and receive operations is also allocated by this function.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on a successful initialization, else an error
 *              code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_init(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the MQTT library only if not already initialized.
     * When returning from lightweight mode, LIBS_INITIALIZED is kept set
     * to preserve TLS state and avoid handshake failures.
     */
    if (!(status_flag & LIBS_INITIALIZED))
    {
        result = cy_mqtt_init();
        CHECK_RESULT(result, LIBS_INITIALIZED, "\nMQTT library initialization failed!\n");
    }
    else
    {
        printf("[MQTT] Library already initialized, reusing TLS state\n");
    }

    /* Allocate buffer for MQTT send and receive operations. */
    mqtt_network_buffer = (uint8_t *) pvPortMalloc(sizeof(uint8_t) * MQTT_NETWORK_BUFFER_SIZE);
    if(NULL == mqtt_network_buffer)
    {
        result = ~CY_RSLT_SUCCESS;
    }
    CHECK_RESULT(result, BUFFER_INITIALIZED, "Network Buffer allocation failed!\n\n");

    /* Select and stage certificate for TLS before creating MQTT client
     * Only needed for mTLS mode - Server-TLS uses password authentication only */
#if MQTT_ENABLE_MUTUAL_AUTH
    uint16_t selected_cert_oid = tesaiot_select_mqtt_certificate();

    bool cert_staged = false;
    if (selected_cert_oid == 0xE0E1) {
        /* Device Certificate selected (preferred) - use corresponding key 0xE0F1 */
        optiga_psa_set_signing_key_oid(OPTIGA_KEY_ID_E0F1);
        cert_staged = trustm_use_device_certificate();
    } else if (selected_cert_oid == 0xE0E0) {
        /* Factory Certificate selected (fallback mode) - use corresponding key 0xE0F0 */
        optiga_psa_set_signing_key_oid(OPTIGA_KEY_ID_E0F0);
        cert_staged = trustm_use_factory_certificate();
    } else {
        #if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Invalid certificate OID returned: 0x%04X\n", LABEL_MQTT, selected_cert_oid);
        #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
        result = ~CY_RSLT_SUCCESS;
    }

    if (!cert_staged) {
        #if TESAIOT_DEBUG_VERBOSE_ENABLED
        printf("%s ERROR: Failed to stage certificate for TLS\n", LABEL_MQTT);
        #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
        result = ~CY_RSLT_SUCCESS;
    }
    CHECK_RESULT(result, LIBS_INITIALIZED, "\nCertificate staging failed!\n");
#else
    /* Server-TLS mode: No client certificate needed, using API key authentication */
    printf("[MQTT] Server-TLS mode: Using API key authentication (no client cert)\n");
#endif /* MQTT_ENABLE_MUTUAL_AUTH */

    /* Create the MQTT client instance. */
    result = cy_mqtt_create(mqtt_network_buffer, MQTT_NETWORK_BUFFER_SIZE,
                            security_info, &broker_info,MQTT_HANDLE_DESCRIPTOR,
                            &mqtt_connection);

    CHECK_RESULT(result, MQTT_INSTANCE_CREATED, "\nMQTT instance creation failed!\n");
    if(CY_RSLT_SUCCESS == result)
    {
        /* Register a MQTT event callback */
        result = cy_mqtt_register_event_callback( mqtt_connection, (cy_mqtt_callback_t)mqtt_event_callback, NULL );
        if(CY_RSLT_SUCCESS == result)
        {
            printf("\nMQTT library initialization successful.\n");
        }
    }
    return result;
}

/******************************************************************************
 * Function Name: mqtt_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates MQTT connect operation. The connection is retried
 *  a maximum of 'MAX_MQTT_CONN_RETRIES' times with interval of
 *  'MQTT_CONN_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful MQTT connection, else an
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_connect(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    bool mqtt_conn_status;

    /* MQTT client identifier string. */
    char mqtt_client_identifier[(MQTT_CLIENT_IDENTIFIER_MAX_LEN + 1)] = MQTT_CLIENT_IDENTIFIER;

    /* Configure the user credentials as a part of MQTT Connect packet */
    if (strlen(MQTT_USERNAME) > 0)
    {
        connection_info.username = MQTT_USERNAME;
        connection_info.password = MQTT_PASSWORD;
        connection_info.username_len = sizeof(MQTT_USERNAME) - 1;
        connection_info.password_len = sizeof(MQTT_PASSWORD) - 1;
    }

    /* Generate a unique client identifier with 'MQTT_CLIENT_IDENTIFIER' string
     * as a prefix if the `GENERATE_UNIQUE_CLIENT_ID` macro is enabled.
     */
#if GENERATE_UNIQUE_CLIENT_ID
    result = mqtt_get_unique_client_identifier(mqtt_client_identifier);
    CHECK_RESULT(result, 0, "Failed to generate unique client identifier for the MQTT client!\n");
#endif /* GENERATE_UNIQUE_CLIENT_ID */

    /* Set the client identifier buffer and length. */
    connection_info.client_id = mqtt_client_identifier;
    connection_info.client_id_len = strlen(mqtt_client_identifier);

    printf("\n'%.*s' connecting to MQTT broker '%.*s'...\n",
           connection_info.client_id_len,
           connection_info.client_id,
           broker_info.hostname_len,
           broker_info.hostname);

    for (uint32_t retry_count = 0; retry_count < MAX_MQTT_CONN_RETRIES; retry_count++)
    {
        if (cy_wcm_is_connected_to_ap() == 0)
        {
            printf("\nUnexpectedly disconnected from Wi-Fi network! \nInitiating Wi-Fi reconnection...\n");
            status_flag &= ~(WIFI_CONNECTED);

            /* Initiate Wi-Fi reconnection. */
            result = wifi_connect();
            if (CY_RSLT_SUCCESS != result)
            {
                break;
            }
        }

        mqtt_conn_status = false;

        /* Establish the MQTT connection. */
        result = cy_mqtt_connect(mqtt_connection, &connection_info);

        if (CY_RSLT_SUCCESS == result)
        {
            printf("MQTT connection successful.\r\n");

            /* Set the appropriate bit in the status_flag to denote successful
             * MQTT connection, and return the result to the calling function.
             */
            status_flag |= MQTT_CONNECTION_SUCCESS;
            mqtt_conn_status = true;

            /* Check if connected using fallback certificate and trigger auto-renewal if needed */
            if (tesaiot_is_using_fallback_certificate()) {
                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("\n%s Connected with Factory Certificate (fallback mode)\n", LABEL_MQTT);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                #if TESAIOT_AUTO_CSR_RENEWAL_ENABLED
                /* Auto CSR Renewal enabled - trigger CSR workflow automatically */
                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("%s Triggering automatic Device Certificate renewal...\n", LABEL_MQTT);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                bool renewal_queued = tesaiot_auto_trigger_csr_renewal();
                if (!renewal_queued) {
                    #if TESAIOT_DEBUG_VERBOSE_ENABLED
                    printf("%s WARNING: Failed to queue auto-CSR renewal\n", LABEL_MQTT);
                    printf("%s Device will continue using Factory Certificate until manual renewal\n", LABEL_MQTT);
                    #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
                }
                #else
                /* Auto CSR Renewal disabled - user must run CSR workflow manually */
                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("%s Auto CSR Renewal is DISABLED\n", LABEL_MQTT);
                printf("%s Run CSR Workflow to obtain Device Certificate\n", LABEL_MQTT);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
                #endif /* TESAIOT_AUTO_CSR_RENEWAL_ENABLED */
            } else {
                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("\n%s Connected with Device Certificate (normal mode)\n", LABEL_MQTT);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
            }

            /* CRITICAL DEBUG (2025-10-23): Print actual keepalive value to verify setting */
            #if TESAIOT_DEBUG_VERBOSE_ENABLED
            printf("%s Keepalive interval: %u seconds\n", LABEL_MQTT, (unsigned int)connection_info.keep_alive_sec);
            printf("%s Clean session: %s\n", LABEL_MQTT, connection_info.clean_session ? "true" : "false");
            #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

            /* Yield to allow other tasks (e.g., CSR workflow wait loop) to see the connection status */
            printf("[MQTT-Broker] Yielding before break...\n");
            taskYIELD();
            printf("[MQTT-Broker] Returning from connect_to_broker\n");

            break;
        }

        printf("\nMQTT connection failed with error code 0x%0X. \nRetrying in %d ms. Retries left: %d\n",
               (int)result, MQTT_CONN_RETRY_INTERVAL_MS, (int)(MAX_MQTT_CONN_RETRIES - retry_count - 1));
        vTaskDelay(pdMS_TO_TICKS(MQTT_CONN_RETRY_INTERVAL_MS));
    }

    if(!mqtt_conn_status)
    {
        printf("\nExceeded maximum MQTT connection attempts\n");
        printf("MQTT connection failed after retrying for %d mins\n\n",
               (int)(MQTT_CONN_RETRY_INTERVAL_MS * MAX_MQTT_CONN_RETRIES) / TIME_DIV_MS);
    }
    return result;
}

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: sdio_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for SDIO instance.
*******************************************************************************/
static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

/*******************************************************************************
* Function Name: host_wake_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for the host wake up input pin.
*******************************************************************************/
static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&wcm_config.wifi_host_wake_pin);
}

/*******************************************************************************
* Function Name: app_sdio_init
********************************************************************************
* Summary:
* This function configures and initializes the SDIO instance used in
* communication between the host MCU and the wireless device.
*******************************************************************************/
static void app_sdio_init(void)
{
    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;
    cy_stc_sysint_t sdio_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY
    };

    cy_stc_sysint_t host_wake_intr_cfg =
    {
            .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
            .intrPriority = APP_HOST_WAKE_INTERRUPT_PRIORITY
    };

    /* Initialize the SDIO interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = Cy_SysInt_Init(&sdio_intr_cfg, sdio_interrupt_handler);

    /* SDIO interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO using the HAL object and desired configuration */
    result = mtb_hal_sdio_setup(&sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config, NULL, &sdhc_host_context);

    /* SDIO setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Initialize and Enable SD HOST */
    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW, CYBSP_WIFI_SDIO_sdio_hal_config.host_config, &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    sdio_hal_cfg.frequencyhal_hz = APP_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = SDHC_SDIO_64BYTES_BLOCK;

    /* Configure SDIO */
    mtb_hal_sdio_configure(&sdio_instance, &sdio_hal_cfg);

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
    /* SDHC SysPm callback registration */
    Cy_SysPm_RegisterCallback(&sdhcDeepSleepCallbackHandler);
#endif /* (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) */

    /* Setup GPIO using the HAL object for WIFI WL REG ON  */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin, CYBSP_WIFI_WL_REG_ON_PORT_NUM, CYBSP_WIFI_WL_REG_ON_PIN);

    /* Setup GPIO using the HAL object for WIFI HOST WAKE PIN  */
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin, CYBSP_WIFI_HOST_WAKE_PORT_NUM, CYBSP_WIFI_HOST_WAKE_PIN);

    /* Initialize the Host wakeup interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status_host_wake =  Cy_SysInt_Init(&host_wake_intr_cfg, host_wake_interrupt_handler);

    /* Host wake up interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status_host_wake)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);
}

/******************************************************************************
 * Function Name: terminate_tasks
 ******************************************************************************
 * Summary:
 *  Cleanup section: Delete subscriber and publisher tasks and perform
 *
 * Parameters:
 *  void
 *
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void terminate_tasks(void)
{
    printf("\nTerminating Publisher and Subscriber tasks...\n");
    if (NULL != subscriber_task_handle )
    {
        vTaskDelete(subscriber_task_handle);
    }
    if (NULL != publisher_task_handle )
    {
        vTaskDelete(publisher_task_handle);
    }
    cleanup();
    printf("\nCleanup Done\nTerminating the MQTT task...\n\n");
    vTaskDelete(NULL);
}

/******************************************************************************
 * Function Name: mqtt_client_task
 ******************************************************************************
 * Summary:
 *  Task for handling initialization & connection of Wi-Fi and the MQTT client.
 *  The task also creates and manages the subscriber and publisher tasks upon 
 *  successful MQTT connection. The task also handles the WiFi and MQTT 
 *  connections by initiating reconnection on the event of disconnections.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_client_task(void *pvParameters)
{
    /* Structures that store the data to be sent/received to/from various
     * message queues.
     */
    mqtt_task_cmd_t mqtt_status;
    subscriber_data_t subscriber_q_data;
    publisher_data_t publisher_q_data;
    bool mqtt_client_status = false;

    app_sdio_init();

    /* Configure the Wi-Fi interface as a Wi-Fi STA (i.e. Client). */
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wcm_config.wifi_interface_instance = &sdio_instance;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Create a message queue to communicate with other tasks and callbacks. */
    mqtt_task_q = xQueueCreate(MQTT_TASK_QUEUE_LENGTH, sizeof(mqtt_task_cmd_t));

    if (mqtt_control_events == NULL)
    {
        mqtt_control_events = xEventGroupCreate();
        if (mqtt_control_events == NULL)
        {
            #if TESAIOT_DEBUG_ERROR_ENABLED
            printf("%s Failed to create MQTT control events.\n", LABEL_ERROR);
            #endif /* TESAIOT_DEBUG_ERROR_ENABLED */
            cleanup();
            vTaskDelete(NULL);
        }
    }

    /* Initialize the Wi-Fi Connection Manager and jump to the cleanup block
     * upon failure.
     */
    if (CY_RSLT_SUCCESS != cy_wcm_init(&wcm_config))
    {
        CY_ASSERT(0);
    }

    /* Set the appropriate bit in the status_flag to denote successful 
     * WCM initialization.
     */
    status_flag |= WCM_INITIALIZED;
    printf("\nWi-Fi Connection Manager initialized.\n");

    /* Initiate connection to the Wi-Fi AP and cleanup if the operation fails. */
    if (CY_RSLT_SUCCESS == wifi_connect())
    {
mqtt_wait_for_start:
        /* Wait for MQTT start request (can return here after lightweight mode cleanup) */
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("\n%s Waiting for MQTT start request from menu...\n", LABEL_INFO);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */
        (void)xEventGroupWaitBits(mqtt_control_events,
                                  MQTT_CTRL_START_BIT,
                                  pdTRUE,
                                  pdFALSE,
                                  portMAX_DELAY);
        mqtt_started = true;
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("%s MQTT start request received. Initialising broker session.\n", LABEL_INFO);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */

        /* NOTE: NTP time synchronization is now performed in main.c immediately after WiFi connection */
        /* This ensures system time is valid before any certificate validation or MQTT operations */

        /* Set-up the MQTT client and connect to the MQTT broker. */
        if ( (CY_RSLT_SUCCESS == mqtt_init()) && (CY_RSLT_SUCCESS == mqtt_connect()) )
        {
            /* Lightweight mode: skip subscriber/publisher task creation */
            if (g_mqtt_lightweight_mode)
            {
                mqtt_client_status = true;
                printf("[MQTT-Lightweight] Connected! No subscriber/publisher tasks created.\n");
                fflush(stdout);

                /* Signal to caller that connection is ready */
                mqtt_started = true;

                /* Wait for stop signal in lightweight mode */
                while (g_mqtt_lightweight_mode && !mqtt_stop_requested)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                /* Cleanup when stop requested */
                if (mqtt_stop_requested)
                {
                    printf("[MQTT-Lightweight] Stop requested, cleaning up...\n");
                    cleanup_mqtt_only();
                    printf("[MQTT-Lightweight] Stopped successfully. Wi-Fi remains connected.\n");
                    mqtt_stop_requested = false;
                    g_mqtt_lightweight_mode = false;
                    mqtt_client_status = false;

                    /* Clear PSA key slots to ensure fresh state for next connection */
                    optiga_psa_clear_all_slots();

                    /* Allow TLS stack to fully release resources before next connection */
                    vTaskDelay(pdMS_TO_TICKS(1000));

                    /* Return to wait for next MQTT start request */
                    goto mqtt_wait_for_start;
                }
            }
            else
            {
                /* Normal mode: create subscriber and publisher tasks */
                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("[MQTT-Task] Connection done, creating subscriber (stack=%u words)...\n",
                       (unsigned)SUBSCRIBER_TASK_STACK_SIZE);
                fflush(stdout);
                printf("[MQTT-Task] >>> CALLING xTaskCreate NOW <<<\n");
                fflush(stdout);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                /* Create the subscriber task and cleanup if the operation fails. */
                BaseType_t sub_result = xTaskCreate(subscriber_task, "Subscriber task", SUBSCRIBER_TASK_STACK_SIZE,
                                          NULL, SUBSCRIBER_TASK_PRIORITY, &subscriber_task_handle);

                #if TESAIOT_DEBUG_VERBOSE_ENABLED
                printf("[MQTT-Task] <<< xTaskCreate RETURNED >>>\n");
                fflush(stdout);
                printf("[MQTT-Task] xTaskCreate returned %d (pdPASS=%d)\n", (int)sub_result, (int)pdPASS);
                fflush(stdout);
                #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                if (pdPASS == sub_result)
                {
                    #if TESAIOT_DEBUG_VERBOSE_ENABLED
                    printf("[MQTT-Task] Subscriber OK! Waiting for subscribe to complete...\n");
                    #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                    /* Wait for the subscribe operation to complete. */
                    vTaskDelay(pdMS_TO_TICKS(TASK_CREATION_DELAY_MS));

                    /* CRITICAL: Wait for Protected Update to complete before creating Publisher task
                     * Protected Update uses OPTIGA async callbacks that require Timer Task priority.
                     * Creating Publisher task during Protected Update causes callbacks to hang. */
                    if (g_protected_update_active) {
                        #if TESAIOT_DEBUG_VERBOSE_ENABLED
                        printf("[MQTT-Task] Protected Update in progress - waiting before creating Publisher...\n");
                        fflush(stdout);
                        #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                        /* Wait up to 30 seconds for Protected Update to complete */
                        uint32_t wait_count = 0;
                        while (g_protected_update_active && wait_count < 300) {
                            vTaskDelay(pdMS_TO_TICKS(100));
                            wait_count++;
                        }

                        if (g_protected_update_active) {
                            #if TESAIOT_DEBUG_ERROR_ENABLED
                            printf("[MQTT-Task] WARNING: Protected Update still active after 30s, proceeding anyway\n");
                            fflush(stdout);
                            #endif /* TESAIOT_DEBUG_ERROR_ENABLED */
                        } else {
                            #if TESAIOT_DEBUG_VERBOSE_ENABLED
                            printf("[MQTT-Task] Protected Update completed\n");
                            fflush(stdout);
                            #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
                        }
                    }

                    /* CRITICAL: Skip Publisher task creation after Protected Update completed
                     * Protected Update workflow will call mqtt_request_stop() to clean up
                     * Creating Publisher task would cause race condition with workflow cleanup */
                    if (g_protected_update_just_completed) {
                        #if TESAIOT_DEBUG_VERBOSE_ENABLED
                        printf("[MQTT-Task] Protected Update just completed - skipping Publisher task\n");
                        printf("[MQTT-Task] Workflow will call mqtt_request_stop() for cleanup\n");
                        fflush(stdout);
                        #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
                        mqtt_client_status = true;  // Mark MQTT as ready (subscriber is running)
                    }
                    else
                    {
                        #if TESAIOT_DEBUG_VERBOSE_ENABLED
                        /* Check available heap before creating task */
                        size_t free_heap = xPortGetFreeHeapSize();
                        size_t min_heap = xPortGetMinimumEverFreeHeapSize();
                        size_t required_bytes = PUBLISHER_TASK_STACK_SIZE * sizeof(StackType_t);

                        printf("[MQTT-Task] Heap before Publisher creation:\n");
                        printf("              Free: %u bytes, Min-ever: %u bytes\n",
                               (unsigned)free_heap, (unsigned)min_heap);
                        printf("              Required: %u bytes (stack=%u words)\n",
                               (unsigned)required_bytes, (unsigned)PUBLISHER_TASK_STACK_SIZE);
                        fflush(stdout);

                        if (free_heap < required_bytes + 1024) {
                            printf("[MQTT-Task] WARNING: Low heap! May fail to create task.\n");
                            fflush(stdout);
                        }
                        #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */

                        printf("[MQTT-Task] Calling xTaskCreate NOW...\n");
                        fflush(stdout);

                        BaseType_t pub_result = xTaskCreate(publisher_task, "Publisher task",
                                                            PUBLISHER_TASK_STACK_SIZE,
                                                            NULL, PUBLISHER_TASK_PRIORITY,
                                                            &publisher_task_handle);

                        printf("[MQTT-Task] xTaskCreate returned: %d (pdPASS=%d)\n",
                               (int)pub_result, (int)pdPASS);
                        fflush(stdout);

                        if (pdPASS == pub_result)
                        {
                            mqtt_client_status = true;
                            #if TESAIOT_DEBUG_VERBOSE_ENABLED
                            printf("[MQTT-Task] Publisher OK! MQTT ready.\n");
                            #endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
                        }
                        else
                        {
                            #if TESAIOT_DEBUG_ERROR_ENABLED
                            printf("[MQTT-Task] ERROR: Publisher task creation FAILED\n");
                            #endif /* TESAIOT_DEBUG_ERROR_ENABLED */
                        }
                    }
                }
                else
                {
                    #if TESAIOT_DEBUG_ERROR_ENABLED
                    printf("[MQTT-Task] ERROR: Subscriber task creation FAILED\n");
                    #endif /* TESAIOT_DEBUG_ERROR_ENABLED */
                }
            }
        }
    }

    if(mqtt_client_status)
    {
        mqtt_client_status = false;
        while (true)
        {
            /* Wait for results of MQTT operations from other tasks and callbacks. */
            if (pdTRUE == xQueueReceive(mqtt_task_q, &mqtt_status, portMAX_DELAY))
            {
                /* In this code example, the disconnection from the MQTT Broker or
                 * the Wi-Fi network is handled by the case 'HANDLE_DISCONNECTION'.
                 *
                 * The publish and subscribe failures (`HANDLE_MQTT_PUBLISH_FAILURE`
                 * and `HANDLE_MQTT_SUBSCRIBE_FAILURE`) does not initiate
                 * reconnection in this example, but they can be handled as per the
                 * application requirement in the following swich cases.
                 */
                switch(mqtt_status)
                {
                    case HANDLE_DISCONNECTION:
                    {
                        /* Deinit the publisher before initiating reconnections. */
                        memset(&publisher_q_data, 0, sizeof(publisher_q_data));  // Initialize to zero
                        publisher_q_data.cmd = PUBLISHER_DEINIT;
                        xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

                        /* Although the connection with the MQTT Broker is lost,
                         * call the MQTT disconnect API for cleanup of threads and
                         * other resources before reconnection.
                         */
                        cy_mqtt_disconnect(mqtt_connection);

                        /* Check if Wi-Fi connection is active. If not, update the
                         * status flag and initiate Wi-Fi reconnection.
                         */
                        if (cy_wcm_is_connected_to_ap() == 0)
                        {
                            status_flag &= ~(WIFI_CONNECTED);
                            printf("\nInitiating Wi-Fi Reconnection...\n");
                            if (CY_RSLT_SUCCESS != wifi_connect())
                            {
                                /* Wi-Fi reconnect failed - terminate */
                                break;
                            }
                        }

                        /* Wi-Fi is connected (either was connected or just reconnected)
                         * Now reconnect MQTT. This fixes the bug where MQTT SND_RCV_FAIL
                         * would cause full termination even though Wi-Fi was still up.
                         */
                        printf("\nInitiating MQTT Reconnection...\n");

                        /* Re-stage certificate for mTLS reconnection */
#if MQTT_ENABLE_MUTUAL_AUTH
                        uint16_t selected_cert_oid = tesaiot_select_mqtt_certificate();
                        bool cert_staged = false;

                        if (selected_cert_oid == 0xE0E1) {
                            /* Device Certificate selected (preferred) - use corresponding key 0xE0F1 */
                            optiga_psa_set_signing_key_oid(OPTIGA_KEY_ID_E0F1);
                            cert_staged = trustm_use_device_certificate();
                            #if TESAIOT_DEBUG_VERBOSE_ENABLED
                            printf("%s Reconnect: Using Device Certificate (OID 0xE0E1)\n", LABEL_MQTT);
                            #endif
                        } else if (selected_cert_oid == 0xE0E0) {
                            /* Factory Certificate selected (fallback mode) - use corresponding key 0xE0F0 */
                            optiga_psa_set_signing_key_oid(OPTIGA_KEY_ID_E0F0);
                            cert_staged = trustm_use_factory_certificate();
                            #if TESAIOT_DEBUG_VERBOSE_ENABLED
                            printf("%s Reconnect: Using Factory Certificate (OID 0xE0E0)\n", LABEL_MQTT);
                            #endif
                        } else {
                            #if TESAIOT_DEBUG_ERROR_ENABLED
                            printf("%s ERROR: Invalid certificate OID during reconnect: 0x%04X\n",
                                   LABEL_MQTT, selected_cert_oid);
                            #endif
                        }

                        if (!cert_staged) {
                            #if TESAIOT_DEBUG_ERROR_ENABLED
                            printf("%s ERROR: Failed to stage certificate for TLS reconnect\n", LABEL_MQTT);
                            #endif
                            break;  /* Cannot reconnect without valid certificate */
                        }
#endif /* MQTT_ENABLE_MUTUAL_AUTH */

                        if (CY_RSLT_SUCCESS == mqtt_connect())
                        {
                            /* Initiate MQTT subscribe post the reconnection. */
                            memset(&subscriber_q_data, 0, sizeof(subscriber_q_data));  // Initialize to zero
                            subscriber_q_data.cmd = SUBSCRIBE_TO_TOPIC;
                            xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);

                            /* Initialize Publisher post the reconnection. */
                            memset(&publisher_q_data, 0, sizeof(publisher_q_data));  // Initialize to zero
                            publisher_q_data.cmd = PUBLISHER_INIT;
                            xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);
                            mqtt_client_status = true;
                        }
                        break;
                    }

                    default:
                        break;
                }
            }
            if(!mqtt_client_status){
                terminate_tasks();
                break;
            }
        }
    }else
    {
        terminate_tasks();
    }
}

#if GENERATE_UNIQUE_CLIENT_ID
/******************************************************************************
 * Function Name: mqtt_get_unique_client_identifier
 ******************************************************************************
 * Summary:
 *  Function that generates unique client identifier for the MQTT client by
 *  appending a timestamp to a common prefix 'MQTT_CLIENT_IDENTIFIER'.
 *
 * Parameters:
 *  char *mqtt_client_identifier : Pointer to the string that stores the 
 *                                 generated unique identifier
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on successful generation of the client 
 *              identifier, else a non-zero value indicating failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;

    /* Check for errors from snprintf. */
    if (0 > snprintf(mqtt_client_identifier,
                     (MQTT_CLIENT_IDENTIFIER_MAX_LEN + 1),
                     MQTT_CLIENT_IDENTIFIER "%lu",
                     (long unsigned int)Clock_GetTimeMs()))
    {
        status = ~CY_RSLT_SUCCESS;
    }

    return status;
}
#endif /* GENERATE_UNIQUE_CLIENT_ID */

/* [] END OF FILE */
