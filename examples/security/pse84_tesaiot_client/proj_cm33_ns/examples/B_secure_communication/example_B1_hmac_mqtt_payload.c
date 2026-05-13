/**
 * @file example_B1_hmac_mqtt_payload.c
 * @brief Example: HMAC-SHA256 signed MQTT payload
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category B: Secure Communication
 *
 * This example shows how to:
 * - Store HMAC key in OPTIGA hardware slot
 * - Create MQTT telemetry payload
 * - Compute HMAC-SHA256 of payload using hardware key
 * - Format "payload + MAC" for authenticated MQTT publish
 *
 * OIDs used: 0xF1D5 (slot 5 - HMAC key storage)
 *
 * Required: Phase 1 functions (tesaiot_hmac_sha256, tesaiot_secure_store_write)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_B1_hmac_mqtt_payload(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example B1: HMAC-SHA256 Signed MQTT Payload\n");
    printf("  OID Category: B - Secure Communication\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Generate and store HMAC key --- */
    printf("[Step 1] Generate 32-byte HMAC key and store in Slot 5 (OID 0xF1D5)...\n");
    uint8_t hmac_key[32];
    rc = tesaiot_random_generate(hmac_key, 32);
    if (rc != TESAIOT_OK) {
        printf("  FAIL generating key: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }

    rc = tesaiot_secure_store_write(5, hmac_key, 32);
    if (rc != TESAIOT_OK) {
        printf("  FAIL storing key: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: HMAC key stored in OPTIGA (never leaves hardware)\n\n");

    /* --- Step 2: Create MQTT payload --- */
    printf("[Step 2] Create sensor telemetry payload...\n");
    const char *payload = "{\"temp\":25.3,\"humidity\":60.2,\"ts\":1738972800}";
    printf("  Payload: %s\n", payload);
    printf("  Length: %zu bytes\n\n", strlen(payload));

    /* --- Step 3: Compute HMAC-SHA256 --- */
    printf("[Step 3] Compute HMAC-SHA256 using hardware key...\n");
    uint8_t mac[32];
    uint16_t mac_len = sizeof(mac);
    rc = tesaiot_hmac_sha256(5, (const uint8_t *)payload,
                             (uint16_t)strlen(payload), mac, &mac_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: HMAC-SHA256 = ");
    for (int i = 0; i < 32; i++) printf("%02X", mac[i]);
    printf("\n\n");

    /* --- Step 4: Format for MQTT publish --- */
    printf("[Step 4] MQTT Publish Format:\n");
    printf("  Topic: device/{uid}/telemetry/sensor\n");
    printf("  Payload (authenticated):\n");
    printf("  {\n");
    printf("    \"data\": %s,\n", payload);
    printf("    \"mac\": \"");
    for (int i = 0; i < 32; i++) printf("%02x", mac[i]);
    printf("\"\n");
    printf("  }\n\n");

    /* --- Step 5: Verify HMAC (same data + same key = same MAC) --- */
    printf("[Step 5] Verify: Recompute HMAC and compare...\n");
    uint8_t mac_verify[32];
    uint16_t mac_verify_len = sizeof(mac_verify);
    rc = tesaiot_hmac_sha256(5, (const uint8_t *)payload,
                             (uint16_t)strlen(payload), mac_verify, &mac_verify_len);
    if (rc == TESAIOT_OK && memcmp(mac, mac_verify, 32) == 0) {
        printf("  OK: HMAC verification passed (deterministic)\n\n");
    } else {
        printf("  FAIL: HMAC verification failed\n\n");
    }

    printf("  Receiver side: Compute HMAC with shared key,\n");
    printf("  compare with received MAC -> Accept or Reject\n\n");

    printf("================================================================\n");
    printf("  Example B1 Complete\n");
    printf("================================================================\n\n");
}
