/**
 * @file tesaiot_config.h
 * @brief TESAIoT Platform & Device Configuration
 * @version 2.0
 * @date 2026-01-23
 *
 * @note Reference: PSoC E84 mqtt_client_config.h
 *       This file contains all platform configurations and extern declarations.
 *
 * Architecture (PSoC E84 Pattern):
 * - Library: Provides API + extern declarations (device-agnostic)
 * - Client: Defines actual values in their own tesaiot_config.h
 *
 * @warning The TESAIoT Platform Public Key is embedded in tesaiot_license.c
 *          for security reasons and is NOT exposed here.
 */

#ifndef TESAIOT_CONFIG_H
#define TESAIOT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Extern Declarations (Client MUST define these)
 *===========================================================================*/

/**
 * @brief Device ID (UUID) from TESAIoT Platform
 *
 * Format: UUID v4 (36 characters including hyphens)
 * Example: "04284137-84be-4f86-8b91-d4d8fef51427"
 *
 * Purpose: MQTT topic routing on TESAIoT Platform
 * Topics: device/{DEVICE_ID}/commands/csr, device/{DEVICE_ID}/commands/certificate
 *
 * @note Client must define this in their tesaiot_config.h:
 *       const char *g_tesaiot_device_id = "your-device-uuid";
 */
extern const char *g_tesaiot_device_id;

/**
 * @brief Device UID from OPTIGA Trust M (OID 0xE0C2)
 *
 * Format: 54 hexadecimal characters (27 bytes)
 * Example: "CD16339301001C000500000A01BB820003009C002C801010712440"
 *
 * Purpose:
 * - MQTT Client ID (unique identifier)
 * - License verification (signature is over this UID)
 *
 * @note Client must define this in their tesaiot_config.h:
 *       const char *g_tesaiot_device_uid = "your-trust-m-uid";
 */
extern const char *g_tesaiot_device_uid;

/**
 * @brief License Key from TESAIoT Platform
 *
 * Format: Base64-encoded ECDSA signature (DER format)
 * Algorithm: ECDSA P-256 (secp256r1) with SHA-256
 *
 * Example: "MEUCIEjUX7JFWXDpOTXbKLxiNpj5X22d+FFhIGqsZe5UMsZ0AiEA..."
 *
 * @note Client must define this in their tesaiot_config.h:
 *       const char *g_tesaiot_license_key = "your-license-signature";
 */
extern const char *g_tesaiot_license_key;

/*============================================================================
 * Backward Compatibility Macros (Reference extern variables)
 *===========================================================================*/

/**
 * @brief Macros for backward compatibility with existing code
 *
 * These macros allow existing library code using TESAIOT_DEVICE_ID, etc.
 * to work without modification. They reference the extern variables.
 */
#ifndef TESAIOT_DEVICE_ID
#define TESAIOT_DEVICE_ID       g_tesaiot_device_id
#endif
#ifndef TESAIOT_DEVICE_UID
#define TESAIOT_DEVICE_UID      g_tesaiot_device_uid
#endif
#ifndef TESAIOT_LICENSE_KEY
#define TESAIOT_LICENSE_KEY     g_tesaiot_license_key
#endif

/*============================================================================
 * TLS Mode Selection
 *===========================================================================*/
/**
 * Select ONE TLS mode:
 * - USE_MTLS_MODE: Mutual TLS with OPTIGA Trust M (port 8883, client cert)
 * - USE_SERVER_TLS_MODE: Server TLS only (port 8884, API key/password)
 *
 * Both modes use the SAME device - mTLS devices can use Server-TLS as fallback
 * when certificate needs to be re-issued (e.g., key mismatch recovery)
 */
#define USE_MTLS_MODE
/* #define USE_SERVER_TLS_MODE */

/*============================================================================
 * MQTT Broker Configuration
 *===========================================================================*/

/**
 * @brief TESAIoT Platform MQTT Broker Address
 * @note MUST use mqtt.tesaiot.com (NOT broker.tesaiot.com)
 */
#define TESAIOT_MQTT_BROKER_HOST        "mqtt.tesaiot.com"

/**
 * @brief MQTT Ports
 * - 8883: mTLS (Mutual TLS with client certificate)
 * - 8884: Server TLS only (password/API key authentication)
 */
#ifdef USE_MTLS_MODE
#define TESAIOT_MQTT_PORT               8883
#define TESAIOT_MQTT_ENABLE_MUTUAL_AUTH 1
#else
#define TESAIOT_MQTT_PORT               8884
#define TESAIOT_MQTT_ENABLE_MUTUAL_AUTH 0
#endif

/**
 * @brief Full MQTT Broker URL
 */
#define TESAIOT_MQTT_BROKER_URL         "ssl://" TESAIOT_MQTT_BROKER_HOST ":8883"
#define TESAIOT_MQTT_BROKER_URL_STLS    "ssl://" TESAIOT_MQTT_BROKER_HOST ":8884"

/**
 * @brief Server Name Indication (SNI) for TLS handshake
 */
#define TESAIOT_MQTT_SNI_HOSTNAME       TESAIOT_MQTT_BROKER_HOST

/*============================================================================
 * MQTT Topic Configuration
 *===========================================================================*/

/**
 * @brief Topic Templates (Device ID substituted at runtime)
 *
 * Command Topics (Platform -> Device):
 * - device/{device_id}/commands/certificate
 * - device/{device_id}/commands/protected_update
 * - device/{device_id}/commands/config
 * - device/{device_id}/commands/firmware
 *
 * Response Topics (Device -> Platform):
 * - device/{device_id}/commands/csr
 * - device/{device_id}/commands/status
 * - device/{device_id}/commands/ack
 *
 * Telemetry Topics (Device -> Platform):
 * - device/{device_id}/telemetry
 * - device/{device_id}/telemetry/sensor
 */

#define TESAIOT_TOPIC_COMMAND_BASE      "device/%s/commands"
#define TESAIOT_TOPIC_TELEMETRY_BASE    "device/%s/telemetry"

/* Command Topics */
#define TESAIOT_TOPIC_CSR               "device/%s/commands/csr"
#define TESAIOT_TOPIC_CERTIFICATE       "device/%s/commands/certificate"
#define TESAIOT_TOPIC_PROTECTED_UPDATE  "device/%s/commands/protected_update"
#define TESAIOT_TOPIC_CONFIG            "device/%s/commands/config"
#define TESAIOT_TOPIC_FIRMWARE          "device/%s/commands/firmware"
#define TESAIOT_TOPIC_STATUS            "device/%s/commands/status"
#define TESAIOT_TOPIC_ACK               "device/%s/commands/ack"

/* Subscription Wildcard */
#define TESAIOT_TOPIC_SUB_COMMANDS      "device/%s/commands/#"
#define TESAIOT_TOPIC_SUB_ALL           "device/%s/#"

/*============================================================================
 * MQTT Connection Settings
 *===========================================================================*/

/**
 * @brief MQTT Keep-Alive interval (seconds)
 */
#define TESAIOT_MQTT_KEEP_ALIVE_SEC     180

/**
 * @brief MQTT QoS Level
 * - 0: At most once (fire and forget)
 * - 1: At least once (with ACK) - RECOMMENDED
 * - 2: Exactly once (4-way handshake)
 */
#define TESAIOT_MQTT_QOS                1

/**
 * @brief MQTT Connection Timeout (milliseconds)
 */
#define TESAIOT_MQTT_CONNECT_TIMEOUT_MS 10000

/**
 * @brief MQTT Publish/Subscribe Timeout (milliseconds)
 */
#define TESAIOT_MQTT_TIMEOUT_MS         10000

/**
 * @brief MQTT Network Buffer Size (bytes)
 *
 * Must accommodate:
 * - CSR JSON payload: ~3100 bytes
 * - Protected Update manifest: ~2500 bytes
 * - Protected Update fragment: ~1500 bytes
 */
#define TESAIOT_MQTT_BUFFER_SIZE        4096

/**
 * @brief MQTT Maximum Reconnection Attempts
 */
#define TESAIOT_MQTT_MAX_RETRIES        150

/**
 * @brief MQTT Reconnection Interval (milliseconds)
 */
#define TESAIOT_MQTT_RETRY_INTERVAL_MS  2000

/*============================================================================
 * Certificate OID Mapping (OPTIGA Trust M)
 *===========================================================================*/

/**
 * @brief Trust M OID Reference
 *
 * | OID    | Purpose                                   |
 * |--------|-------------------------------------------|
 * | 0xE0C2 | Trust M UID (read-only, 27 bytes)         |
 * | 0xE0E0 | Factory device certificate (Infineon)     |
 * | 0xE0E1 | LOCKED - DO NOT USE (Change:NEV)          |
 * | 0xE0E2 | TESAIoT device certificate (after CSR)    |
 * | 0xE0E8 | Infineon factory CA (trust anchor)        |
 * | 0xE0E9 | TESAIoT CA chain                          |
 * | 0xE0F0 | Factory private key                       |
 * | 0xE0F1 | CSR key slot                              |
 *
 * @warning OID 0xE0E1 was accidentally locked with Change:NEV on 2026-01-30.
 *          Use 0xE0E2 for device certificate storage instead.
 *          See incident documentation in README files.
 */
#define TESAIOT_OID_TRUSTM_UID          0xE0C2
#define TESAIOT_OID_FACTORY_CERT        0xE0E0
#define TESAIOT_OID_DEVICE_CERT         0xE0E2  /* Changed from 0xE0E1 (locked) */
#define TESAIOT_OID_DEVICE_CERT_LOCKED  0xE0E1  /* DO NOT USE - Change:NEV */
#define TESAIOT_OID_FACTORY_CA          0xE0E8
#define TESAIOT_OID_TESAIOT_CA          0xE0E9
#define TESAIOT_OID_FACTORY_KEY         0xE0F0
#define TESAIOT_OID_CSR_KEY             0xE0F1

/**
 * @brief Factory Key OID for trustm_provider (mTLS)
 * Format: "0xE0F0:^" means use key at OID 0xE0F0, '^' indicates OPTIGA engine
 */
#define TESAIOT_FACTORY_KEY_OID         "0xe0f0:^"

/**
 * @brief CA Certificate File Path (for MQTT TLS)
 * The embedded TESAIOT_CA_CERTIFICATE will be written to this file at runtime
 */
#define TESAIOT_CA_CERT_PATH            "/tmp/tesaiot_ca.pem"

/*============================================================================
 * TESAIoT CA Certificate Chain
 *===========================================================================*/

/**
 * @brief TESAIoT Root and Intermediate CA Certificates
 *
 * Used for TLS server verification when connecting to mqtt.tesaiot.com
 * This chain should be stored in Trust M OID 0xE0E9
 */
#define TESAIOT_CA_CERTIFICATE \
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

/*============================================================================
 * Infineon Factory Trust Anchor
 *===========================================================================*/

/**
 * @brief Infineon OPTIGA Trust M Factory CA Certificate Chain
 *
 * This CA signed the factory device certificate in OID 0xE0E0
 * Store in OID 0xE0E8 before first boot
 */
#define TESAIOT_INFINEON_FACTORY_CA \
"-----BEGIN CERTIFICATE-----\n" \
"MIICvTCCAh+gAwIBAgIEFwlfzjAKBggqhkjOPQQDBDB5MQswCQYDVQQGEwJERTEh\n" \
"MB8GA1UECgwYSW5maW5lb24gVGVjaG5vbG9naWVzIEFHMRswGQYDVQQLDBJPUFRJ\n" \
"R0EoVE0pIERldmljZXMxKjAoBgNVBAMMIUluZmluZW9uIE9QVElHQShUTSkgRUND\n" \
"IFJvb3QgQ0EgMjAgFw0yMDA4MDcwOTE4MTJaGA8yMDUwMDgwNzA5MTgxMlowcjEL\n" \
"MAkGA1UEBhMCREUxITAfBgNVBAoMGEluZmluZW9uIFRlY2hub2xvZ2llcyBBRzET\n" \
"MBEGA1UECwwKT1BUSUdBKFRNKTErMCkGA1UEAwwiSW5maW5lb24gT1BUSUdBKFRN\n" \
"KSBUcnVzdCBNIENBIDMwMDB2MBAGByqGSM49AgEGBSuBBAAiA2IABIwtFWh1wQ9t\n" \
"2kjQfxUIpi4Mgsp3/EKFVaSBN4xZtG4HKecZWq1dU2luNoKDqjZ3ZQQuxZSS1EXm\n" \
"iXKGjoMGT+IyaW5BUIsPSU7KPr3C1NikAKPKvks6SGAPQaV+BbmMDqN9MHswHQYD\n" \
"VR0OBBYEFLOD4axWlAZZr9ivVyF4RXSODEmZMA4GA1UdDwEB/wQEAwIABDASBgNV\n" \
"HRMBAf8ECDAGAQH/AgEAMBUGA1UdIAQOMAwwCgYIKoIUAEQBFAEwHwYDVR0jBBgw\n" \
"FoAUgrg9zHG4Pn72nNYdyE1SMnBsx50wCgYIKoZIzj0EAwQDgYsAMIGHAkEq7C5I\n" \
"nNeDi4uG2/nonJeJ90oeZMmD3dPJiwdQIkIZbKoC2wBF/09sGMs9v5DVcpujatyN\n" \
"y3y+but2jOewwN5Q7QJCAI/M5Rj8wb5/9Mlql5JXuO8K+10I5lIuZlVGaB5SRQMJ\n" \
"YfnRVef8AgdYpu2+R+v6natnDFSfBsgNlq9vLXZU7vZZ\n" \
"-----END CERTIFICATE-----\n" \
"-----BEGIN CERTIFICATE-----\n" \
"MIICrTCCAg6gAwIBAgIBWjAKBggqhkjOPQQDBDB5MQswCQYDVQQGEwJERTEhMB8G\n" \
"A1UECgwYSW5maW5lb24gVGVjaG5vbG9naWVzIEFHMRswGQYDVQQLDBJPUFRJR0Eo\n" \
"VE0pIERldmljZXMxKjAoBgNVBAMMIUluZmluZW9uIE9QVElHQShUTSkgRUNDIFJv\n" \
"b3QgQ0EgMjAgFw0xOTExMjIwMDAwMDBaGA8yMDU0MTEyMjIzNTk1OVoweTELMAkG\n" \
"A1UEBhMCREUxITAfBgNVBAoMGEluZmluZW9uIFRlY2hub2xvZ2llcyBBRzEbMBkG\n" \
"A1UECwwST1BUSUdBKFRNKSBEZXZpY2VzMSowKAYDVQQDDCFJbmZpbmVvbiBPUFRJ\n" \
"R0EoVE0pIEVDQyBSb290IENBIDIwgZswEAYHKoZIzj0CAQYFK4EEACMDgYYABABD\n" \
"6MnFaakBVM/vveSWg55BTIdxWdxAzGf2+fEUo5b9hMF6kVSWaR0wAAm2p9qeXNAV\n" \
"j7tfQkhz1CxvNz4TauSBQQGf94WLcIKyh7d6zC6/AIloqPizTIGb5xl4ogqyz6ZC\n" \
"T/D5FiOPA98TYzoThdqM8cpcI74e2xOyNgAffsm/BRiuFKNCMEAwHQYDVR0OBBYE\n" \
"FIK4PcxxuD5+9pzWHchNUjJwbMedMA4GA1UdDwEB/wQEAwIABjAPBgNVHRMBAf8E\n" \
"BTADAQH/MAoGCCqGSM49BAMEA4GMADCBiAJCAOXJYwRt86BtRKSuiN5LNATNX6Nc\n" \
"hs4DUiggpQhbgggV3Lf+T39l71KvCIPb8n5ZjSi5AKflmPGzumCjqDAPsgmsAkIA\n" \
"vpqNqptg4Sf3hrdAsLAqNPZGnx8gRBnsTvvQzNUOZETuBp+nbmSrKMWZpd5G7HkM\n" \
"9uXFb5ctX1cZQUbYFA2qG5g=\n" \
"-----END CERTIFICATE-----\n"

/*============================================================================
 * Debug Configuration
 *===========================================================================*/

/**
 * @brief Debug Level
 * 0 = NONE (no debug output)
 * 1 = ERROR (errors only)
 * 2 = WARNING (errors + warnings)
 * 3 = INFO (errors + warnings + info)
 * 4 = VERBOSE (all debug messages)
 */
#ifndef TESAIOT_DEBUG_LEVEL
#define TESAIOT_DEBUG_LEVEL             3  /* INFO level */
#endif

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_CONFIG_H */
