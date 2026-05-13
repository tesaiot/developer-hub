/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for PSE84 Trust M End-to-End Trust Management
*              via TESAIoT Platform using MQTT with Mutual TLS authentication.
*
* Related Document: See README.md
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

/*******************************************************************************
* TESAIoT Platform Integration
*******************************************************************************
* SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This application has been modified by the TESAIoT Platform Developer Team to add:
* - Integration with TESAIoT AIoT Foundation Platform
* - Certificate Lifecycle Management (Protected Update workflow)
* - Protected Update workflow support (manifest + fragments, v3.0.0)
* - OPTIGA Trust M hardware security integration
* - Interactive menu for provisioning and testing
* Security by Design: IoT Cybersecurity compliance (ETSI EN 303 645, NIST IR 8259A, IEC 62443, IEEE 802.1AR)
* Original work Copyright 2024-2025 Cypress Semiconductor Corporation (Infineon)
* Modifications Copyright 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This application is part of the TESAIoT AIoT Foundation Platform, developed in
* collaboration with Infineon Technologies AG for PSoC Edge E84 + OPTIGA Trust M.
*
* Contact: Wiroon Sriborrirux <sriborrirux@gmail.com>
*******************************************************************************/

/* Header file includes */
#include <stdio.h>
#include "cybsp.h"
#include "retarget_io_init.h"
#include "mqtt_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "cy_time.h"
#include "cycfg_peripherals.h"
#include "cy_scb_uart.h"         /* For Cy_SCB_UART_GetNumInRxFifo() - FreeRTOS-friendly getchar */
#include "cy_tls_optiga_key.h"
#include "optiga_psa_se.h"
#include "cy_wcm.h"
#include "mqtt_client_config.h"
#include "../cryptography/cryptography_examples.h"
#include "subscriber_task.h"
#include "publisher_task.h"
#include "semphr.h"
#include "event_groups.h"

/*----------------------------------------------------------------------------
 * TESAIoT Library v3.0.0 - Umbrella Headers (5 Domain Groups)
 *
 * Instead of including 12+ individual headers, use these 6 umbrella headers:
 *---------------------------------------------------------------------------*/
#include "tesaiot.h"                   // Group 6: License Management
#include "tesaiot_config.h"            // Configuration (debug, OID settings)
#include "tesaiot_optiga_core.h"       // Group 1: Core OPTIGA instance
#include "tesaiot_protected_update.h"  // Group 2: Protected Update Workflow
#include "tesaiot_optiga.h"            // Group 3: TESAIoT OPTIGA Trust M
#include "tesaiot_platform.h"          // Group 4: Standard Libs (MQTT, SNTP)

/******************************************************************************
 * Macros
 ******************************************************************************/
/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_US            (10U)
/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles
 * to come into effect. This wait time value will depend on the actual CLK_LF
 * frequency set by the BSP.
 */
#define LPTIMER_0_WAIT_TIME_USEC           (62U)
/* Define the LPTimer interrupt priority number. '1' implies highest priority.
 */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)


#ifndef PSA_KEY_PERSISTANCE_VOLATILE
#define PSA_KEY_PERSITANCE_VOLATILE ((psa_key_persistance_t)0)
#endif
#ifndef PSA_KEY_LOCATION_OPTIGA
#define PSA_KEY_LOCATION_OPTIGA ((psa_key_location_t) 1)
#endif


/*******************************************************************************
 * Global Variables
 ******************************************************************************/
/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;
typedef mtb_hal_rtc_t rtc_type;
extern EventGroupHandle_t data_received_event_group;


/*****************************************************************************
 * Function Definitions
 *****************************************************************************/
extern int cy_tls_set_client_cert(const uint8_t *buf, size_t len, int is_pem);
/* Extract the certificate and use it for the actual communication */
extern void use_optiga_certificate(void);

static void print_menu_prompt(void)
{
    bool wifi_connected = (cy_wcm_is_connected_to_ap() != 0);
    bool mqtt_started_flag = mqtt_is_started();
    bool mqtt_connected_flag = mqtt_is_connected();
    const char *mqtt_status = mqtt_connected_flag ? "connected" : (mqtt_started_flag ? "connecting" : "idle");
    const char *wifi_status = wifi_connected ? "connected" : "connecting";

    printf("\n================ PSoC Edge-to-TESAIoT Platform Menu ================\n");
    #if TESAIOT_DEBUG_INFO_ENABLED
    printf("%s PSE84 Trust M + TESAIoT Firmware v3.0.0 Production\n", LABEL_VERSION);
    printf("%s Wi-Fi: %s | MQTT: %s\n", LABEL_STATUS, wifi_status, mqtt_status);
    /* Display TESAIoT License Status */
    if (tesaiot_is_licensed()) {
        uint8_t uid[27];
        uint16_t uid_len = sizeof(uid);
        printf("[LICENSE] Valid - UID: ");
        if (tesaiot_get_device_uid(uid, &uid_len)) {
            for (int i = 0; i < uid_len; i++) printf("%02X", uid[i]);
        }
        printf("\n");
    } else {
        printf("[LICENSE] INVALID - TESAIoT functions disabled!\n");
    }
    #endif /* TESAIOT_DEBUG_INFO_ENABLED */

    // Display current time from NTP sync to verify RTC/NTP working
    // Note: System time is UTC for certificate validation
    // Display uses local time (UTC+7 for Thailand) for user convenience
    time_t current_time;
    if (tesaiot_sntp_get_time(&current_time)) {
        char time_str[64];
        tesaiot_sntp_format_local_time(current_time, time_str, sizeof(time_str));
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("%s %s (NTP synced)\n", LABEL_TIME, time_str);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    } else {
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("%s Not synced yet (waiting for NTP sync after WiFi connect)\n", LABEL_TIME);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    }

    printf("1) Print factory UID and factory certificate\n");
    printf("2) Test MQTT connection with current certificate\n");
    printf("3) Full Protected Update workflow with TESAIoT\n");
    printf("4) Test OPTIGA Trust M metadata operations (diagnostics)\n");
#if TESAIOT_DEBUG_ENABLED  /* Menu 6 hidden in production - use for debug only */
	printf("........................................................\n");
    printf("6) Test Protected Update (Isolated - No TESAIoT Platform)\n");
    printf("Select option (1-6) then press Enter: ");
#else
    printf("Select option (1-4) then press Enter: ");
#endif
    fflush(stdout);
}

static void print_factory_identity(void)
{
    char factory_uid[65] = {0};
    if (tesaiot_read_factory_uid(factory_uid, sizeof(factory_uid)))
    {
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("\n%s %s\n", LABEL_FACTORY_UID, factory_uid);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    }
    else
    {
        #if TESAIOT_DEBUG_WARNING_ENABLED
        printf("\n%s Failed to read OPTIGA factory UID\n", LABEL_WARN);
        #endif /* TESAIOT_DEBUG_WARNING_ENABLED */
    }

    char factory_cert_pem[1200];
    uint16_t factory_cert_len = (uint16_t)sizeof(factory_cert_pem);
    if (tesaiot_read_factory_certificate(factory_cert_pem, &factory_cert_len))
    {
        #if TESAIOT_DEBUG_INFO_ENABLED
        printf("%s\n%s\n", LABEL_FACTORY_CERT, factory_cert_pem);
        #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    }
    else
    {
        #if TESAIOT_DEBUG_WARNING_ENABLED
        printf("%s Failed to read factory device certificate\n", LABEL_WARN);
        #endif /* TESAIOT_DEBUG_WARNING_ENABLED */
    }
}

/*******************************************************************************
 * MQTT Connection Test Feature
 *
 * Enable/Disable: Comment or uncomment the line below
 * When enabled: Test will publish a message after successful connection
 * When disabled: Only tests connection (original behavior)
 ******************************************************************************/
#define MQTT_LIGHTWEIGHT_PUBLISH_TEST  /* Lightweight mode - direct publish only */

#ifdef MQTT_LIGHTWEIGHT_PUBLISH_TEST
#include "cy_mqtt_api.h"  /* For cy_mqtt_publish direct call */

/**
 * @brief Lightweight MQTT Publish Test
 *
 * Uses lightweight MQTT mode (no subscriber/publisher tasks):
 * 1. Connect to broker with lightweight mode
 * 2. Publish test message to telemetry topic
 * 3. Disconnect
 *
 * No subscribe - avoids MQTT event thread conflict with publish.
 */
static void test_mqtt_lightweight_publish(void)
{
    printf("\n[MQTT-Test] Starting Lightweight Publish Test...\n");

    /* Check Wi-Fi connection */
    if (cy_wcm_is_connected_to_ap() == 0)
    {
        printf("[MQTT-Test] ERROR: Wi-Fi not connected\n");
        return;
    }
    printf("[MQTT-Test] Wi-Fi connected\n");

    /* Request lightweight MQTT connection */
    printf("[MQTT-Test] Requesting lightweight MQTT connection...\n");
    if (!mqtt_request_start_lightweight())
    {
        printf("[MQTT-Test] ERROR: Failed to request lightweight MQTT\n");
        return;
    }

    /* Wait for connection (max 30 seconds) */
    printf("[MQTT-Test] Waiting for broker connection...\n");
    int timeout_ms = 30000;
    while (!mqtt_is_connected() && timeout_ms > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        timeout_ms -= 500;
    }

    if (!mqtt_is_connected())
    {
        printf("[MQTT-Test] ERROR: Connection timeout\n");
        mqtt_stop_lightweight();
        return;
    }
    printf("[MQTT-Test] Connected to broker!\n");

    /* Build test payload */
    char payload[256];
    int payload_len = snprintf(payload, sizeof(payload),
        "{"
        "\"type\":\"mqtt_test\","
        "\"device_id\":\"%s\","
        "\"trustm_uid\":\"%s\","
        "\"test\":\"connection\","
        "\"mode\":\"lightweight\""
        "}",
        DEVICE_ID, FACTORY_UID);

    /* Build topic string */
    const char *topic = MQTT_PUB_TOPIC_SENSOR;

    printf("[MQTT-Test] Publishing %d bytes to '%s'...\n", payload_len, topic);

    /* Use QoS 0 (fire-and-forget) to avoid waiting for PUBACK */
    cy_mqtt_publish_info_t pub_info = {
        .qos = CY_MQTT_QOS0,
        .topic = topic,
        .topic_len = strlen(topic),
        .payload = payload,
        .payload_len = (size_t)payload_len,
        .retain = false,
        .dup = false
    };

    cy_rslt_t result = cy_mqtt_publish(mqtt_connection, &pub_info);

    if (result == CY_RSLT_SUCCESS)
    {
        /* Small delay to let TCP send the packet */
        vTaskDelay(pdMS_TO_TICKS(500));

        printf("\n[MQTT-Test] ========== PUBLISH SUCCESS ==========\n");
        printf("[MQTT-Test] Topic: %s\n", topic);
        printf("[MQTT-Test] Payload: %s\n", payload);
        printf("[MQTT-Test] [x] PUBLISH: OK\n");
        printf("[MQTT-Test] =====================================\n\n");
    }
    else
    {
        printf("\n[MQTT-Test] ========== PUBLISH FAILED ==========\n");
        printf("[MQTT-Test] Error: 0x%08X\n", (unsigned int)result);
        printf("[MQTT-Test] [ ] PUBLISH: FAILED\n");
        printf("[MQTT-Test] ====================================\n\n");
    }

    /* Stop lightweight MQTT */
    printf("[MQTT-Test] Stopping lightweight MQTT...\n");
    mqtt_stop_lightweight();

    printf("[MQTT-Test] Test complete!\n");
}
#endif /* MQTT_LIGHTWEIGHT_PUBLISH_TEST */

/* Test MQTT connection with current certificate */
static void test_mqtt_connection(void)
{
#ifdef MQTT_LIGHTWEIGHT_PUBLISH_TEST
    /* Use lightweight publish test (no subscriber/publisher tasks) */
    test_mqtt_lightweight_publish();
#else
    /* Original full MQTT test with subscriber/publisher tasks */
    #if TESAIOT_DEBUG_VERBOSE_ENABLED
    printf("\n%s Starting MQTT connectivity test...\n", MENU_MQTT_TEST);
    #endif

    if (cy_wcm_is_connected_to_ap() == 0)
    {
        printf("%s ERROR: Wi-Fi not connected.\n", MENU_MQTT_TEST);
        return;
    }
    printf("%s --> Wi-Fi connected\n", MENU_MQTT_TEST);

    printf("%s Requesting MQTT connection...\n", MENU_MQTT_TEST);
    if (!mqtt_request_start())
    {
        printf("%s ERROR: MQTT subsystem not ready\n", MENU_MQTT_TEST);
        return;
    }

    printf("%s Waiting for MQTT broker connection (timeout: 30s)...\n", MENU_MQTT_TEST);
    const uint32_t max_attempts = 60;
    for (uint32_t attempt = 0; attempt < max_attempts; ++attempt)
    {
        if (mqtt_is_connected())
        {
            printf("\n%s ========== CONNECTION SUCCESS ==========\n", MENU_MQTT_TEST);
            printf("%s Broker: %s\n", MENU_MQTT_TEST, MQTT_BROKER_ADDRESS);
            printf("%s ========== MQTT TEST COMPLETE ==========\n\n", MENU_MQTT_TEST);
            return;
        }

        if ((attempt % 10) == 0 && attempt > 0)
        {
            printf("%s Still connecting... (%lu seconds elapsed)\n", MENU_MQTT_TEST, (attempt / 2));
        }

        vTaskDelay(pdMS_TO_TICKS(500U));
    }

    printf("\n%s TIMEOUT: Failed to connect within 30 seconds\n", MENU_MQTT_TEST);
    printf("  1. Certificate mismatch (factory cert may not be trusted by broker)\n");
    printf("  2. Network connectivity issues\n");
    printf("  3. Broker configuration issues\n");
    printf("%s Recommendation: Check certificate configuration or run Protected Update\n\n", MENU_MQTT_TEST);
#endif /* MQTT_LIGHTWEIGHT_PUBLISH_TEST */
}

static void wait_for_wifi_connection(void)
{
    const TickType_t poll_interval = pdMS_TO_TICKS(500U);
    uint32_t elapsed_ms = 0U;

    while (cy_wcm_is_connected_to_ap() == 0)
    {
        vTaskDelay(poll_interval);
        elapsed_ms += 500U;
        if ((elapsed_ms % 5000U) == 0U)
        {
            #if TESAIOT_DEBUG_INFO_ENABLED
            printf("%s Waiting for Wi-Fi connection... (%lus)\n", LABEL_INFO, (unsigned long)(elapsed_ms / 1000U));
            #endif /* TESAIOT_DEBUG_INFO_ENABLED */
        }
    }

    #if TESAIOT_DEBUG_INFO_ENABLED
    printf("%s Wi-Fi connection established.\n", LABEL_INFO);
    #endif /* TESAIOT_DEBUG_INFO_ENABLED */
}

static void drain_line(void)
{
    int ch;
    do
    {
        /* Wait for UART data with FreeRTOS-friendly delay */
        while (Cy_SCB_UART_GetNumInRxFifo(CYBSP_DEBUG_UART_HW) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        /* Read directly from UART FIFO */
        ch = (int)Cy_SCB_UART_Get(CYBSP_DEBUG_UART_HW);
    } while ((ch != '\n') && (ch != '\r') && (ch != EOF));
}

/**
 * FreeRTOS-friendly getchar that yields CPU while waiting for input.
 * Standard getchar() uses busy-wait polling which can starve other tasks
 * and prevent UART RX from working properly after MQTT connection.
 */
static int rtos_getchar(void)
{
    /* Poll for UART data with FreeRTOS-friendly delay */
    while (Cy_SCB_UART_GetNumInRxFifo(CYBSP_DEBUG_UART_HW) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    /* Read directly from UART FIFO - bypass retarget_io to avoid blocking after MQTT */
    return (int)Cy_SCB_UART_Get(CYBSP_DEBUG_UART_HW);
}

/******************************************************************************
 * Function Name: optiga_client_task
 ******************************************************************************
 * Summary:
 *  A task to properly initialize the chip and use the certificate installed
 *  to the OPTIGA Secure Element. After this start the MQTT task
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void optiga_client_task(void *pvParameters) {
	CY_UNUSED_PARAMETER(pvParameters);
	char optiga_cert_pem[1024];
	uint16_t optiga_cert_pem_size = 1024;

	printf("\x1b[2J\x1b[;H");
	optiga_trust_init();

	/* Initialize persistent OPTIGA instance manager (must be after optiga_trust_init)
	 * This creates a singleton instance that will be reused across all workflows
	 * to avoid instance state corruption and critical section deadlocks.
	 */
	if (!optiga_manager_init(optiga_util_callback, NULL)) {
		printf("ERROR: Failed to initialize OPTIGA manager! Cannot proceed.\n");
		CY_ASSERT(0);  // Fatal error - cannot proceed without OPTIGA
	}
	printf("OPTIGA persistent instance manager initialized successfully\n");

	/* TESAIoT License Check - MUST be after OPTIGA init since it reads device UID */
	printf("Verifying TESAIoT Library License...\n");
	tesaiot_license_status_t lic_status = tesaiot_license_init();
	if (lic_status != TESAIOT_LICENSE_OK) {
		printf("\n");
		printf("!!! LICENSE ERROR: %s !!!\n", tesaiot_license_status_str(lic_status));
		printf("!!! TESAIoT functions will not work !!!\n");
		printf("\n");
	} else {
		printf("License verified - TESAIoT Library ready\n\n");
	}

    /* This is the place where the certificate is initialized. This is a function
	 * which will allow to read it out and populate internal mbedtls settings wit it*/
	 read_certificate_from_optiga(0xe0e0, optiga_cert_pem,
	 		&optiga_cert_pem_size);
	// printf("Your certificate is:\n%s\n", optiga_cert_pem);

	/* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
	printf("====================================================================\n");
	printf("PSoC Edge - TESAIoT Example: MQTT Client using mTLS + OPTIGA Trust M\n");
	printf("====================================================================\n\n");

	/* Initialize the PSA Crypto subsystem to allow TLS handshake using
	 * the OPTIGA Trust M private key and certificate.
	 */
	psa_status_t r = optiga_psa_register();
	r = psa_crypto_init();
	if (r != PSA_SUCCESS)
		printf("PSA Init fail %d \n\r", (int)r);

	/* Set the OPTIGA key ID for the TLS handshake
	 * This is the key created during manufacturing and used to sign the
	 * factory certificate.
	 */
	psa_key_id_t optiga_key = 0;
	psa_key_attributes_t att = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&att, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&att, 256);
	psa_set_key_usage_flags(&att, PSA_KEY_USAGE_SIGN_HASH );
	psa_set_key_algorithm(&att, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

	psa_set_key_lifetime(&att, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(PSA_KEY_PERSISTENCE_VOLATILE, PSA_KEY_LOCATION_OPTIGA));

	r = psa_generate_key(&att, &optiga_key);
	if ( r != PSA_SUCCESS)
	{
		printf("PSA Generate key Fail %d\n\r", (int)r );
	}

	/* Configure the TLS layer to use the OPTIGA key and certificate */
	cy_tls_set_optiga_key_id(optiga_key);

	/* Set the client certificate for the TLS handshake.
	 * This is either the factory certificate, or a new certificate installed
	 * via Protected Update workflow (v3.0.0).
	 */
	cy_tls_set_client_cert((const uint8_t *)optiga_cert_pem, optiga_cert_pem_size, 1);

	/* Create the MQTT Client task to bring up Wi-Fi + MQTT stack. */
	xTaskCreate(mqtt_client_task, "MQTT Client task",
			MQTT_CLIENT_TASK_STACK_SIZE,
			NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);

	/* Give the connectivity stack time to start and then wait for Wi-Fi link. */
	vTaskDelay(pdMS_TO_TICKS(1000U));
	wait_for_wifi_connection();

	/* Synchronize system time with NTP server immediately after WiFi connection */
	/* This enables accurate certificate date validation and time display in menu */
	#if TESAIOT_DEBUG_INFO_ENABLED
	printf("\n%s Synchronizing time with NTP server...\n", LABEL_NTP);
	#endif /* TESAIOT_DEBUG_INFO_ENABLED */
	tesaiot_sntp_result_t ntp_result;
	if (tesaiot_sntp_sync_time(NULL, &ntp_result))  // NULL = use default pool.ntp.org
	{
		#if TESAIOT_DEBUG_INFO_ENABLED
		printf("%s Time synchronized successfully!\n", LABEL_NTP);
		printf("%s Server: %s (stratum %u)\n", LABEL_NTP, ntp_result.server, ntp_result.stratum);
		printf("%s Current time: %s\n", LABEL_NTP, ctime(&ntp_result.unix_time));
		#endif /* TESAIOT_DEBUG_INFO_ENABLED */
	}
	else
	{
		#if TESAIOT_DEBUG_INFO_ENABLED
		printf("%s WARNING: NTP sync failed - certificate validation may fail!\n", LABEL_NTP);
		printf("%s Proceeding with potentially invalid system time...\n", LABEL_NTP);
		#endif /* TESAIOT_DEBUG_INFO_ENABLED */
	}

	#if TESAIOT_DEBUG_INFO_ENABLED
	printf("%s Interactive provisioning menu ready.\n", LABEL_INFO);
	#endif /* TESAIOT_DEBUG_INFO_ENABLED */

	for (;;)
	{
		print_menu_prompt();
		/* Use FreeRTOS-friendly getchar to prevent blocking after MQTT connection */
		int selection = rtos_getchar();
		if ((selection == '\n') || (selection == '\r'))
		{
			continue;
		}
		drain_line();
		switch (selection)
		{
		case '1':
			print_factory_identity();
			break;
		case '2':
			test_mqtt_connection();
			break;
		case '3':
			#if TESAIOT_DEBUG_VERBOSE_ENABLED
			printf("\n%s Starting Protected Update workflow with TESAIoT...\n", LABEL_MENU);
			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			tesaiot_run_protected_update_workflow();
			printf("\n%s Protected Update workflow returned. Returning to menu...\n", LABEL_MENU);
			fflush(stdout);
			break;
		case '4':
			#if TESAIOT_DEBUG_VERBOSE_ENABLED
			printf(LABEL_METADATA_TEST " Starting OPTIGA Trust M metadata diagnostic tests...\n");
			printf(LABEL_METADATA_TEST " This will help diagnose why certificate write fails.\n");
			fflush(stdout);
			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			tesaiot_test_metadata_operations();
			break;
#if TESAIOT_DEBUG_ENABLED  /* Menu 6 hidden in production */
		case '6':
			#if TESAIOT_DEBUG_VERBOSE_ENABLED
			printf("\n" MENU_TEST_PROTUPD " Starting Protected Update Isolated Test...\n");
			#endif /* TESAIOT_DEBUG_VERBOSE_ENABLED */
			tesaiot_run_protected_update_isolated_test();
			break;
#endif
		default:
			#if TESAIOT_DEBUG_WARNING_ENABLED
#if TESAIOT_DEBUG_ENABLED
			printf("%s Invalid selection '%c'. Please choose 1-6.\n", LABEL_WARN, selection);
#else
			printf("%s Invalid selection '%c'. Please choose 1-4.\n", LABEL_WARN, selection);
#endif
			#endif /* TESAIOT_DEBUG_WARNING_ENABLED */
			break;
		}
	}
}

#ifdef ENABLE_SECURE_SOCKETS_LOGS
 int app_log_output_callback(CY_LOG_FACILITY_T facility, CY_LOG_LEVEL_T level, char *logmsg)
 {
     (void)facility;     // Can be used to decide to reduce output or send output to remote logging
     (void)level;        // Can be used to decide to reduce output, although the output has already been
                         // limited by the log routines

     return printf( "%s\n", logmsg);   // print directly to console
 }

/*
  Log time callback - get the current time for the log message timestamp in millseconds
 */
 cy_rslt_t app_log_time(uint32_t* time)
 {
     if (time != NULL)
     {
         *time =  xTaskGetTickCount(); // get system time (in milliseconds)
     }
     return CY_RSLT_SUCCESS;
 }
#endif

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
* 1. This function first configures and initializes an interrupt for LPTimer.
* 2. Then it initializes the LPTimer HAL object to be used in the RTOS
*    tickless idle mode implementation to allow the device enter deep sleep
*    when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
* 3. It then passes the LPTimer object to abstraction RTOS library that
*    implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status =
                                    Cy_SysInt_Init(&lptimer_intr_cfg,
                                                    lptimer_interrupt_handler);

    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status =
                                    Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW,
                                                &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }

    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk,
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
                                            &CYBSP_CM33_LPTIMER_0_hal_config);

    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements
     * tickless idle mode
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, RTC, sets up 
 *  the MQTT client task, enables CM55 and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/

int main(void)
{
    cy_rslt_t result;
    rtc_type obj;
    /* Initialize the board support package. */
    
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
    CY_UNUSED_PARAMETER(result);

    /* Enable global interrupts. */
    __enable_irq();

#ifdef ENABLE_SECURE_SOCKETS_LOGS
    /*
        This function is required only if there are some problems with the
        secure sockets (pkcs11 interface to the optiga chip) and you would
        like to understand better what is happening. Additionally add the
        following Macros for the Makefile list of DEFINES
        CY_SECURE_SOCKETS_PKCS_SUPPORT MBEDTLS_VERBOSE=4
    */
    cy_rslt_t rs = cy_log_init(CY_LOG_MAX, app_log_output_callback, app_log_time);
    if (rs != CY_RSLT_SUCCESS)
    {
        printf("cy_log_init() FAILED %ld\n", result);
    }
#endif

    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();

    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* Initialize rtc */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);
    
    /* Initialize the CLIB support library */
    mtb_clib_support_init(&obj);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("PSoC Edge E84 + OPTIGA Trust M + TESAIoT Platform\n");
    printf("===============================================================\n");
    #if TESAIOT_DEBUG_INFO_ENABLED
    printf("%s\n", LABEL_FEATURES);
    #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    printf("  - OPTIGA Trust M V3 hardware security\n");
    printf("  - Protected Update workflow with TESAIoT Platform (MQTT)\n");
    printf("  - Certificate Lifecycle Management (Protected Update)\n");
    printf("  - Factory pre-provisioning + secure certificate renewal\n");
    printf("  - Mutual TLS authentication\n");
    printf("===============================================================\n\n");

    /* NOTE: TESAIoT License Check is performed in optiga_client_task()
     * AFTER OPTIGA Trust M is initialized. This is required because
     * tesaiot_license_init() reads the device UID from OPTIGA.
     */

    /* Enable CM55. CY_CORTEX_M55_APPL_ADDR must be updated if CM55 memory layout is changed. */
    Cy_SysEnableCM55(MXCM55, CY_CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_US);


    /*Create an OPTIGA Task */
    result = xTaskCreate(optiga_client_task, "OPTIGA", 1024 * 12, NULL, 2, NULL);


    if( pdPASS == result )
    {
        /* Start the FreeRTOS scheduler. */
        vTaskStartScheduler();
        
        /* Should never get here. */
        handle_app_error();
    }
    else
    {
        handle_app_error();
    }
}

/*******************************************************************************
* FreeRTOS Hook Functions for Debugging
*******************************************************************************/

/**
 * @brief Stack overflow hook - called by FreeRTOS when stack overflow detected
 *
 * This hook is called when configCHECK_FOR_STACK_OVERFLOW = 2 detects overflow.
 * Critical for debugging Publisher task stack issues (256 words = 1024 bytes).
 *
 * @param xTask Handle of task with stack overflow
 * @param pcTaskName Name of task with stack overflow
 */

/* [] END OF FILE */
