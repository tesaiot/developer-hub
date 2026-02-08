/**
 * @file examples.h
 * @brief TESAIoT v3.0.0 Example Code Declarations
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * This header declares example functions that demonstrate
 * TESAIoT Secure Library crypto utility usage patterns.
 */

#ifndef TESAIOT_EXAMPLES_H
#define TESAIOT_EXAMPLES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Example 01: Hardware TRNG + Secure Credential Storage */
int example_01_random_and_store(void);

/* Example 02: AES Encryption of Sensor Data at Rest */
int example_02_aes_encrypt_sensor(void);

/* Example 03: HMAC-SHA256 MQTT Payload Integrity */
int example_03_hmac_mqtt_integrity(void);

/* Example 04: ECDH Device-to-Device Key Exchange */
int example_04_ecdh_device_pairing(void);

/* Example 05: Signed Non-Repudiation Telemetry */
int example_05_sign_verify_telemetry(void);

/* Example 06: System Health Monitor Dashboard */
int example_06_health_monitor(void);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_EXAMPLES_H */
