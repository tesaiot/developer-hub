/**
 * @file example_C2_challenge_response_auth.c
 * @brief Example: Challenge-response authentication between devices
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category C: Identification & Attestation
 *
 * Challenge-Response Authentication Protocol:
 * 1. Verifier generates random challenge (nonce)
 * 2. Prover signs challenge with private key
 * 3. Verifier checks signature with prover's public certificate
 * 4. If valid -> Prover identity confirmed
 *
 * This prevents replay attacks (fresh nonce each time)
 * and man-in-the-middle (signature bound to private key).
 *
 * OIDs used: 0xE0F1 (Device Key for signing)
 *
 * Required: Phase 1+2 functions (tesaiot_random_generate, tesaiot_sign_data)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_C2_challenge_response_auth(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example C2: Challenge-Response Authentication\n");
    printf("  OID Category: C - Identification & Attestation\n");
    printf("  Pattern: Random Challenge + ECDSA Signature\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Verifier generates random challenge --- */
    printf("[Step 1] [VERIFIER] Generate 32-byte random challenge...\n");
    uint8_t challenge[32];
    rc = tesaiot_random_generate(challenge, 32);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        return;
    }
    printf("  Challenge: ");
    for (int i = 0; i < 16; i++) printf("%02X", challenge[i]);
    printf("...\n");
    printf("  (Sent to prover via MQTT or direct link)\n\n");

    /* --- Step 2: Prover signs challenge with device key --- */
    printf("[Step 2] [PROVER] Sign challenge with Device Key (0xE0F1)...\n");
    uint8_t response[80]; /* DER-encoded ECDSA signature */
    uint16_t resp_len = sizeof(response);

    rc = tesaiot_sign_data(0xE0F1, challenge, 32, response, &resp_len);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        printf("  Note: Requires Device Key in OID 0xE0F1\n");
        printf("  (Run CSR workflow first to generate the key)\n\n");
        return;
    }
    printf("  Response (%u bytes): ", resp_len);
    for (int i = 0; i < 16; i++) printf("%02X", response[i]);
    printf("...\n");
    printf("  (Sent back to verifier)\n\n");

    /* --- Step 3: Show protocol flow --- */
    printf("[Step 3] Complete Protocol Flow:\n\n");
    printf("  Verifier (Server)              Prover (Device)\n");
    printf("  -----------------              ---------------\n");
    printf("  1. nonce = TRNG(32)\n");
    printf("     challenge = nonce\n");
    printf("                          ---->  2. sig = Sign(0xE0F1, challenge)\n");
    printf("  3. Verify(sig, challenge,      \n");
    printf("     prover_cert.pubkey)  <----     response = sig\n");
    printf("  4. If VALID -> AUTHENTICATED\n\n");

    /* --- Step 4: Show security properties --- */
    printf("[Step 4] Security Properties:\n\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | REPLAY PROTECTION:                               |\n");
    printf("  |   Fresh random challenge each time.              |\n");
    printf("  |   Old responses are useless for new challenges.  |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | IMPERSONATION PROTECTION:                        |\n");
    printf("  |   Only the device with private key in OPTIGA     |\n");
    printf("  |   can produce a valid signature.                 |\n");
    printf("  |   Key is hardware-bound, cannot be cloned.       |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | MAN-IN-THE-MIDDLE:                               |\n");
    printf("  |   Attacker can't forge signature without key.    |\n");
    printf("  |   Certificate chain validates device identity.   |\n");
    printf("  +--------------------------------------------------+\n\n");

    /* --- Step 5: MQTT-based authentication example --- */
    printf("[Step 5] MQTT-Based Authentication:\n\n");
    printf("  Topic (challenge): device/{uid}/auth/challenge\n");
    printf("  {\n");
    printf("    \"nonce\": \"");
    for (int i = 0; i < 32; i++) printf("%02x", challenge[i]);
    printf("\",\n");
    printf("    \"timestamp\": 1738972800,\n");
    printf("    \"expires_in\": 30\n");
    printf("  }\n\n");
    printf("  Topic (response): device/{uid}/auth/response\n");
    printf("  {\n");
    printf("    \"nonce\": \"");
    for (int i = 0; i < 32; i++) printf("%02x", challenge[i]);
    printf("\",\n");
    printf("    \"signature\": \"");
    for (int i = 0; i < resp_len && i < 8; i++) printf("%02x", response[i]);
    printf("...\",\n");
    printf("    \"sig_alg\": \"ECDSA-P256-SHA256\"\n");
    printf("  }\n\n");

    printf("================================================================\n");
    printf("  Example C2 Complete\n");
    printf("  Use cases: Device fleet authentication, peer verification,\n");
    printf("  secure pairing, access control\n");
    printf("================================================================\n\n");
}
