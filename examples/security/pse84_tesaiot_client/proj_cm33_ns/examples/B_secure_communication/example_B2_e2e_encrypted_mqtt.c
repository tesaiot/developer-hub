/**
 * @file example_B2_e2e_encrypted_mqtt.c
 * @brief Example: End-to-end encrypted MQTT using Encrypt-then-MAC
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category B: Secure Communication
 *
 * Encrypt-then-MAC pattern:
 * 1. AES-CBC encrypt payload (IV auto-generated)
 * 2. HMAC-SHA256 over ciphertext
 * 3. Output: IV (16) || ciphertext || HMAC (32)
 *
 * Even if MQTT broker is compromised, payload can't be read.
 *
 * OIDs used: 0xE200 (AES key), 0xF1D5 (HMAC key)
 *
 * Required: Phase 1 functions (tesaiot_aes_*, tesaiot_hmac_sha256)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_B2_e2e_encrypted_mqtt(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example B2: End-to-End Encrypted MQTT\n");
    printf("  OID Category: B - Secure Communication\n");
    printf("  Pattern: Encrypt-then-MAC\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Generate AES key for encryption --- */
    printf("[Step 1] Generate AES-256 key in OPTIGA...\n");
    rc = tesaiot_aes_generate_key(256);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: AES-256 key ready in hardware\n\n");

    /* --- Step 2: Store HMAC key for MAC --- */
    printf("[Step 2] Generate and store HMAC key in Slot 5...\n");
    uint8_t hmac_key[32];
    rc = tesaiot_random_generate(hmac_key, 32);
    if (rc != TESAIOT_OK) { printf("  FAIL\n"); return; }
    rc = tesaiot_secure_store_write(5, hmac_key, 32);
    if (rc != TESAIOT_OK) { printf("  FAIL\n"); return; }
    printf("  OK\n\n");

    /* --- Step 3: Create and pad sensor payload --- */
    printf("[Step 3] Create sensor payload...\n");
    const char *sensor_json = "{\"temp\":25.3,\"hum\":60.2}";
    uint8_t plaintext[32]; /* padded to 32 bytes (2 AES blocks) */
    memset(plaintext, 0, sizeof(plaintext));
    memcpy(plaintext, sensor_json, strlen(sensor_json));
    uint16_t plain_len = 32;
    printf("  Payload: %s\n", sensor_json);
    printf("  Padded: %u bytes\n\n", plain_len);

    /* --- Step 4: Encrypt (IV auto-generated) --- */
    printf("[Step 4] AES-CBC Encrypt (IV from TRNG)...\n");
    uint8_t iv[16];
    uint8_t ciphertext[32];
    uint16_t cipher_len = sizeof(ciphertext);

    rc = tesaiot_aes_encrypt(plaintext, plain_len, NULL, iv,
                             ciphertext, &cipher_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: Encrypted %u bytes\n\n", cipher_len);

    /* --- Step 5: HMAC over ciphertext (Encrypt-then-MAC) --- */
    printf("[Step 5] HMAC-SHA256 over ciphertext (Encrypt-then-MAC)...\n");
    uint8_t mac[32];
    uint16_t mac_len = sizeof(mac);
    rc = tesaiot_hmac_sha256(5, ciphertext, cipher_len, mac, &mac_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  OK: MAC computed\n\n");

    /* --- Step 6: Show wire format --- */
    printf("[Step 6] Wire Format: IV || Ciphertext || HMAC\n");
    printf("  +------------------+\n");
    printf("  | IV (16 bytes)    | ");
    for (int i = 0; i < 8; i++) printf("%02X", iv[i]);
    printf("...\n");
    printf("  +------------------+\n");
    printf("  | Ciphertext       | ");
    for (int i = 0; i < 8; i++) printf("%02X", ciphertext[i]);
    printf("...\n");
    printf("  | (%u bytes)       |\n", cipher_len);
    printf("  +------------------+\n");
    printf("  | HMAC (32 bytes)  | ");
    for (int i = 0; i < 8; i++) printf("%02X", mac[i]);
    printf("...\n");
    printf("  +------------------+\n");
    printf("  Total: %u bytes\n\n", 16 + cipher_len + 32);

    /* --- Step 7: Receiver side: Verify MAC then Decrypt --- */
    printf("[Step 7] Receiver: Verify MAC -> Decrypt...\n");

    /* Verify MAC first */
    uint8_t mac_check[32];
    uint16_t mac_check_len = sizeof(mac_check);
    rc = tesaiot_hmac_sha256(5, ciphertext, cipher_len, mac_check, &mac_check_len);
    if (rc != TESAIOT_OK || memcmp(mac, mac_check, 32) != 0) {
        printf("  MAC FAILED - reject message (tampered!)\n");
        return;
    }
    printf("  MAC verified OK\n");

    /* Then decrypt */
    uint8_t decrypted[32];
    uint16_t dec_len = sizeof(decrypted);
    rc = tesaiot_aes_decrypt(ciphertext, cipher_len, iv, decrypted, &dec_len);
    if (rc != TESAIOT_OK) {
        printf("  Decrypt FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  Decrypted: %.*s\n", (int)strlen(sensor_json), (char *)decrypted);
    printf("  Round-trip: %s\n\n",
           memcmp(plaintext, decrypted, plain_len) == 0 ? "MATCH" : "MISMATCH");

    printf("================================================================\n");
    printf("  Example B2 Complete\n");
    printf("  Even if MQTT broker is compromised, data is E2E encrypted.\n");
    printf("================================================================\n\n");
}
