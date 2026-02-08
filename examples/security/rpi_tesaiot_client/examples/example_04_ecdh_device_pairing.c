/**
 * @file example_04_ecdh_device_pairing.c
 * @brief Example: ECDH Device-to-Device Key Exchange
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_optiga_generate_keypair() — Generate ECDH keypair at 0xE0F2
 *   - tesaiot_ecdh_shared_secret()      — ECDH key agreement
 *   - tesaiot_hkdf_derive()             — Derive session keys from shared secret
 *   - tesaiot_aes_encrypt/decrypt()     — Encrypt messages with derived key
 *
 * OIDs used:
 *   0xE0F2 — Application key (ECDH private key, never exported)
 *   0xE200 — AES session key (derived from ECDH)
 *
 * Use Case B3: Device-to-Device Secure Channel
 *   Two devices establish a shared secret without a server.
 *   Private keys never leave OPTIGA hardware.
 *
 * Flow:
 *   Device A                              Device B
 *   --------                              --------
 *   Generate keypair at 0xE0F2            Generate keypair at 0xE0F2
 *   Send pubkey_A ────────────────────> Receive pubkey_A
 *   Receive pubkey_B <──────────────── Send pubkey_B
 *   ECDH(0xE0F2, pubkey_B) = shared_S    ECDH(0xE0F2, pubkey_A) = shared_S
 *   HKDF(shared_S) = aes_key             HKDF(shared_S) = aes_key
 *   AES_Enc(data, aes_key) = cipher      AES_Dec(cipher, aes_key) = data
 *
 * Application Patterns:
 *   - Smart Home: Sensor <-> Gateway secure pairing
 *   - Industrial: PLC <-> SCADA encrypted channel
 *   - Medical IoT: Wearable <-> Bedside monitor secure link
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_optiga.h"
#include "tesaiot_crypto.h"

/*---------------------------------------------------------------------------
 * Simulate Device A Side (this device)
 *
 * In a real scenario, Device B's public key would be received via
 * Bluetooth, NFC, or MQTT exchange.
 *---------------------------------------------------------------------------*/
static int demo_ecdh_key_exchange(void)
{
    printf("\n=== B3: Device-to-Device ECDH Key Exchange ===\n");

    /* Step 1: Generate our ECDH keypair at OID 0xE0F2 */
    printf("\n  [Device A] Generating ECDH keypair at OID 0xE0F2...\n");

    uint8_t our_pubkey[100];
    uint16_t our_pubkey_len = sizeof(our_pubkey);

    int rc = tesaiot_optiga_generate_keypair(0xE0F2, our_pubkey, &our_pubkey_len);
    if (rc != 0) {
        printf("  ERROR: Keypair generation failed (rc=%d)\n", rc);
        return rc;
    }
    printf("  Our public key (%u bytes): %02X%02X%02X%02X...\n",
           our_pubkey_len, our_pubkey[0], our_pubkey[1],
           our_pubkey[2], our_pubkey[3]);

    /*
     * Step 2: Exchange public keys (simulated)
     *
     * In production:
     *   - Send our_pubkey to Device B via Bluetooth/NFC/MQTT
     *   - Receive Device B's pubkey from the same channel
     *
     * For this demo, we simulate Device B's pubkey.
     * In a real test, use a second OPTIGA device or OpenSSL keypair.
     */
    printf("\n  [Simulated] In production, exchange pubkeys via BLE/NFC/MQTT\n");
    printf("  [Simulated] Peer public key would be received here\n");

    /*
     * Step 3: ECDH shared secret (commented out - needs real peer key)
     *
     * uint8_t shared_secret[32];
     * uint16_t secret_len = sizeof(shared_secret);
     *
     * rc = tesaiot_ecdh_shared_secret(0xE0F2, peer_pubkey, peer_pubkey_len,
     *                                 shared_secret, &secret_len);
     *
     * Step 4: Derive session key via HKDF
     *
     * const uint8_t info[] = "TESAIoT-session-key-v1";
     * uint8_t session_key[32];
     *
     * rc = tesaiot_hkdf_derive(0xE0F2, NULL, 0, info, sizeof(info) - 1,
     *                          session_key, 32);
     *
     * Step 5: Use session key for AES encryption
     *
     * tesaiot_aes_encrypt(data, len, iv, iv_out, ciphertext, &cipher_len);
     */

    printf("\n  ECDH Flow Summary:\n");
    printf("    1. Generate keypair     -> privkey stays in OPTIGA\n");
    printf("    2. Exchange pubkeys     -> via BLE/NFC/MQTT\n");
    printf("    3. ECDH shared secret   -> 32 bytes (private key never exposed)\n");
    printf("    4. HKDF derive key      -> session key for AES\n");
    printf("    5. AES encrypt/decrypt  -> secure bidirectional channel\n");

    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_04_ecdh_device_pairing(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 04: ECDH Device-to-Device Key Exchange\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc = tesaiot_init();
    if (rc != 0) return rc;

    rc = tesaiot_verify_license();
    if (rc != 0) { tesaiot_deinit(); return rc; }

    demo_ecdh_key_exchange();

    tesaiot_deinit();
    printf("\n=== Example 04 Complete ===\n\n");
    return 0;
}

int main(void) { return example_04_ecdh_device_pairing(); }
