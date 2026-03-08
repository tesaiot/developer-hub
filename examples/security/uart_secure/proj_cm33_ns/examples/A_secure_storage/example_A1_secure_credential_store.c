/**
 * @file example_A1_secure_credential_store.c
 * @brief Example: Store and retrieve credentials in OPTIGA hardware vault
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category A: Secure Storage
 *
 * This example shows how to:
 * - Generate random nonces using hardware TRNG
 * - Store WiFi credentials (SSID + password) in OPTIGA slot 0
 * - Store API tokens in OPTIGA slot 2
 * - Store large JSON config in OPTIGA slot 12 (1500 bytes)
 * - Read back and verify stored data
 * - Handle reserved slot (slot 4) rejection
 * - Print slot mapping table
 *
 * OIDs used: 0xF1D0 (slot 0), 0xF1D2 (slot 2), 0xF1E0 (slot 12)
 *
 * Required: Phase 1 functions (tesaiot_random_generate, tesaiot_secure_store_*)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_A1_secure_credential_store(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example A1: Secure Credential Store\n");
    printf("  OID Category: A - Secure Storage\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Generate random nonce --- */
    printf("[Step 1] Generate 32-byte random nonce from OPTIGA TRNG...\n");
    uint8_t nonce[32];
    rc = tesaiot_random_generate(nonce, 32);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Nonce = ");
    for (int i = 0; i < 8; i++) printf("%02X", nonce[i]);
    printf("...\n\n");

    /* --- Step 2: Store WiFi credentials in slot 0 --- */
    printf("[Step 2] Store WiFi credentials in Slot 0 (OID 0xF1D0)...\n");
    const char *wifi_cred = "SSID:MyNetwork;PASS:SecureP@ss123";
    rc = tesaiot_secure_store_write(0, (const uint8_t *)wifi_cred,
                                    (uint16_t)strlen(wifi_cred));
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Stored %zu bytes\n\n", strlen(wifi_cred));

    /* --- Step 3: Store API token in slot 2 --- */
    printf("[Step 3] Store API token in Slot 2 (OID 0xF1D2)...\n");
    const char *api_token = "Bearer eyJhbGciOiJIUzI1NiJ9.TESAIoT";
    rc = tesaiot_secure_store_write(2, (const uint8_t *)api_token,
                                    (uint16_t)strlen(api_token));
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Stored %zu bytes\n\n", strlen(api_token));

    /* --- Step 4: Verify slot 4 is rejected --- */
    printf("[Step 4] Attempt to write to Slot 4 (RESERVED for Protected Update)...\n");
    rc = tesaiot_secure_store_write(4, (const uint8_t *)"test", 4);
    if (rc == TESAIOT_ERROR_RESERVED_OID) {
        printf("  OK: Correctly rejected - %s\n\n", tesaiot_error_str((tesaiot_error_t)rc));
    } else {
        printf("  UNEXPECTED: Should have returned RESERVED_OID error\n\n");
    }

    /* --- Step 5: Read back WiFi credentials --- */
    printf("[Step 5] Read back WiFi credentials from Slot 0...\n");
    uint8_t read_buf[256];
    uint16_t read_len = sizeof(read_buf);
    rc = tesaiot_secure_store_read(0, read_buf, &read_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    read_buf[read_len] = '\0';
    printf("  OK: Read %u bytes: \"%s\"\n", read_len, (char *)read_buf);

    /* Verify match */
    if (read_len == strlen(wifi_cred) &&
        memcmp(read_buf, wifi_cred, read_len) == 0) {
        printf("  VERIFY: Data matches original!\n\n");
    } else {
        printf("  VERIFY: DATA MISMATCH!\n\n");
    }

    /* --- Step 6: Print slot mapping table --- */
    printf("[Step 6] Secure Store Slot Mapping:\n");
    printf("  +------+--------+----------+-------------+\n");
    printf("  | Slot |  OID   | Max Size | Status      |\n");
    printf("  +------+--------+----------+-------------+\n");
    printf("  |  0   | 0xF1D0 |  140 B   | User Data   |\n");
    printf("  |  1   | 0xF1D1 |  140 B   | User Data   |\n");
    printf("  |  2   | 0xF1D2 |  140 B   | User Data   |\n");
    printf("  |  3   | 0xF1D3 |  140 B   | User Data   |\n");
    printf("  |  4   | 0xF1D4 |    -     | RESERVED(PU)|\n");
    printf("  | 5-11 | 0xF1D5 |  140 B   | User Data   |\n");
    printf("  |      | -F1DB  |          |             |\n");
    printf("  |  12  | 0xF1E0 | 1500 B   | Large Data  |\n");
    printf("  |  13  | 0xF1E1 | 1500 B   | Large Data  |\n");
    printf("  +------+--------+----------+-------------+\n\n");

    printf("================================================================\n");
    printf("  Example A1 Complete\n");
    printf("================================================================\n\n");
}
