/**
 * @file tesaiot_advanced.h
 * @brief TESAIoT v3.0.0 Advanced Utility Functions (Phase 3)
 * @version 3.0.0
 * @date 2026-02-08
 *
 * Phase 3 (NICE TO HAVE):
 *   - tesaiot_counter_read/increment() — Monotonic counter operations
 *   - tesaiot_jwt_sign()               — JWT token generation (ES256)
 *   - tesaiot_health_check()           — Comprehensive device diagnostics
 *   - tesaiot_attestation_generate()   — PSA Entity Attestation Token
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 */

#ifndef TESAIOT_ADVANCED_H
#define TESAIOT_ADVANCED_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Health Check Report Structure
 *===========================================================================*/

/**
 * @brief Device health check report
 */
typedef struct {
    bool optiga_ok;          /**< OPTIGA reachable and initialized */
    bool factory_cert_ok;    /**< OID 0xE0E0 readable and non-empty */
    bool device_cert_ok;     /**< OID 0xE0E1 readable and non-empty */
    bool license_ok;         /**< License verified successfully */
    bool mqtt_ok;            /**< MQTT connected (if applicable) */
    bool tools_ok;           /**< External tools verified (ECDSA code signing) */
    uint8_t lcso_value;      /**< Lifecycle State Object value from OID 0xE0C0 */
} tesaiot_health_report_t;

/*============================================================================
 * Monotonic Counter Operations
 *===========================================================================*/

/**
 * @brief Read monotonic counter value from OPTIGA
 *
 * @param[in]  counter_id  Counter (0-3, maps to OID 0xE120-0xE123)
 * @param[out] value       Output: current 32-bit counter value
 * @return 0 on success, negative on error
 *
 * @note Counter data is 8 bytes: [4-byte threshold][4-byte current_value]
 * @note NVM write limit: ~600,000 increments per counter
 */
int tesaiot_counter_read(uint8_t counter_id, uint32_t *value);

/**
 * @brief Increment monotonic counter (irreversible without Protected Update)
 *
 * @param[in] counter_id  Counter (0-3)
 * @param[in] step        Increment step (1-255)
 * @return 0 on success, negative on error
 *
 * @warning Counter increments are IRREVERSIBLE. The counter can only go up.
 *          NVM write limit: ~600,000 writes per counter.
 */
int tesaiot_counter_increment(uint8_t counter_id, uint8_t step);

/*============================================================================
 * JWT Token Generation
 *===========================================================================*/

/**
 * @brief Sign JWT token with OPTIGA hardware key (ES256)
 *
 * @param[in]     key_oid      Signing key OID (e.g., 0xE0F1)
 * @param[in]     claims_json  JSON claims string (will be base64url-encoded)
 * @param[out]    jwt_out      Output: complete JWT string "header.payload.signature"
 * @param[in,out] jwt_len      In: buffer size, Out: actual JWT length
 * @return 0 on success, negative on error
 *
 * @note Algorithm: ES256 (ECDSA P-256 + SHA-256)
 * @note Fixed header: {"alg":"ES256","typ":"JWT"}
 * @note Compatible with: AWS IoT, Google Cloud IoT, Azure IoT Hub
 */
int tesaiot_jwt_sign(uint16_t key_oid,
                     const char *claims_json,
                     char *jwt_out, size_t *jwt_len);

/*============================================================================
 * Health Check
 *===========================================================================*/

/**
 * @brief Run comprehensive device health check
 *
 * @param[out] report  Output: health check results
 * @return 0 if all checks pass, negative if any check fails
 *
 * @note Composes existing functions: optiga_is_initialized, read_data,
 *       get_license_status, mqtt_is_connected, verify_all_tools
 */
int tesaiot_health_check(tesaiot_health_report_t *report);

/*============================================================================
 * Device Attestation
 *===========================================================================*/

/**
 * @brief Generate PSA-aligned Entity Attestation Token (EAT)
 *
 * @param[in]     nonce      Challenge nonce from verifier
 * @param[in]     nonce_len  Nonce length (typically 32 bytes)
 * @param[out]    token      Output: COSE_Sign1 encoded attestation token
 * @param[in,out] token_len  In: buffer size, Out: actual token length
 * @return 0 on success, negative on error
 *
 * @note Token claims: UID, lifecycle, fw_version, license_status, nonce
 * @note Signed with device key (0xE0F1)
 * @note Requires CBOR encoder (internal helper)
 */
int tesaiot_attestation_generate(const uint8_t *nonce, uint16_t nonce_len,
                                 uint8_t *token, uint16_t *token_len);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_ADVANCED_H */
