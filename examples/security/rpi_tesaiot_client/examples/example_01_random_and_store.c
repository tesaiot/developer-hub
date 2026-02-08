/**
 * @file example_01_random_and_store.c
 * @brief Example: Hardware TRNG + Secure Credential Storage
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_random_generate()     — Hardware True Random Number Generation
 *   - tesaiot_secure_store_write()  — Store credentials in OPTIGA hardware
 *   - tesaiot_secure_store_read()   — Read credentials back
 *
 * OIDs used:
 *   0xF1D0 (slot 0) — WiFi credentials
 *   0xF1D1 (slot 1) — MQTT broker credentials
 *   0xF1D2 (slot 2) — API tokens
 *   0xF1E0 (slot 8) — Large JSON config (1500 bytes)
 *
 * Application Patterns:
 *   - Smart Home: Store WiFi + MQTT credentials securely
 *   - Industrial: Store API tokens + config in tamper-proof storage
 *   - Medical IoT: Store calibration data in hardware-protected slots
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_optiga.h"
#include "tesaiot_crypto.h"

/*---------------------------------------------------------------------------
 * Use Case A1: WiFi Credential Storage (OID 0xF1D0)
 *
 * Store WiFi SSID + password in OPTIGA hardware instead of plaintext
 * on SD card. Even if the SD card is cloned, credentials are safe.
 *---------------------------------------------------------------------------*/
static int demo_wifi_credential_storage(void)
{
    printf("\n=== A1: WiFi Credential Storage (Slot 0 = OID 0xF1D0) ===\n");

    /* Pack WiFi credentials: [1 byte ssid_len][ssid][password] */
    const char *ssid = "TESAIoT-Lab-5G";
    const char *password = "SecureP@ss2026!";

    uint8_t cred_buf[140]; /* Max 140 bytes for slot 0-7 */
    uint8_t ssid_len = (uint8_t)strlen(ssid);
    size_t offset = 0;

    cred_buf[offset++] = ssid_len;
    memcpy(&cred_buf[offset], ssid, ssid_len);
    offset += ssid_len;
    memcpy(&cred_buf[offset], password, strlen(password));
    offset += strlen(password);

    /* Write to slot 0 (OID 0xF1D0) */
    int rc = tesaiot_secure_store_write(0, cred_buf, (uint16_t)offset);
    if (rc != 0) {
        printf("  ERROR: Failed to store WiFi credentials (rc=%d)\n", rc);
        return rc;
    }
    printf("  Stored WiFi credentials (%zu bytes) in hardware slot 0\n", offset);

    /* Read back and verify */
    uint8_t read_buf[140];
    uint16_t read_len = sizeof(read_buf);

    rc = tesaiot_secure_store_read(0, read_buf, &read_len);
    if (rc != 0) {
        printf("  ERROR: Failed to read WiFi credentials (rc=%d)\n", rc);
        return rc;
    }

    if (read_len == offset && memcmp(cred_buf, read_buf, offset) == 0) {
        printf("  Verified: credentials read back correctly (%u bytes)\n", read_len);
    } else {
        printf("  ERROR: credential mismatch!\n");
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case A2: MQTT Broker Credentials (OID 0xF1D1)
 *
 * Pattern: Write-Once Vault
 * Store MQTT credentials in hardware. In production, set Read=NEVER
 * metadata so credentials can only be used internally by OPTIGA.
 *---------------------------------------------------------------------------*/
static int demo_mqtt_credential_storage(void)
{
    printf("\n=== A2: MQTT Broker Credentials (Slot 1 = OID 0xF1D1) ===\n");

    /* MQTT credentials: username:password */
    const char *mqtt_cred = "device-001:MqTT$ecret!2026";

    int rc = tesaiot_secure_store_write(1, (const uint8_t *)mqtt_cred,
                                        (uint16_t)strlen(mqtt_cred));
    if (rc != 0) {
        printf("  ERROR: Failed to store MQTT credentials (rc=%d)\n", rc);
        return rc;
    }
    printf("  Stored MQTT credentials (%zu bytes) in hardware slot 1\n",
           strlen(mqtt_cred));

    /* In production: lock slot with metadata Read=NEVER */
    /* This makes it a write-only vault — credentials cannot be extracted */
    printf("  TIP: In production, lock with: tesaiot_secure_store_lock(1)\n");

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case A3: Application Configuration (OID 0xF1E0, 1500 bytes)
 *
 * Store a JSON config blob in a large data object.
 * Protected from tampering by OPTIGA hardware access control.
 *---------------------------------------------------------------------------*/
static int demo_app_config_storage(void)
{
    printf("\n=== A3: Application Config (Slot 8 = OID 0xF1E0, 1500 bytes) ===\n");

    const char *config_json =
        "{"
        "\"mqtt_broker\":\"mqtt.tesaiot.com\","
        "\"mqtt_port\":8883,"
        "\"report_interval_sec\":60,"
        "\"sensor_thresholds\":{\"temp_max\":45.0,\"humidity_min\":20.0},"
        "\"firmware_update_url\":\"https://ota.tesaiot.com/v2\","
        "\"feature_flags\":{\"data_encryption\":true,\"local_logging\":false}"
        "}";

    int rc = tesaiot_secure_store_write(8, (const uint8_t *)config_json,
                                        (uint16_t)strlen(config_json));
    if (rc != 0) {
        printf("  ERROR: Failed to store config (rc=%d)\n", rc);
        return rc;
    }
    printf("  Stored config JSON (%zu bytes) in hardware slot 8\n",
           strlen(config_json));

    /* Read back */
    uint8_t read_buf[1500];
    uint16_t read_len = sizeof(read_buf);

    rc = tesaiot_secure_store_read(8, read_buf, &read_len);
    if (rc != 0) {
        printf("  ERROR: Failed to read config (rc=%d)\n", rc);
        return rc;
    }
    printf("  Read back: %.*s\n", read_len, read_buf);

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case: Hardware TRNG for Nonce/Session ID Generation
 *---------------------------------------------------------------------------*/
static int demo_random_generation(void)
{
    printf("\n=== Hardware TRNG: Random Nonce Generation ===\n");

    /* Generate 32-byte nonce for challenge-response */
    uint8_t nonce[32];
    int rc = tesaiot_random_generate(nonce, 32);
    if (rc != 0) {
        printf("  ERROR: TRNG failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Nonce (32 bytes): ");
    for (int i = 0; i < 32; i++) printf("%02X", nonce[i]);
    printf("\n");

    /* Generate 16-byte session ID */
    uint8_t session_id[16];
    rc = tesaiot_random_generate(session_id, 16);
    if (rc != 0) {
        printf("  ERROR: TRNG failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Session ID (16 bytes): ");
    for (int i = 0; i < 16; i++) printf("%02X", session_id[i]);
    printf("\n");

    /* Demonstrate: two random outputs always differ */
    uint8_t rand_a[32], rand_b[32];
    tesaiot_random_generate(rand_a, 32);
    tesaiot_random_generate(rand_b, 32);

    if (memcmp(rand_a, rand_b, 32) != 0) {
        printf("  Quality check: Two 32-byte outputs differ (PASS)\n");
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_01_random_and_store(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 01: Hardware TRNG + Secure Credential Storage\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc;

    /* Initialize library */
    rc = tesaiot_init();
    if (rc != 0) {
        printf("ERROR: tesaiot_init() failed (rc=%d)\n", rc);
        return rc;
    }

    /* Verify license */
    rc = tesaiot_verify_license();
    if (rc != 0) {
        printf("ERROR: License verification failed (rc=%d)\n", rc);
        tesaiot_deinit();
        return rc;
    }

    /* Run demos */
    demo_random_generation();
    demo_wifi_credential_storage();
    demo_mqtt_credential_storage();
    demo_app_config_storage();

    /* Cleanup */
    tesaiot_deinit();

    printf("\n=== Example 01 Complete ===\n\n");
    return 0;
}

int main(void) { return example_01_random_and_store(); }
