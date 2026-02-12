/******************************************************************************
* File Name: mqtt_client_config.h
*
* Description: MQTT client configuration for TESAIoT Platform - OPTIGA Trust M
* Device ID: 61e0b46b-e9b1-4............
* Factory UID: CD16339301001C000500000A..............
* Generated: 2025-12-15T17:20:13.831510+00:00
*
* Instructions:
* 1. Replace the content in your mqtt_client_config.h with this configuration
* 2. This uses mutual TLS mode with OPTIGA Trust M secure element
* 3. Factory UID (Trust M UID from OID 0xE0C2) is used as MQTT client identifier
* 4. Device ID is used for topic routing on the TESAIoT Platform
*
* Related Document: See TESAIoT Platform Documentation
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

#ifndef MQTT_CLIENT_CONFIG_H_
#define MQTT_CLIENT_CONFIG_H_

#ifndef CY_MQTT_MAX_OUTGOING_PUBLISHES
#define CY_MQTT_MAX_OUTGOING_PUBLISHES ( 2U )  /* Reduced from 5 to save ~300-500 bytes RAM */
#endif

#include "cy_mqtt_api.h"

/*******************************************************************************
* Debug Controls (Moved to Makefile for global scope)
*******************************************************************************/
/* TESAIOT_MQTT_DEBUG_ENABLED is now defined in Makefile via DEFINES variable.
 * This ensures ALL .c files (project + library code like cy_mqtt_api.c) see
 * the same debug flag without requiring #include of this header.
 * Professional MCU practice: Use -D compiler flag, not header-based defines.
 *
 * To enable: Add DEFINES+=TESAIOT_MQTT_DEBUG_ENABLED=1 in Makefile
 * To disable: Remove or change to TESAIOT_MQTT_DEBUG_ENABLED=0 in Makefile
 */

/*******************************************************************************
* Debug Level System - Conditional Compilation
*******************************************************************************/
/* Fine-grained debug control to reduce production binary size by 15-20%.
 *
 * USAGE:
 * Production build: #define TESAIOT_DEBUG_LEVEL TESAIOT_DEBUG_LEVEL_ERROR
 * Development build: #define TESAIOT_DEBUG_LEVEL TESAIOT_DEBUG_LEVEL_VERBOSE
 *
 * BENEFITS:
 * - Production: Smaller binary, faster execution (no printf overhead)
 * - Development: Full debug output (same as before)
 * - Zero functional changes - only affects debug output
 *
 * DEBUG LEVELS:
 * NONE (0): No debug output at all (absolute minimum)
 * ERROR (1): Errors only (recommended for production)
 * WARNING (2): Errors + warnings
 * INFO (3): Errors + warnings + info messages
 * VERBOSE (4): All debug messages (recommended for development)
 */

/* Define debug levels */
#define TESAIOT_DEBUG_LEVEL_NONE 0
#define TESAIOT_DEBUG_LEVEL_ERROR 1
#define TESAIOT_DEBUG_LEVEL_WARNING 2
#define TESAIOT_DEBUG_LEVEL_INFO 3
#define TESAIOT_DEBUG_LEVEL_VERBOSE 4

/* Set current debug level (can be overridden in Makefile with -DTESAIOT_DEBUG_LEVEL=x) */
#ifndef TESAIOT_DEBUG_LEVEL
#define TESAIOT_DEBUG_LEVEL TESAIOT_DEBUG_LEVEL_INFO /* INFO level: show workflow steps and important events */
#endif

/* Component-specific debug flags (derived from TESAIOT_DEBUG_LEVEL) */
#define TESAIOT_DEBUG_MQTT_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_INFO)
#define TESAIOT_DEBUG_TRUSTM_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_VERBOSE)
#define TESAIOT_DEBUG_CERTIFICATE_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_VERBOSE)
#define TESAIOT_DEBUG_CSR_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_VERBOSE)
#define TESAIOT_DEBUG_PROTECTED_UPDATE_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_VERBOSE)
#define TESAIOT_DEBUG_SUBSCRIBER_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_INFO)
#define TESAIOT_DEBUG_PUBLISHER_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_INFO)
#define TESAIOT_DEBUG_MENU_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_INFO)

/* Always-enabled critical messages (errors, warnings) */
#ifndef TESAIOT_DEBUG_ERROR_ENABLED
#define TESAIOT_DEBUG_ERROR_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_ERROR)
#endif
#ifndef TESAIOT_DEBUG_WARNING_ENABLED
#define TESAIOT_DEBUG_WARNING_ENABLED (TESAIOT_DEBUG_LEVEL >= TESAIOT_DEBUG_LEVEL_WARNING)
#endif

/*******************************************************************************
* Macros
********************************************************************************/

/***************** LOCAL MOSQUITTO TESTING *****************/
/* Enable this to test against local Mosquitto Docker container
 * Run: cd mosquitto-docker && docker compose up -d
 * Disable (comment out) for production TESAIoT Platform
 */
// #define USE_LOCAL_MOSQUITTO
#define LOCAL_MOSQUITTO_IP                "192.168.1.110"

/***************** TLS MODE SELECTION *****************/
/* Select ONE TLS mode:
 * - USE_MTLS_MODE: Mutual TLS with OPTIGA Trust M (port 8883, client cert)
 * - USE_SERVER_TLS_MODE: Server TLS only (port 8884, API key/password)
 *
 * Both modes use the SAME device - mTLS devices can use Server-TLS as fallback
 * when certificate needs to be re-issued (e.g., key mismatch recovery)
 */
#define USE_MTLS_MODE
//#define USE_SERVER_TLS_MODE

/***************** CERTIFICATE RECOVERY MODE *****************/
/* Runtime flag for forcing Factory Certificate usage.
 * Set via runtime g_force_factory_cert flag - no rebuild needed!
 *
 * USE CASE: Recovery when Device Certificate exists but doesn't match key in 0xE0F1
 *
 * WORKFLOW:
 * 1. SAFE MODE is enabled by default on boot (g_force_factory_cert = true)
 * 2. Run CSR Workflow - connects with Factory Cert, gets new Device Cert
 * 3. Flag auto-resets after successful CSR, or on next boot
 */
/* Note: FORCE_FACTORY_CERTIFICATE compile flag removed - now using runtime g_force_factory_cert */

/***************** DEVICE IDENTIFICATION *****************/
/* SINGLE device identity - used for BOTH mTLS and Server-TLS modes
 * This allows mTLS devices to fallback to Server-TLS for certificate recovery
 *
 * DEVICE_ID: UUID registered on TESAIoT Platform (for topic routing)
 * FACTORY_UID: Trust M hardware UID from OID 0xE0C2 (for MQTT client ID)
 * API_KEY: Server-TLS fallback password (generated from TESAIoT Platform)
 */

/* Active device configuration */
/* TODO: Get these values from TESAIoT Platform after device registration */
#define DEVICE_ID                         "YOUR_DEVICE_ID_HERE"          /* Example: "1167fe60-6108-40dc-aefc-67a942d333ac" */
#define FACTORY_UID                       "YOUR_FACTORY_UID_HERE"       /* Read from Menu Option 1 */
#define API_KEY                           "YOUR_API_KEY_HERE"           /* Get from Device's credential Tab at TESAIoT Platform */
                                            
/***************** MQTT CLIENT CONNECTION CONFIGURATION MACROS *****************/
/* MQTT Broker/Server address and port used for the MQTT connection. */
#ifdef USE_LOCAL_MOSQUITTO
#define MQTT_BROKER_ADDRESS               LOCAL_MOSQUITTO_IP
#else
#define MQTT_BROKER_ADDRESS               "mqtt.tesaiot.com"
#endif

#ifdef USE_SERVER_TLS_MODE
/* Server-TLS mode: Port 8884, API key authentication (fallback for mTLS devices) */
#define MQTT_PORT                         8884
#define MQTT_SECURE_CONNECTION            ( 1 )
#define MQTT_ENABLE_MUTUAL_AUTH           ( 0 )
#define MQTT_USERNAME                     DEVICE_ID
#define MQTT_PASSWORD                     API_KEY
#else
/* mTLS mode: Port 8883, client certificate from OPTIGA Trust M (primary mode) */
#ifdef USE_LOCAL_MOSQUITTO
#define MQTT_PORT                         8885  /* Local Mosquitto: remapped from internal 8883 */
#else
#define MQTT_PORT                         8883
#endif
#define MQTT_SECURE_CONNECTION            ( 1 )
/* Enable mutual TLS authentication
 * TRUST M MODE: Configured for two-way authentication with secure element
 * - Using port 8883 (MQTTs) with mutual TLS authentication
 * - Client certificate stored in Trust M OID 0xE0E0 or 0xE0E1
 * - Private key secured in Trust M hardware (never exported)
 * - Provides highest level of security with hardware-backed keys
 */
#define MQTT_ENABLE_MUTUAL_AUTH           ( 1 )
/* Configure the user credentials to be sent as part of MQTT CONNECT packet
 * For Trust M mTLS: Username is device_id for Platform routing
 * Uses DEVICE_ID macro defined above - change DEVICE_ID to update everywhere
 */
#define MQTT_USERNAME                     DEVICE_ID
#define MQTT_PASSWORD                     ""  /* Empty for mTLS authentication */
#endif


/********************* MQTT MESSAGE CONFIGURATION MACROS **********************/
/* Base topics for commands and telemetry
 * Uses C string literal concatenation: "device/" DEVICE_ID "/commands"
 * Compiler combines adjacent string literals automatically
 */
#define MQTT_COMMAND_TOPIC_BASE           "device/" DEVICE_ID "/commands"
#define MQTT_TELEMETRY_TOPIC_BASE         "device/" DEVICE_ID "/telemetry"

/* The MQTT topics to be used by the publisher and subscriber. */
#define MQTT_PUB_TOPIC                    MQTT_TELEMETRY_TOPIC_BASE
#define MQTT_PUB_TOPIC_SENSOR             MQTT_TELEMETRY_TOPIC_BASE "/sensor"

/*
 * Default subscription topic listens for device-specific commands. If you need
 * broader coverage (for example, to capture config or firmware broadcasts),
 * switch MQTT_SUB_TOPIC to one of the wildcard variants below.
 */
#define MQTT_SUB_TOPIC_COMMAND            MQTT_COMMAND_TOPIC_BASE "/#"
#define MQTT_SUB_TOPIC_DEVICE_ALL         "device/" DEVICE_ID "/#"
#define MQTT_SUB_TOPIC_LEGACY_ALL         DEVICE_ID "/#"

/* Active subscription topic (default to commands channel with wildcard). */
#define MQTT_SUB_TOPIC                    MQTT_SUB_TOPIC_COMMAND

/* Command publish topics (Device -> Platform) */
#define MQTT_PUB_TOPIC_COMMAND_CSR        MQTT_COMMAND_TOPIC_BASE "/csr"
#define MQTT_PUB_TOPIC_COMMAND_REQUEST    MQTT_COMMAND_TOPIC_BASE "/request"
#define MQTT_PUB_TOPIC_COMMAND_STATUS     MQTT_COMMAND_TOPIC_BASE "/status"
#define MQTT_PUB_TOPIC_COMMAND_ACK        MQTT_COMMAND_TOPIC_BASE "/ack"

/* Command subscription topics (Platform -> Device) */
#define MQTT_SUB_TOPIC_COMMAND_PROTECTED_UPDATE MQTT_COMMAND_TOPIC_BASE "/protected_update"
#define MQTT_SUB_TOPIC_COMMAND_CERT       MQTT_COMMAND_TOPIC_BASE "/certificate"
#define MQTT_SUB_TOPIC_COMMAND_CONFIG     MQTT_COMMAND_TOPIC_BASE "/config"
#define MQTT_SUB_TOPIC_COMMAND_FIRMWARE   MQTT_COMMAND_TOPIC_BASE "/firmware"

/* Smart Auto-Fallback response topics */
#define MQTT_SUB_TOPIC_COMMAND_CHECK_CERT_RESPONSE  MQTT_COMMAND_TOPIC_BASE "/check_certificate_response"
#define MQTT_SUB_TOPIC_COMMAND_UPLOAD_CERT_RESPONSE MQTT_COMMAND_TOPIC_BASE "/upload_certificate_response"
#define MQTT_SUB_TOPIC_COMMAND_SYNC_CERT_RESPONSE   MQTT_COMMAND_TOPIC_BASE "/sync_certificate_response"

/* DEPRECATED topics (kept for reference, will be removed in future) */
#define MQTT_SUB_TOPIC_COMMAND_MANIFEST   MQTT_COMMAND_TOPIC_BASE "/manifest"
#define MQTT_SUB_TOPIC_COMMAND_FRAGMENT   MQTT_COMMAND_TOPIC_BASE "/fragment"
#define MQTT_SUB_TOPIC_COMMAND_PUB_KEY    MQTT_COMMAND_TOPIC_BASE "/pubkey"

/* Set the QoS that is associated with the MQTT publish, and subscribe messages.
 * Valid choices are 0, 1, and 2. Other values should not be used in this macro.
 */
#define MQTT_MESSAGES_QOS                 ( 1 )

/* Configuration for the 'Last Will and Testament (LWT)'. It is an MQTT message
 * that will be published by the MQTT broker if the MQTT connection is
 * unexpectedly closed. This configuration is sent to the MQTT broker during
 * MQTT connect operation and the MQTT broker will publish the Will message on
 * the Will topic when it recognizes an unexpected disconnection from the client.
 *
 * If you want to use the last will message, set this macro to 1 and configure
 * the topic and will message, else 0.
 */
#define ENABLE_LWT_MESSAGE                ( 0 )
#if ENABLE_LWT_MESSAGE
 #define MQTT_WILL_TOPIC_NAME             MQTT_TELEMETRY_TOPIC_BASE "/will"
 #define MQTT_WILL_MESSAGE                ("MQTT client unexpectedly disconnected!")
#endif

/* MQTT messages which are published on the MQTT_PUB_TOPIC that control the
 * device (user LED in this example) state in this code example.
 */
#define MQTT_DEVICE_ON_MESSAGE            "TURN ON"
#define MQTT_DEVICE_OFF_MESSAGE           "TURN OFF"


/******************* OTHER MQTT CLIENT CONFIGURATION MACROS *******************/
/* A unique client identifier to be used for every MQTT connection.
 * For Trust M devices: Uses FACTORY_UID (Trust M UID, 54 hex characters)
 * This ensures globally unique client ID from hardware secure element
 * Uses FACTORY_UID macro defined above - hardware-backed unique identifier
 */
 #ifdef USE_SERVER_TLS_MODE
/* Server-TLS device (different device registration on Platform) */
#define MQTT_CLIENT_IDENTIFIER            DEVICE_ID
#elif defined(USE_MTLS_MODE)
/* mTLS device with OPTIGA Trust M */
#define MQTT_CLIENT_IDENTIFIER            FACTORY_UID
#endif

/* The timeout in milliseconds for MQTT operations in this example. */
#define MQTT_TIMEOUT_MS                   ( 10000 ) /* Increased to 10s for QoS 1 PUBACK wait */

/* The keep-alive interval in seconds used for MQTT ping request. */
#define MQTT_KEEP_ALIVE_SECONDS           ( 180 )

/* Every active MQTT connection must have a unique client identifier. If you
 * are using the above 'MQTT_CLIENT_IDENTIFIER' as client ID for multiple MQTT
 * connections simultaneously, set this macro to 1. The device will then
 * generate a unique client identifier by appending a timestamp to the
 * 'MQTT_CLIENT_IDENTIFIER' string. Example: 'psoc6-mqtt-client5927'
 */
#define GENERATE_UNIQUE_CLIENT_ID         ( 0 ) /* Disabled - Trust M UID is already unique */

/* The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. However some MQTT brokers support
 * longer client IDs. Configure this macro as per the MQTT broker specification.
 * For Trust M UID (54 hex chars): Set to 64 for buffer safety
 */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 64 )

/* Server Name Indication (SNI) hostname to be sent as part of the Client Hello
 * during TLS handshake as specified by the MQTT Broker.
 */
#ifdef USE_LOCAL_MOSQUITTO
#define MQTT_SNI_HOSTNAME                 "localhost"
#else
#define MQTT_SNI_HOSTNAME                 "mqtt.tesaiot.com"
#endif

/* A Network buffer is allocated for sending and receiving MQTT packets over
 * the network. Specify the size of this buffer using this macro.
 *
 * Note: The minimum buffer size is defined by 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'
 * macro in the MQTT library. Please ensure this macro value is larger than
 * 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'.
 *
 * CSR Workflow requires larger buffer:
 * - CSR JSON payload: ~3100 bytes (Base64 CSR + metadata)
 * - Protected Update manifest: ~2500 bytes
 * - Protected Update fragment: ~1500 bytes per chunk
 * Reduced to 3.2KB (from 5KB) to save ~1.9 KB RAM (98.88% usage crisis)
 * 3200 bytes provides 100-byte safety margin for CSR payload
 */
#define MQTT_NETWORK_BUFFER_SIZE          ( 3200 )

/* Maximum MQTT connection re-connection limit. */
#define MAX_MQTT_CONN_RETRIES             (150u)

/* MQTT re-connection time interval in milliseconds. */
#define MQTT_CONN_RETRY_INTERVAL_MS       (2000)

/* Optional ALPN (for example, when tunnelling MQTT over HTTPS/port 443). */
// #define MQTT_ALPN_PROTOCOL_NAME         "x-amzn-mqtt-ca"


/**************** MQTT CLIENT CERTIFICATE CONFIGURATION MACROS ****************/
/* Configure the below credentials in case of a secure MQTT connection. */

/*
 * Certificate Material Mapped to Infineon's OPTIGA™ Trust M OIDs
 *
 * Recommended mapping:
 *   - 0xE0E8 : Infineon factory trust anchor (OPTIGA_TRUSTM_FACTORY_ANCHOR)
 *   - 0xE0E9 : TESAIoT Root/Intermediate CA chain (for TLS server verification)
 *   - 0xE0E0 : Factory device certificate (before rotation)
 *   - 0xE0E1 : TESAIoT-issued device certificate (after Protected Update)
 *   - 0xE0F1 : CSR/Key slot for certificate rotation
 */

/* TESAIoT Root/Intermediate CA chain (used for TLS server verification)
 * Store in OID 0xE0E9 for Trust Anchor during Protected Update rotation
 */

/*******************************************************************************
 * ROOT_CA_CERTIFICATE - MQTT Broker Root CA
 *******************************************************************************
 * USAGE: Used in mqtt_client_config.c for MQTT TLS handshake
 *   - security_info.root_ca = ROOT_CA_CERTIFICATE
 *   - Verifies MQTT broker server certificate
 *
 * USED IN: Menu 2, 3, 4 (all MQTT connections)
 *   - Menu 2: Test MQTT connection
 *   - Menu 3: CSR workflow (connects to TESAIoT via MQTT)
 *   - Menu 4: Protected Update workflow (connects to TESAIoT via MQTT)
 *
 * NOTE: This is for TLS server authentication (broker → client)
 *       Separate from OPTIGA Trust M client authentication (client → broker)
 ******************************************************************************/
#ifdef USE_LOCAL_MOSQUITTO
/* Self-signed CA for local Mosquitto Docker testing */
#define ROOT_CA_CERTIFICATE \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDnzCCAoegAwIBAgIUI5+Gm14rrPbSUA24bFXyPcVdjBMwDQYJKoZIhvcNAQEL\n" \
"BQAwXzELMAkGA1UEBhMCVEgxEDAOBgNVBAgMB0Jhbmdrb2sxEDAOBgNVBAcMB0Jh\n" \
"bmdrb2sxFTATBgNVBAoMDFRlc3QgTVFUVCBDQTEVMBMGA1UEAwwMVGVzdCBSb290\n" \
"IENBMB4XDTI2MDExMTE2MTk0MFoXDTI3MDExMTE2MTk0MFowXzELMAkGA1UEBhMC\n" \
"VEgxEDAOBgNVBAgMB0Jhbmdrb2sxEDAOBgNVBAcMB0Jhbmdrb2sxFTATBgNVBAoM\n" \
"DFRlc3QgTVFUVCBDQTEVMBMGA1UEAwwMVGVzdCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAt/9PQwzekVwjb693tQd2zbqv0Q3WU9u/N/PC\n" \
"AirS+SMMCvtDuZeFayrbiL1UXLSHs4VnK+Gnyemiop+D3LjylnRcdt3WSqjk6/4h\n" \
"Y3Q/2Pvzw/dbG1kp6D0YOsF/HAyGw2zzLglja1OdE063vwVe9qxnU2Ic7dBz5gMT\n" \
"F8v2i+P6s2N4R6G2e9H1/hfEbUAZfoucd0QTFPSUTsKZgEA9cW9SoE/nrJhbF9/k\n" \
"BYMSHeXtwidNRv7jo2gcqznfuyCUvUfGniKKePHdHG+78ewOsRJkPOzsYPmhmf1m\n" \
"Kvq61EKq6cRDKan+Mw4zgZ0WhXRl/RlY+EksH6teehY/CZjDNwIDAQABo1MwUTAd\n" \
"BgNVHQ4EFgQUwhojE6mXjMt6wgb4pKXJR0asrlYwHwYDVR0jBBgwFoAUwhojE6mX\n" \
"jMt6wgb4pKXJR0asrlYwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC\n" \
"AQEAih2Opc2rRecLO5vIw5gH3b8XDaHl+Rz2Iw3uglwXMXujN8AeY5JZuT0rtUEF\n" \
"jbO4YrKkRBJ9/5FFyRcNvzw7fVdBbR91R2/pUjqOzl3CZPKZMYdpm7FwlIS1xmR+\n" \
"ykZnf2wkdIZHdzGNiz7X9Q9uL9K2dgxswstqYK2cSNDwuRmkzHijDAtwNpVjihTr\n" \
"0jrg35Q0cSUfad1m8ZOTTieLwf8t2qjNwyBiJf+TB8HXh0unF9bwnZAT1ERBqe4d\n" \
"nC1PioQmt7s1gL3pkaX7gjh0TmXjdgTeKbbRN4G1fiD0oKa2pJG3CVEuAjJeOLcg\n" \
"ulJ3ZggfHt++YZ3mEfuN9qdbzw==\n" \
"-----END CERTIFICATE-----\n"
#else
/* TESAIoT Platform CA chain */
#define ROOT_CA_CERTIFICATE \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFGTCCAwGgAwIBAgIUQyPdoywMvhynALmoOkaVRjRxdgMwDQYJKoZIhvcNAQEL\n" \
"BQAwgYcxCzAJBgNVBAYTAlRIMRAwDgYDVQQIEwdCYW5na29rMRAwDgYDVQQHEwdC\n" \
"YW5na29rMRowGAYDVQQKExFURVNBIElvVCBQbGF0Zm9ybTEeMBwGA1UECxMVQ2Vy\n" \
"dGlmaWNhdGUgQXV0aG9yaXR5MRgwFgYDVQQDEw9URVNBSW9UIFJvb3QgQ0EwHhcN\n" \
"MjUwOTA2MTE1NDUyWhcNMzAwOTA1MTE1NTIyWjAiMSAwHgYDVQQDExdURVNBSW9U\n" \
"IEludGVybWVkaWF0ZSBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AMkIdY3CwBaqRtQkugeRMgXOT8maywvc56zHil/nBsbrfD1c4WdF1sUKnALiKlOz\n" \
"6PER5lQRKOloVxRsXfA3/5JnRXipoPpjh0wSxzdec8rmOAB2zOjPx9ZC4OFS1Oc5\n" \
"3nfE015dCM3qmt5YU+0fMrOkdHg84cG3mqwlEPCiyR2q6Q90eEe6oDAbdZf/VP/8\n" \
"m6Nto+h5q9HskeLb+Q597I/1mGxVfuQ44dLEhjIi4xXsVoYEP/huPd6QKNMfiYyd\n" \
"UggGzIOxOWXvpbAaERzMV4ORCBQLP0GT+ErzthjOoDdsYCn524vPVHUnDaKaNr2e\n" \
"DdQYL+9FQJuJKILJx/yDx30CAwEAAaOB4DCB3TAOBgNVHQ8BAf8EBAMCAQYwDwYD\n" \
"VR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUYOv9VE15yidpsZ6fT0RhYwXVuJgwHwYD\n" \
"VR0jBBgwFoAUzeeXC4tRrrEdw4RCROaPjfMtVhkwQQYIKwYBBQUHAQEENTAzMDEG\n" \
"CCsGAQUFBzAChiVodHRwOi8vdGVzYS12YXVsdDo4MjAwL3YxL3BraS1yb290L2Nh\n" \
"MDcGA1UdHwQwMC4wLKAqoCiGJmh0dHA6Ly90ZXNhLXZhdWx0OjgyMDAvdjEvcGtp\n" \
"LXJvb3QvY3JsMA0GCSqGSIb3DQEBCwUAA4ICAQBlpS+p+VHNBoNZoyx+tYwPn/QK\n" \
"WYsmd6m4RL7Tdb+MQ1w3STDrcQBXZvpgn1V00cKoEnOrw2obtW/zIdAjXm3XGh9g\n" \
"ODna7olj/lRZa5OxyN4Yy2HdWLI6vomMF24GJz0Ve+aeNbLAU7TET2wzUiOpi15E\n" \
"wsByAXpsud7bYPnyuQ+QF2Aiau0ZVB5sMmylvHVRewXeGSb6mnQHsadtMEDJHkMr\n" \
"uhY/1WZmafjETo7uOQi2BNv2X7taOam9NCIluXNLQxrQyljyrF7tkeeJQfRp0vJc\n" \
"cP1AKmbu5wQJyDtLfJD6IbyyOkth5yyVnWHLZkOXjJ3sgXN6/mtMEv7y0wItOSVd\n" \
"e7N/O5CK+BOYYrqQXdHF5Vd0fuCaUrVVC2nXWZPXTtet4ShN6uHQKJY+Bx1IU2/U\n" \
"yBVj6vk1fQM5P1ixL44xNd3tanNX3n9Z/rVToo8iuHSVQkjuFqbc3puFpoWjMZr6\n" \
"uOEUkamOryx4wC/dC0Y5IO8M/MqGZWocwf4YuAJF5ApkyNbXTscnJYZmi3F31+6p\n" \
"rKzG2CUpNm2b5FxU9zefBe8ASoMF/h6+yqrnohHTtAWmvhh1IcJDpDiYOWu2E21c\n" \
"KRVacmR6J4vQOSL9sj1HVw+zntFPNBQo7pu3ISQgBMBmknR9vWXCy4/tXo+2pgDI\n" \
"xhdvQh0uPlE5Luai0A==\n" \
"-----END CERTIFICATE-----\n" \
"-----BEGIN CERTIFICATE-----\n" \
"MIIGATCCA+mgAwIBAgIUeSB82qv/64u3Ud+oZXUOdDZ0cx0wDQYJKoZIhvcNAQEL\n" \
"BQAwgYcxCzAJBgNVBAYTAlRIMRAwDgYDVQQIEwdCYW5na29rMRAwDgYDVQQHEwdC\n" \
"YW5na29rMRowGAYDVQQKExFURVNBIElvVCBQbGF0Zm9ybTEeMBwGA1UECxMVQ2Vy\n" \
"dGlmaWNhdGUgQXV0aG9yaXR5MRgwFgYDVQQDEw9URVNBSW9UIFJvb3QgQ0EwHhcN\n" \
"MjUwOTA2MTE1NDMyWhcNMzUwOTA0MTE1NDU5WjCBhzELMAkGA1UEBhMCVEgxEDAO\n" \
"BgNVBAgTB0Jhbmdrb2sxEDAOBgNVBAcTB0Jhbmdrb2sxGjAYBgNVBAoTEVRFU0Eg\n" \
"SW9UIFBsYXRmb3JtMR4wHAYDVQQLExVDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxGDAW\n" \
"BgNVBAMTD1RFU0FJb1QgUm9vdCBDQTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCC\n" \
"AgoCggIBAJhC+0WyByj85dlnI/v1W78cspV3lrAnshVmmoLVWelgaNKyfU2uO6Tc\n" \
"sNUX2V8qty0J8jJwK2sdHY1V7CmN/GAi2DinhX0My1NyFxfs7MS1TQhNmqTqgf70\n" \
"N1FpqB4QsG2dAbrPPDRaCkQyuSxwkldXSTJz1NA8vCNq54gc4O0g6ZwWHAJaWyqy\n" \
"4U89i6+fW167IwElqr5Pcz2Jal62he8fafFcRPpPoRfDMecvlTeK2cUmH50ivO9N\n" \
"GaYGPmmip/2DawT8fZXDR4rKVry66ZGzUD2tmKtzuWGLHLAUHKGnTrVGNCodwWAY\n" \
"lkT99uqcyNBWvO0XPfcQ3NSvO4pYJ6Pwt+BhuQyRnRNbNd2oHdSsNhZLZjtgLMUP\n" \
"0p3b5Rvt4JEkb3MiDaRVfIfm0mkoKoKIKn573hagiiB53DM2k4S7ZjSwmZh7z5h0\n" \
"sdXdC7zIL5kjhrW/Q9cIX2dj67LXOuhDU/Ve3UyIlHRW+RbIFHOy/bsDZWYoldkE\n" \
"dGmb+edcWtGfO8oPhQFFspUk+MzSwkfCpRpP3tremNnmTbom4ppIBxAFNjesFsfO\n" \
"OOVyxKsH+esHrTZLoFpQQ+Jwb/y4NufHuZ9x1AENLXLLbLF0oonWrdjC7Lk96GOs\n" \
"AWSHUDSefoAQzPtwJUumf5Dxrc5y4jOZ8tNqrLo/c2Cf5kHZgCw3AgMBAAGjYzBh\n" \
"MA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTN55cL\n" \
"i1GusR3DhEJE5o+N8y1WGTAfBgNVHSMEGDAWgBTN55cLi1GusR3DhEJE5o+N8y1W\n" \
"GTANBgkqhkiG9w0BAQsFAAOCAgEAOEP6PUFE7CZvr5L/HvD81roekDMtxRZlyS7U\n" \
"9+a4LJOZlqLKuwaN3h3yh47TdFV3OAtsmwX+BqqDtjvlXEv9zPeLrVIC/+2MwKri\n" \
"8EO0ZjnL1LPiVoMqImh6p4OOuorDilCQs6OCGKJeWDazY2Rosc4UdosB0ESjJcbA\n" \
"BJgNThO+ZaJFFoTcxdu19L4J7jRqN4/pFQa+/W9ZZkGlZCRUIz9FMhRhi4NwKE+V\n" \
"KbOK02KqJ6MUjhQjqhWLVcKfKxBfHo65HigEm4s8E+mJrWxw+xbr9ZRbyDE8Kr8C\n" \
"TJrZczDP7bCaJkq5aZN+e7kO6zLjxIlPo2fG+lfJU1uy4nL/8Mvd2Bm/VdYQqEsG\n" \
"t7soEI6oKQ8PDm2XA0ojNnkdgQunw9ikCpodVZeeVbMB41MAfHVBeE76H5RnOrkD\n" \
"pvbANmCUgQ1r3/uDmXfIwuTrMw1aUSodQIhr7VdyqtOIDSJPIloPe58ponCJmSgg\n" \
"GQ5zxhCZwj8yzv4ZJeVX1BrRb9DMePrihIjJMBKhVw9nWv2OFujHFm2lc+W4GzCM\n" \
"7yBpMQBw3aPad+QE0YEVTk3B9cpBIsXbEr3u3Qmf6VQY92XgaEk7HhIGjjDexL/5\n" \
"eQzdinKseYWGQ2OcSPabIlPfAMxKpAmeIJzTg9bkITCJcV0aXrhk62wOZzOt/Ioo\n" \
"iAfI2Ao=\n" \
"-----END CERTIFICATE-----\n"
#endif /* USE_LOCAL_MOSQUITTO */

/* TESAIoT-issued client certificate and private key
 * COMMENTED OUT: Trust M uses hardware-secured credentials via PSA integration
 * - Factory certificate in OID 0xE0E0 (before rotation)
 * - TESAIoT certificate in OID 0xE0E1 (after Protected Update)
 * - Private key never leaves Trust M hardware
 * DO NOT UNCOMMENT: Empty defines cause cy_tls_create_identity() BADARG errors
 */
// #define CLIENT_CERTIFICATE       ""
// #define CLIENT_PRIVATE_KEY       ""


/*******************************************************************************
* Notes for Firmware Integration - OPTIGA Trust M Workflow
*******************************************************************************/
/*
 * FACTORY PROVISIONING (One-time setup):
 * 1. Store OPTIGA_TRUSTM_FACTORY_ANCHOR in OID 0xE0E8
 * 2. Store ROOT_CA_CERTIFICATE (TESAIoT chain) in OID 0xE0E9
 * 3. Factory certificate already in OID 0xE0E0 from Infineon production
 *
 * FIRST BOOT (Factory certificate):
 * 1. Device uses factory cert (OID 0xE0E0) for initial TLS handshake
 * 2. Device connects to TESAIoT Platform using DEVICE_ID and FACTORY_UID
 * 3. Platform verifies factory certificate against Infineon CA
 *
 * PROTECTED UPDATE (Certificate rotation):
 * 1. Device triggers Protected Update via console menu or API
 * 2. Platform signs new CSR and creates Protected Update manifest
 * 3. Device writes new certificate to OID 0xE0E1 via Protected Update
 * 4. After rotation, device uses OID 0xE0E1 for TLS (not factory cert)
 *
 * OID REFERENCE:
 * - 0xE0C2 : Trust M UID (read-only, 27 bytes = 54 hex chars)
 * - 0xE0E0 : Factory device certificate (Infineon-issued)
 * - 0xE0E1 : TESAIoT device certificate (after rotation)
 * - 0xE0E8 : Infineon factory CA (trust anchor)
 * - 0xE0E9 : TESAIoT CA chain (trust anchor for rotation)
 * - 0xE0F1 : CSR key slot for rotation workflow
 *
 * Need help? https://docs.tesaiot.com/security/trustm
 */

/******************************************************************************
* Global Variables
*******************************************************************************/
extern cy_mqtt_broker_info_t broker_info;
extern cy_awsport_ssl_credentials_t *security_info;
extern cy_mqtt_connect_info_t connection_info;

#endif /* MQTT_CLIENT_CONFIG_H_ */
