/**
 * @file example_A2_data_encryption_at_rest.c
 * @brief Example: Encrypt sensor data before storing to flash
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category A: Data Encryption at Rest
 *
 * This example shows how to:
 * - Generate AES-256 key in OPTIGA (key never leaves hardware)
 * - Encrypt sensor data with AES-CBC (auto-generated IV)
 * - Decrypt and verify round-trip
 * - Pattern: "save IV+ciphertext to flash" - even if flash is dumped,
 *   data can't be decrypted without OPTIGA hardware
 *
 * OIDs used: 0xE200 (AES symmetric key - OPTIGA_KEY_ID_SECRET_BASED)
 *
 * Required: Phase 1 functions (tesaiot_aes_*, tesaiot_random_generate)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_A2_data_encryption_at_rest(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example A2: Data Encryption at Rest\n");
    printf("  OID Category: A - Secure Storage (AES-CBC)\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Generate AES-256 key in OPTIGA --- */
    printf("[Step 1] Generate AES-256 key in OPTIGA (OID 0xE200)...\n");
    printf("  Key material NEVER leaves the secure element!\n");
    rc = tesaiot_aes_generate_key(256);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: AES-256 key generated in hardware\n\n");

    /* --- Step 2: Create sensor data (padded to 16-byte boundary) --- */
    printf("[Step 2] Prepare sensor data for encryption...\n");
    /* Sensor data: 48 bytes (3 blocks of 16) */
    const char *sensor_json = "{\"temp\":25.3,\"hum\":60.2,\"ts\":1738972800}";
    uint8_t plaintext[48];
    memset(plaintext, 0, sizeof(plaintext));
    memcpy(plaintext, sensor_json, strlen(sensor_json));
    uint16_t plain_len = 48; /* padded to 16-byte boundary */
    printf("  Original: %s\n", sensor_json);
    printf("  Padded length: %u bytes (%u blocks)\n\n", plain_len, plain_len / 16);

    /* --- Step 3: Encrypt with auto-generated IV --- */
    printf("[Step 3] Encrypt with AES-CBC (IV auto-generated from TRNG)...\n");
    uint8_t iv_out[16];
    uint8_t ciphertext[48];
    uint16_t cipher_len = sizeof(ciphertext);

    rc = tesaiot_aes_encrypt(plaintext, plain_len, NULL, iv_out,
                             ciphertext, &cipher_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Encrypted %u bytes\n", cipher_len);
    printf("  IV:  ");
    for (int i = 0; i < 16; i++) printf("%02X", iv_out[i]);
    printf("\n");
    printf("  CT:  ");
    for (int i = 0; i < 16; i++) printf("%02X", ciphertext[i]);
    printf("...\n\n");

    /* --- Step 4: Show storage pattern --- */
    printf("[Step 4] Flash Storage Pattern:\n");
    printf("  +------------------+\n");
    printf("  | IV (16 bytes)    |  <-- Store in flash\n");
    printf("  +------------------+\n");
    printf("  | Ciphertext       |  <-- Store in flash\n");
    printf("  | (%u bytes)       |\n", cipher_len);
    printf("  +------------------+\n");
    printf("  | AES Key          |  <-- STAYS IN OPTIGA (0xE200)\n");
    printf("  | (never exported) |  <-- Can't be read even with physical access\n");
    printf("  +------------------+\n\n");

    /* --- Step 5: Decrypt and verify --- */
    printf("[Step 5] Decrypt (simulating read from flash)...\n");
    uint8_t decrypted[48];
    uint16_t dec_len = sizeof(decrypted);

    rc = tesaiot_aes_decrypt(ciphertext, cipher_len, iv_out,
                             decrypted, &dec_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Decrypted %u bytes\n", dec_len);
    printf("  Result: %.*s\n", (int)strlen(sensor_json), (char *)decrypted);

    /* Verify round-trip */
    if (memcmp(plaintext, decrypted, plain_len) == 0) {
        printf("  VERIFY: Round-trip successful - data matches!\n\n");
    } else {
        printf("  VERIFY: DATA MISMATCH!\n\n");
    }

    printf("================================================================\n");
    printf("  Example A2 Complete\n");
    printf("  Key insight: Even if flash is physically dumped,\n");
    printf("  data can't be decrypted without THIS specific OPTIGA chip.\n");
    printf("================================================================\n\n");
}
