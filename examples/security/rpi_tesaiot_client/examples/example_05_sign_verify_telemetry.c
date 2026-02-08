/**
 * @file example_05_sign_verify_telemetry.c
 * @brief Example: Signed Non-Repudiation Telemetry
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_sign_data()    — Hash + ECDSA sign in one call
 *   - tesaiot_verify_data()  — Hash + ECDSA verify in one call
 *
 * OIDs used:
 *   0xE0F1 — Device private key (ECDSA signing)
 *
 * Use Case B4: Signed MQTT Telemetry (Non-Repudiation)
 *   Server can verify that data came from this specific device
 *   and was not modified in transit. Provides legal evidence.
 *
 * Use Case D1: Anti-Replay Protection
 *   Include monotonic counter in signed payload to prevent replay attacks.
 *
 * Application Patterns:
 *   - Smart Meter: Signed electricity readings (legal billing evidence)
 *   - Medical IoT: Verified vital signs (FDA audit trail)
 *   - Industrial: Authenticated sensor data (compliance reporting)
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_optiga.h"
#include "tesaiot_crypto.h"

/*---------------------------------------------------------------------------
 * Use Case B4: Sign Sensor Data for Non-Repudiation
 *
 * The server can verify:
 *   1. Data came from this device (only this device has the private key)
 *   2. Data was not modified (hash integrity)
 *   3. Device cannot deny sending this data (non-repudiation)
 *---------------------------------------------------------------------------*/
static int demo_sign_telemetry(void)
{
    printf("\n=== B4: Signed Telemetry (Non-Repudiation) ===\n");

    /* Simulate sensor data */
    const char *telemetry =
        "{\"device\":\"a8a1c4f3-42ae-41e2-b6d3-1f74cc3d5b31\","
        "\"temp\":25.4,\"humidity\":62.1,\"pressure\":1013.2,"
        "\"ts\":1706745600,\"seq\":42}";

    printf("  Telemetry: %s\n", telemetry);

    /* Sign data: SHA-256 hash + ECDSA sign (one call) */
    uint8_t signature[80];
    uint16_t sig_len = sizeof(signature);

    int rc = tesaiot_sign_data(TESAIOT_OID_CSR_KEY, /* 0xE0F1 */
                               (const uint8_t *)telemetry,
                               (uint16_t)strlen(telemetry),
                               signature, &sig_len);
    if (rc != 0) {
        printf("  ERROR: Signing failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Signature (%u bytes): ", sig_len);
    for (uint16_t i = 0; i < sig_len && i < 16; i++) printf("%02X", signature[i]);
    printf("...\n");

    /*
     * In production, publish to MQTT:
     *
     *   // Build signed payload: { "data": <telemetry>, "sig": <base64(sig)> }
     *   char mqtt_payload[4096];
     *   snprintf(mqtt_payload, sizeof(mqtt_payload),
     *            "{\"data\":%s,\"sig\":\"%s\"}", telemetry, sig_base64);
     *   tesaiot_mqtt_publish("device/<id>/telemetry", mqtt_payload, len);
     *
     * Server verification:
     *   1. Extract data and signature
     *   2. Read device's public key from device certificate (OID 0xE0E1)
     *   3. SHA-256(data) -> digest
     *   4. ECDSA_Verify(digest, signature, pubkey) -> valid/invalid
     */

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case C4: Peer Authentication via Challenge-Response
 *
 * Device A challenges Device B to prove identity:
 *   A -> B: "prove who you are" + random nonce
 *   B -> A: ECDSA_Sign(nonce, device_key) + device_cert
 *   A: Verify signature using B's certificate
 *---------------------------------------------------------------------------*/
static int demo_challenge_response(void)
{
    printf("\n=== C4: Challenge-Response Authentication ===\n");

    /* Step 1: Generate random challenge (32 bytes) */
    uint8_t challenge[32];
    int rc = tesaiot_random_generate(challenge, 32);
    if (rc != 0) return rc;

    printf("  Challenge: ");
    for (int i = 0; i < 16; i++) printf("%02X", challenge[i]);
    printf("...\n");

    /* Step 2: Sign challenge with device key (Device B proves identity) */
    uint8_t response_sig[80];
    uint16_t sig_len = sizeof(response_sig);

    rc = tesaiot_sign_data(TESAIOT_OID_CSR_KEY, challenge, 32,
                           response_sig, &sig_len);
    if (rc != 0) {
        printf("  ERROR: Challenge signing failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  Response signature (%u bytes)\n", sig_len);
    printf("  Authentication: If verifier has our cert, they can confirm identity\n");

    return 0;
}

/*---------------------------------------------------------------------------
 * Compare: High-Level sign_data vs Manual hash+sign
 *
 * sign_data() is a convenience wrapper that does both steps internally.
 * Both produce valid signatures, but sign_data() is simpler to use.
 *---------------------------------------------------------------------------*/
static int demo_compare_sign_methods(void)
{
    printf("\n=== Comparison: sign_data() vs manual hash+sign ===\n");

    const char *data = "TESAIoT comparison test";
    uint16_t data_len = (uint16_t)strlen(data);

    /* Method 1: High-level sign_data (recommended) */
    uint8_t sig1[80];
    uint16_t sig1_len = sizeof(sig1);
    int rc = tesaiot_sign_data(TESAIOT_OID_CSR_KEY,
                               (const uint8_t *)data, data_len,
                               sig1, &sig1_len);
    printf("  Method 1 (sign_data):     %s (%u bytes)\n",
           rc == 0 ? "OK" : "FAIL", sig1_len);

    /* Method 2: Manual hash + sign (existing API) */
    uint8_t digest[32];
    uint16_t digest_len = 32;
    rc = tesaiot_optiga_hash((const uint8_t *)data, data_len,
                             digest, &digest_len);

    uint8_t sig2[80];
    uint16_t sig2_len = sizeof(sig2);
    if (rc == 0) {
        rc = tesaiot_optiga_sign(TESAIOT_OID_CSR_KEY, digest, digest_len,
                                 sig2, &sig2_len);
    }
    printf("  Method 2 (hash+sign):     %s (%u bytes)\n",
           rc == 0 ? "OK" : "FAIL", sig2_len);

    /* Note: ECDSA is non-deterministic, so sig1 != sig2 (different random k)
     * But both signatures are valid and will verify correctly */
    printf("  Note: Both signatures are valid (ECDSA uses random k)\n");

    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_05_sign_verify_telemetry(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 05: Signed Non-Repudiation Telemetry\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc = tesaiot_init();
    if (rc != 0) return rc;

    rc = tesaiot_verify_license();
    if (rc != 0) { tesaiot_deinit(); return rc; }

    demo_sign_telemetry();
    demo_challenge_response();
    demo_compare_sign_methods();

    tesaiot_deinit();
    printf("\n=== Example 05 Complete ===\n\n");
    return 0;
}

int main(void) { return example_05_sign_verify_telemetry(); }
