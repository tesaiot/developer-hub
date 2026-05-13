/**
 * @file example_03_hmac_mqtt_integrity.c
 * @brief Example: HMAC-SHA256 MQTT Payload Integrity
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_hmac_sha256()         — Compute HMAC-SHA256 with hardware key
 *   - tesaiot_secure_store_write()  — Provision HMAC key
 *   - tesaiot_random_generate()     — Generate key material
 *
 * OIDs used:
 *   0xF1D5 (slot 5) — HMAC secret key
 *
 * Use Case B2: End-to-End Encrypted MQTT
 *   Even though TLS protects transport, application-level HMAC adds
 *   defense-in-depth. If MQTT broker is compromised, payload integrity
 *   can still be verified at the receiving end.
 *
 * Application Patterns:
 *   - Smart Home: Signed sensor readings (tamper detection)
 *   - Industrial: Verified PLC commands (safety-critical)
 *   - Medical IoT: Authenticated vital signs (regulatory compliance)
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_crypto.h"

/*---------------------------------------------------------------------------
 * Step 1: Provision HMAC Key (do once during device setup)
 *---------------------------------------------------------------------------*/
static int provision_hmac_key(void)
{
    printf("\n=== Step 1: Provision HMAC Key (Slot 5 = OID 0xF1D5) ===\n");

    /* Generate 32-byte random key using hardware TRNG */
    uint8_t hmac_key[32];
    int rc = tesaiot_random_generate(hmac_key, 32);
    if (rc != 0) {
        printf("  ERROR: Random key generation failed (rc=%d)\n", rc);
        return rc;
    }

    /* Store in slot 5 (OID 0xF1D5) */
    rc = tesaiot_secure_store_write(5, hmac_key, 32);
    if (rc != 0) {
        printf("  ERROR: Failed to store HMAC key (rc=%d)\n", rc);
        return rc;
    }

    printf("  HMAC key provisioned (32 bytes) in hardware slot 5\n");
    printf("  TIP: Share this key with the server via secure channel\n");
    printf("  TIP: In production, set Read=NEVER on slot 5\n");

    /* Clear key from memory (security best practice) */
    memset(hmac_key, 0, sizeof(hmac_key));

    return 0;
}

/*---------------------------------------------------------------------------
 * Step 2: Sign MQTT Payload with HMAC before Publishing
 *
 * Payload format: [JSON data || 32-byte HMAC]
 * Receiver: Split last 32 bytes as MAC, verify, then parse JSON
 *---------------------------------------------------------------------------*/
static int demo_sign_mqtt_payload(void)
{
    printf("\n=== Step 2: HMAC-Sign MQTT Payload ===\n");

    /* Simulate sensor payload */
    const char *payload_json =
        "{\"device\":\"sensor-001\",\"temp\":25.4,\"humidity\":62.1,"
        "\"ts\":1706745600,\"seq\":42}";
    uint16_t payload_len = (uint16_t)strlen(payload_json);

    printf("  Payload: %s\n", payload_json);

    /* Compute HMAC-SHA256 using key in slot 5 */
    uint8_t mac[32];
    uint16_t mac_len = sizeof(mac);

    int rc = tesaiot_hmac_sha256(5, (const uint8_t *)payload_json,
                                 payload_len, mac, &mac_len);
    if (rc != 0) {
        printf("  ERROR: HMAC computation failed (rc=%d)\n", rc);
        return rc;
    }

    printf("  HMAC-SHA256: ");
    for (int i = 0; i < 32; i++) printf("%02X", mac[i]);
    printf("\n");

    /*
     * In production, publish to MQTT:
     *   uint8_t signed_payload[4096];
     *   memcpy(signed_payload, payload_json, payload_len);
     *   memcpy(signed_payload + payload_len, mac, 32);
     *   tesaiot_mqtt_publish(topic, signed_payload, payload_len + 32);
     *
     * Server verifies:
     *   1. Split: data = payload[0..N-32], mac = payload[N-32..N]
     *   2. Compute HMAC-SHA256(data, shared_key)
     *   3. Compare computed MAC with received MAC
     *   4. If match -> data is authentic and unmodified
     */

    /* Verify HMAC is deterministic (same input -> same output) */
    uint8_t mac2[32];
    uint16_t mac2_len = sizeof(mac2);

    rc = tesaiot_hmac_sha256(5, (const uint8_t *)payload_json,
                             payload_len, mac2, &mac2_len);
    if (rc == 0 && memcmp(mac, mac2, 32) == 0) {
        printf("  Deterministic check: PASS (same payload -> same MAC)\n");
    }

    /* Verify different data -> different MAC */
    const char *modified = "{\"device\":\"sensor-001\",\"temp\":99.9}";
    uint8_t mac3[32];
    uint16_t mac3_len = sizeof(mac3);

    rc = tesaiot_hmac_sha256(5, (const uint8_t *)modified,
                             (uint16_t)strlen(modified), mac3, &mac3_len);
    if (rc == 0 && memcmp(mac, mac3, 32) != 0) {
        printf("  Tamper detection: PASS (modified payload -> different MAC)\n");
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_03_hmac_mqtt_integrity(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 03: HMAC-SHA256 MQTT Payload Integrity\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc = tesaiot_init();
    if (rc != 0) return rc;

    rc = tesaiot_verify_license();
    if (rc != 0) { tesaiot_deinit(); return rc; }

    provision_hmac_key();
    demo_sign_mqtt_payload();

    tesaiot_deinit();
    printf("\n=== Example 03 Complete ===\n\n");
    return 0;
}

int main(void) { return example_03_hmac_mqtt_integrity(); }
