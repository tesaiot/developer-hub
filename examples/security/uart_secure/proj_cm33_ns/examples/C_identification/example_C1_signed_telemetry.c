/**
 * @file example_C1_signed_telemetry.c
 * @brief Example: Non-repudiation signed sensor telemetry
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category C: Identification & Attestation
 *
 * Pattern: SHA-256 hash + ECDSA sign for non-repudiation
 * - Server can verify data came from THIS specific device
 * - Cannot be forged (private key never leaves OPTIGA)
 * - Legal evidence quality for medical/industrial IoT
 *
 * OIDs used: 0xE0F1 (Device Key for signing), 0xE0E1 (Device Certificate)
 *
 * Required: Phase 2 functions (tesaiot_optiga_hash, tesaiot_sign_data)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_C1_signed_telemetry(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example C1: Signed Telemetry (Non-Repudiation)\n");
    printf("  OID Category: C - Identification & Attestation\n");
    printf("  Pattern: SHA-256 + ECDSA Digital Signature\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Create sensor telemetry payload --- */
    printf("[Step 1] Create sensor telemetry data...\n");
    const char *telemetry = "{\"temp\":25.3,\"hum\":60.2,"
                            "\"pressure\":1013.25,"
                            "\"ts\":1738972800,"
                            "\"seq\":42}";
    printf("  Payload: %s\n", telemetry);
    printf("  Length: %zu bytes\n\n", strlen(telemetry));

    /* --- Step 2: Hash the telemetry data (SHA-256) --- */
    printf("[Step 2] SHA-256 hash of telemetry data...\n");
    uint8_t hash[32];
    uint16_t hash_len = sizeof(hash);
    rc = tesaiot_optiga_hash((const uint8_t *)telemetry,
                              (uint16_t)strlen(telemetry),
                              hash, &hash_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  SHA-256 = ");
    for (int i = 0; i < 32; i++) printf("%02X", hash[i]);
    printf("\n\n");

    /* --- Step 3: Sign hash with device private key (ECDSA P-256) --- */
    printf("[Step 3] ECDSA sign with Device Key (OID 0xE0F1)...\n");
    printf("  Private key NEVER leaves OPTIGA hardware\n");
    uint8_t signature[80]; /* DER-encoded ECDSA signature */
    uint16_t sig_len = sizeof(signature);

    rc = tesaiot_sign_data(0xE0F1,
                            (const uint8_t *)telemetry,
                            (uint16_t)strlen(telemetry),
                            signature, &sig_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        printf("  Note: Requires Device Key in OID 0xE0F1\n");
        printf("  (Run CSR workflow first to generate the key)\n\n");
        return;
    }
    printf("  OK: Signature (%u bytes): ", sig_len);
    for (int i = 0; i < 16; i++) printf("%02X", signature[i]);
    printf("...\n\n");

    /* --- Step 4: Format signed MQTT message --- */
    printf("[Step 4] Signed MQTT Message Format:\n");
    printf("  Topic: device/{uid}/telemetry/signed\n");
    printf("  {\n");
    printf("    \"data\": %s,\n", telemetry);
    printf("    \"hash\": \"");
    for (int i = 0; i < 32; i++) printf("%02x", hash[i]);
    printf("\",\n");
    printf("    \"sig\": \"");
    for (int i = 0; i < sig_len && i < 16; i++) printf("%02x", signature[i]);
    printf("...\",\n");
    printf("    \"sig_alg\": \"ECDSA-P256-SHA256\",\n");
    printf("    \"cert_oid\": \"0xE0E1\"\n");
    printf("  }\n\n");

    /* --- Step 5: Explain verification --- */
    printf("[Step 5] Server-Side Verification:\n\n");
    printf("  1. Extract data + signature from MQTT message\n");
    printf("  2. Get device's public key from certificate (0xE0E1)\n");
    printf("  3. SHA-256(data) -> hash'\n");
    printf("  4. ECDSA_Verify(hash', signature, pubkey)\n");
    printf("  5. If valid -> data is authentic from this device\n\n");

    printf("  +------------------------------------------+\n");
    printf("  | NON-REPUDIATION GUARANTEE:               |\n");
    printf("  |                                          |\n");
    printf("  | - Data was produced by THIS device       |\n");
    printf("  | - Data was NOT modified in transit       |\n");
    printf("  | - Device CANNOT deny producing this data |\n");
    printf("  | - Legal evidence quality (CC EAL6+)      |\n");
    printf("  +------------------------------------------+\n\n");

    printf("================================================================\n");
    printf("  Example C1 Complete\n");
    printf("  Use cases: Medical IoT, Industrial Sensors, Smart Metering\n");
    printf("================================================================\n\n");
}
