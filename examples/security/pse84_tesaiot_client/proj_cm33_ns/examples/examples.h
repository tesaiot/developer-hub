/**
 * @file examples.h
 * @brief TESAIoT v3.0.0 Crypto Utilities - Example Function Declarations
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Example codes demonstrating practical IoT application patterns
 * using the TESAIoT crypto utility API (tesaiot_crypto.h).
 *
 * Each example is a standalone function that can be called from main.c menu.
 *
 * Categories (based on OID Practical Use Cases):
 *   A: Secure Storage        - Credential management, data encryption at rest
 *   B: Secure Communication  - HMAC, E2E encryption, device-to-device channels
 *   C: Identification        - Signed telemetry, challenge-response authentication
 *   D: Security Operations   - Anti-replay counters, health monitoring
 *   E: Application Patterns  - Complete IoT sensor flow
 *
 * Usage in main.c:
 * @code
 * #include "examples/examples.h"
 *
 * case '6': example_A1_secure_credential_store(); break;
 * case '7': example_B1_hmac_mqtt_payload(); break;
 * @endcode
 */

#ifndef TESAIOT_EXAMPLES_H
#define TESAIOT_EXAMPLES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Category A: Secure Storage (Phase 1) */
void example_A1_secure_credential_store(void);
void example_A2_data_encryption_at_rest(void);

/* Category B: Secure Communication (Phase 1+2) */
void example_B1_hmac_mqtt_payload(void);
void example_B2_e2e_encrypted_mqtt(void);
void example_B3_device_to_device_channel(void);

/* Category C: Identification & Attestation (Phase 2) */
void example_C1_signed_telemetry(void);
void example_C2_challenge_response_auth(void);

/* Category D: Security Operations (Phase 3) */
void example_D1_anti_replay_counter(void);
void example_D2_health_dashboard(void);

/* Category E: Complete Application Patterns (Phase 1-3) */
void example_E1_smart_sensor_complete(void);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_EXAMPLES_H */
