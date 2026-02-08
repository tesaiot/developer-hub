/**
 * @file example_02_aes_encrypt_sensor.c
 * @brief Example: AES Encryption of Sensor Data at Rest
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_aes_generate_key()  — Generate AES key inside OPTIGA
 *   - tesaiot_aes_encrypt()       — Encrypt data (key never leaves hardware)
 *   - tesaiot_aes_decrypt()       — Decrypt data
 *   - tesaiot_random_generate()   — Generate random IV
 *
 * OIDs used:
 *   0xE200 — AES symmetric key (session context, internal)
 *
 * Application Patterns:
 *   - Smart Home: Encrypt sensor logs before writing to SD card
 *   - Industrial: Protect PLC data at rest on edge gateway
 *   - Medical IoT: Encrypt patient vitals stored locally (HIPAA compliance)
 *
 * Use Case D5: Data Encryption at Rest
 *   Key stays in OPTIGA hardware. Even if flash/SD card is cloned,
 *   data cannot be decrypted without the original OPTIGA chip.
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_crypto.h"

/*---------------------------------------------------------------------------
 * Use Case D5: Encrypt Sensor Data Before Storing to Flash/SD
 *
 * Flow:
 *   1. Generate AES-256 key in OPTIGA (once, persists across power cycles)
 *   2. For each sensor reading:
 *      a. Generate random IV (16 bytes)
 *      b. Encrypt sensor data with AES-CBC
 *      c. Store [IV || ciphertext] to flash/SD card
 *   3. To read back:
 *      a. Read [IV || ciphertext] from flash
 *      b. Decrypt with same AES key in OPTIGA
 *---------------------------------------------------------------------------*/
static int demo_encrypt_sensor_data(void)
{
    printf("\n=== D5: Encrypt Sensor Data at Rest ===\n");

    /* Step 1: Generate AES-256 key (do this once during provisioning) */
    int rc = tesaiot_aes_generate_key(256);
    if (rc != 0) {
        printf("  ERROR: AES key generation failed (rc=%d)\n", rc);
        return rc;
    }
    printf("  AES-256 key generated in OPTIGA OID 0xE200\n");

    /* Step 2: Simulate sensor data */
    const char *sensor_json =
        "{\"temp\":25.4,\"humidity\":62.1,\"pressure\":1013.2,\"ts\":1706745600}";
    uint16_t data_len = (uint16_t)strlen(sensor_json);

    printf("  Plaintext (%u bytes): %s\n", data_len, sensor_json);

    /* Step 3: Encrypt with auto-generated IV */
    uint8_t iv_out[16];
    uint8_t ciphertext[256];
    uint16_t cipher_len = sizeof(ciphertext);

    rc = tesaiot_aes_encrypt((const uint8_t *)sensor_json, data_len,
                             NULL, /* NULL = auto-generate IV via TRNG */
                             iv_out, ciphertext, &cipher_len);
    if (rc != 0) {
        printf("  ERROR: Encryption failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Encrypted (%u bytes)\n", cipher_len);
    printf("  IV: ");
    for (int i = 0; i < 16; i++) printf("%02X", iv_out[i]);
    printf("\n");

    /*
     * In production, save to flash/SD card:
     *   fwrite(iv_out, 1, 16, fp);          // Save IV
     *   fwrite(ciphertext, 1, cipher_len, fp); // Save ciphertext
     *
     * Even if SD card is cloned, cannot decrypt without OPTIGA chip
     */

    /* Step 4: Decrypt (simulate reading from flash) */
    uint8_t decrypted[256];
    uint16_t decrypted_len = sizeof(decrypted);

    rc = tesaiot_aes_decrypt(ciphertext, cipher_len,
                             iv_out, decrypted, &decrypted_len);
    if (rc != 0) {
        printf("  ERROR: Decryption failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Decrypted (%u bytes): %.*s\n", decrypted_len,
           decrypted_len, decrypted);

    /* Verify roundtrip */
    if (decrypted_len == data_len &&
        memcmp(sensor_json, decrypted, data_len) == 0) {
        printf("  Roundtrip verification: PASS\n");
    } else {
        printf("  ERROR: Roundtrip verification FAILED!\n");
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * Pattern: Encrypt Multiple Records with Different IVs
 *
 * Same key, different IV for each record = different ciphertext.
 * This is the correct way to encrypt multiple records.
 *---------------------------------------------------------------------------*/
static int demo_encrypt_multiple_records(void)
{
    printf("\n=== Pattern: Multiple Records with Different IVs ===\n");

    const char *records[] = {
        "{\"temp\":25.4,\"ts\":1000}",
        "{\"temp\":26.1,\"ts\":2000}",
        "{\"temp\":24.8,\"ts\":3000}"
    };

    for (int i = 0; i < 3; i++) {
        uint8_t iv[16], ciphertext[256];
        uint16_t cipher_len = sizeof(ciphertext);

        int rc = tesaiot_aes_encrypt((const uint8_t *)records[i],
                                     (uint16_t)strlen(records[i]),
                                     NULL, iv, ciphertext, &cipher_len);
        if (rc != 0) {
            printf("  Record %d: ENCRYPT FAILED\n", i);
            return rc;
        }
        printf("  Record %d: IV=%02X%02X...  Cipher=%u bytes\n",
               i, iv[0], iv[1], cipher_len);
    }

    printf("  Each record has unique IV -> unique ciphertext\n");
    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_02_aes_encrypt_sensor(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 02: AES Encryption of Sensor Data at Rest\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc = tesaiot_init();
    if (rc != 0) return rc;

    rc = tesaiot_verify_license();
    if (rc != 0) { tesaiot_deinit(); return rc; }

    demo_encrypt_sensor_data();
    demo_encrypt_multiple_records();

    tesaiot_deinit();
    printf("\n=== Example 02 Complete ===\n\n");
    return 0;
}

int main(void) { return example_02_aes_encrypt_sensor(); }
