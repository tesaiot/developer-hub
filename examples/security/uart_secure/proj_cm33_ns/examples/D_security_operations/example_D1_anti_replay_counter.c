/**
 * @file example_D1_anti_replay_counter.c
 * @brief Example: Anti-replay protection using monotonic counters
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category D: Security Operations
 *
 * Anti-replay pattern:
 * 1. Read monotonic counter from OPTIGA (OID 0xE120)
 * 2. Include counter value in message payload
 * 3. Sign message (counter + data) with device key
 * 4. Receiver checks: counter > last_seen_counter
 * 5. Counter can ONLY go up (survives power cycles)
 *
 * OIDs used: 0xE120 (Counter 0), 0xE0F1 (Device Key)
 *
 * Required: Phase 3 functions (tesaiot_counter_read/increment)
 *           Phase 2 functions (tesaiot_sign_data)
 *
 * @warning NVM write limit: ~600,000 increments per counter lifetime
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_D1_anti_replay_counter(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example D1: Anti-Replay Counter\n");
    printf("  OID Category: D - Security Operations\n");
    printf("  Pattern: Monotonic Counter + Signed Message\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Read current counter value --- */
    printf("[Step 1] Read monotonic counter 0 (OID 0xE120)...\n");
    uint32_t counter_val = 0;
    rc = tesaiot_counter_read(0, &counter_val);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        printf("  Note: Counter may need initial configuration\n\n");
        /* Continue with simulated value for demonstration */
        counter_val = 100;
        printf("  Using simulated counter value: %lu\n\n", (unsigned long)counter_val);
    } else {
        printf("  Current counter value: %lu\n\n", (unsigned long)counter_val);
    }

    /* --- Step 2: Increment counter (message sequence number) --- */
    printf("[Step 2] Increment counter by 1...\n");
    rc = tesaiot_counter_increment(0, 1);
    if (rc != TESAIOT_OK) {
        printf("  FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        printf("  (Expected if counter not configured - demo continues)\n");
        counter_val++; /* Simulate */
    } else {
        counter_val++;
        printf("  OK: New counter value: %lu\n", (unsigned long)counter_val);
    }
    printf("  WARNING: ~600,000 total increments per counter lifetime!\n\n");

    /* --- Step 3: Create signed message with counter --- */
    printf("[Step 3] Create signed message with counter...\n");
    char message[128];
    snprintf(message, sizeof(message),
             "{\"seq\":%lu,\"temp\":25.3,\"ts\":1738972800}",
             (unsigned long)counter_val);
    printf("  Message: %s\n", message);

    uint8_t signature[80];
    uint16_t sig_len = sizeof(signature);
    rc = tesaiot_sign_data(0xE0F1,
                            (const uint8_t *)message,
                            (uint16_t)strlen(message),
                            signature, &sig_len);
    if (rc != TESAIOT_OK) {
        printf("  Sign FAIL: %s (expected without Device Key)\n\n",
               tesaiot_error_str((tesaiot_error_t)rc));
    } else {
        printf("  Signature (%u bytes): ", sig_len);
        for (int i = 0; i < 8; i++) printf("%02X", signature[i]);
        printf("...\n\n");
    }

    /* --- Step 4: MQTT wire format --- */
    printf("[Step 4] Anti-Replay MQTT Format:\n");
    printf("  Topic: device/{uid}/telemetry/secured\n");
    printf("  {\n");
    printf("    \"data\": %s,\n", message);
    printf("    \"counter\": %lu,\n", (unsigned long)counter_val);
    printf("    \"sig\": \"<ECDSA signature>\",\n");
    printf("    \"sig_alg\": \"ECDSA-P256-SHA256\"\n");
    printf("  }\n\n");

    /* --- Step 5: Receiver verification logic --- */
    printf("[Step 5] Receiver Verification Logic:\n\n");
    printf("  received_counter = message.counter\n");
    printf("  \n");
    printf("  if (received_counter <= last_seen_counter)\n");
    printf("      REJECT (replay attack detected!)\n");
    printf("  \n");
    printf("  if (ECDSA_Verify(message, signature, device_cert) == FAIL)\n");
    printf("      REJECT (tampered!)\n");
    printf("  \n");
    printf("  ACCEPT: Update last_seen_counter = received_counter\n\n");

    /* --- Step 6: Show all counter OIDs --- */
    printf("[Step 6] Available Monotonic Counters:\n");
    printf("  +----+--------+------------------------------+\n");
    printf("  | ID |  OID   | Suggested Use                |\n");
    printf("  +----+--------+------------------------------+\n");
    printf("  |  0 | 0xE120 | Message sequence number      |\n");
    printf("  |  1 | 0xE121 | Boot counter                 |\n");
    printf("  |  2 | 0xE122 | Firmware update counter      |\n");
    printf("  |  3 | 0xE123 | Security event counter       |\n");
    printf("  +----+--------+------------------------------+\n");
    printf("  NVM lifetime: ~600,000 writes per counter\n\n");

    /* --- Step 7: Security analysis --- */
    printf("[Step 7] Why Replay Attacks Fail:\n\n");
    printf("  Attacker captures:  {seq:42, temp:25.3, sig:ABC...}\n");
    printf("  Attacker replays:   {seq:42, temp:25.3, sig:ABC...}\n");
    printf("  Server checks:      42 <= last_seen(42) -> REJECT!\n\n");
    printf("  Attacker modifies:  {seq:99, temp:25.3, sig:ABC...}\n");
    printf("  Server checks:      ECDSA_Verify FAILS (sig mismatch)\n\n");
    printf("  Counter survives power cycle (stored in OPTIGA NVM)\n");
    printf("  Counter cannot go backwards (hardware enforced)\n\n");

    printf("================================================================\n");
    printf("  Example D1 Complete\n");
    printf("  Use cases: Command integrity, firmware versioning,\n");
    printf("  audit trails, rollback protection\n");
    printf("================================================================\n\n");
}
