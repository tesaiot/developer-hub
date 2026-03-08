/**
 * @file tesaiot_optiga.h
 * @brief TESAIoT OPTIGA Trust M Integration - Umbrella Header (Group 4)
 * @version 2.2
 * @date 2026-01-18
 *
 * Part of TESAIoT Firmware SDK
 *
 * This umbrella header provides access to TESAIoT's OPTIGA integration layer:
 * - Trust M certificate lifecycle management
 * - Certificate validation and utilities
 * - High-level OPTIGA operations for TESAIoT workflows
 *
 * Domain Group: Core TESAIoT's Optiga Trust M (Integration layer)
 *
 * Usage:
 * @code
 * #include "tesaiot_optiga.h"
 *
 * // Certificate validation
 * tesaiot_cert_validation_result_t result;
 * tesaiot_check_certificate_validity(0xE0E1, &result);
 *
 * // Trust M state machine operations
 * trustm_state_t state = trustm_get_state();
 * @endcode
 *
 * @note For low-level OPTIGA instance management, use tesaiot_optiga_core.h
 * @note For CSR workflow, use tesaiot_csr.h
 * @note For Protected Update, use tesaiot_protected_update.h
 *
 * ============================================================================
 * VERSION HISTORY:
 * v2.1 (2026-01-18): Initial umbrella header.
 * v2.2 (2026-01-18): Merged cert_utils and optiga_trust_m directly.
 * ============================================================================
 */

#ifndef TESAIOT_OPTIGA_H
#define TESAIOT_OPTIGA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "optiga_util.h"
#include "optiga_trust_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Constants
 *---------------------------------------------------------------------------*/

/** OPTIGA OID for Device Keypair (Session/CSR key) */
#define TESAIOT_OPTIGA_OID_SESSION_KEY  0xE0F1

/** OPTIGA OID for Device Certificate */
#define TESAIOT_OPTIGA_OID_DEVICE_CERT  0xE0E1

/** Maximum public key size (DER format) */
#define TESAIOT_OPTIGA_PUBKEY_MAX_LEN   100

/** Maximum CSR size (PEM format) */
#define TESAIOT_OPTIGA_CSR_MAX_LEN      600

/*----------------------------------------------------------------------------
 * Public API Functions
 *---------------------------------------------------------------------------*/

/**
 * Generate ECC P-256 keypair in OPTIGA Trust M
 *
 * @param key_oid         OID where keypair will be stored (e.g., 0xE0F1)
 * @param public_key_der  Output buffer for public key (DER format)
 * @param pubkey_len      Input: buffer size, Output: actual key size
 * @return true on success, false on error
 */
bool tesaiot_optiga_generate_keypair(
    uint16_t key_oid,
    uint8_t *public_key_der,
    uint16_t *pubkey_len
);

/**
 * Generate Certificate Signing Request (CSR) signed by OPTIGA
 *
 * @param key_oid           OID of keypair to use for signing (e.g., 0xE0F1)
 * @param public_key_der    Public key from keypair (DER format)
 * @param pubkey_len        Public key size
 * @param subject           Subject string (e.g., "CN=device_uid,O=TESAIoT")
 * @param csr_pem           Output buffer for CSR (PEM format)
 * @param csr_pem_len       CSR buffer size
 * @return true on success, false on error
 */
bool tesaiot_optiga_generate_csr(
    uint16_t key_oid,
    const uint8_t *public_key_der,
    uint16_t pubkey_len,
    const char *subject,
    char *csr_pem,
    size_t csr_pem_len
);

/*============================================================================
 * Certificate Utility Types and Functions (from tesaiot_cert_utils.h)
 *============================================================================*/

/**
 * @brief TESAIoT-namespaced alias for cert_validation_result_t
 */
typedef cert_validation_result_t tesaiot_cert_validation_result_t;

/**
 * @brief TESAIoT-namespaced alias for cert_source_t
 */
typedef cert_source_t tesaiot_cert_source_t;

/**
 * @brief TESAIoT-namespaced enum values
 */
#define TESAIOT_CERT_SOURCE_DEVICE  CERT_SOURCE_DEVICE
#define TESAIOT_CERT_SOURCE_FACTORY CERT_SOURCE_FACTORY

/**
 * @brief Check certificate validity (TESAIoT namespace)
 * @see optiga_check_certificate_validity()
 */
static inline bool tesaiot_check_certificate_validity(uint16_t oid,
                                                      tesaiot_cert_validation_result_t *result) {
    return optiga_check_certificate_validity(oid, result);
}

/**
 * @brief Get days until certificate expiry (TESAIoT namespace)
 * @see optiga_get_cert_days_until_expiry()
 */
static inline uint32_t tesaiot_get_cert_days_until_expiry(uint16_t oid) {
    return optiga_get_cert_days_until_expiry(oid);
}

/*============================================================================
 * Trust M State Machine Types (from tesaiot_optiga_trust_m.h)
 *============================================================================*/

typedef enum
{
    TRUSTM_STATE_IDLE = 0,
    TRUSTM_STATE_PUBLISHING_CSR,
    TRUSTM_STATE_WAITING_FOR_MANIFEST,
    TRUSTM_STATE_APPLYING_UPDATE,
    TRUSTM_STATE_WAITING_FOR_CERTIFICATE,
    TRUSTM_STATE_COMPLETE,
    TRUSTM_STATE_ERROR,
    TRUSTM_STATE_WAITING_FOR_JSON_BUNDLE,
    TRUSTM_STATE_PROCESSING_JSON_BUNDLE,
    TRUSTM_STATE_WRITING_TRUST_ANCHOR,
    TRUSTM_STATE_VERIFYING_MANIFEST,
    TRUSTM_STATE_APPLYING_FRAGMENTS,
    TRUSTM_STATE_PROTECTED_UPDATE_SUCCESS,
    TRUSTM_STATE_PROTECTED_UPDATE_FAILED
} trustm_state_t;

/*============================================================================
 * Trust M Configuration Constants
 *============================================================================*/

#define TRUSTM_CSR_MAX_ATTEMPTS         (5U)
#define TRUSTM_CSR_RETRY_DELAY_MS       (2000U)
#define TRUSTM_STATUS_MAX_ATTEMPTS      (3U)
#define TRUSTM_STATUS_RETRY_DELAY_MS    (1500U)
#define TRUSTM_STATUS_TELEMETRY_ATTEMPTS (3U)
#define TRUSTM_STATUS_TELEMETRY_SUFFIX  "/status"
#define TRUSTM_MANIFEST_TIMEOUT_MS      (300000UL)
#define TRUSTM_DEV_CERT_TIMEOUT_MS      (300000UL)
#define TRUSTM_RNG_TIMEOUT_MS           (50U)
#define TRUSTM_INITIAL_PAYLOAD_VERSION  (1U)
#define TRUSTM_FIRMWARE_VERSION         "WiFi_MQTTs_Client_Protected_Update"

/*============================================================================
 * Trust M State Management Functions
 *============================================================================*/

/* State management */
void trustm_reset_state(void);
trustm_state_t trustm_get_state(void);
void trustm_update_state(trustm_state_t new_state, const char *status_code, const char *detail);
void trustm_report_error(const char *status_code, const char *detail);

/* MQTT publishing */
int publish_mqtt_msg(const char *msg, const char *topic_suffix);
int publish_csr(uint8_t *csr, size_t csr_length, uint16_t target_oid,
                uint16_t trust_anchor_oid, uint32_t payload_version);
int tesaiot_publish_protected_update(const char *target_oid, const char *trust_anchor_oid,
                                     uint32_t payload_version);

const char *trustm_current_correlation_id(void);
uint32_t trustm_generate_payload_version(void);
void trustm_handle_pubkey(uint8_t *pubkey_buf, size_t pubkey_len);

/* Runtime initialization */
bool trustm_runtime_init(void);
bool trustm_enqueue_scenario(uint8_t scenario);
bool trustm_wait_for_scenario(uint8_t *scenario, TickType_t ticks_to_wait);

/**
 * \brief Publish MQTT command payload to device/{device_id}/commands/{topic_suffix}
 */
int trustm_publish_command_payload(const char *topic_suffix, char *payload, size_t payload_len,
                                   bool take_ownership, uint8_t max_attempts, uint32_t retry_delay_ms);

/**
 * \brief Publish MQTT command with QoS 0 (fire-and-forget).
 */
int trustm_publish_command_qos0(const char *topic_suffix, char *payload, size_t payload_len);

void publish_request_protected_update(void);

/*============================================================================
 * Certificate Lifecycle Management Functions
 *============================================================================*/

uint16_t tesaiot_select_mqtt_certificate(void);
bool tesaiot_is_using_fallback_certificate(void);
bool tesaiot_auto_trigger_csr_renewal(void);
bool tesaiot_check_pending_csr(void);  /* Call from main loop to execute deferred CSR */
void tesaiot_reset_fallback_state(void);

/* Runtime certificate recovery (Menu 7) */
void tesaiot_set_force_factory_cert(bool force);
bool tesaiot_get_force_factory_cert(void);

/* Smart Auto-Fallback */
bool check_device_certificate_exists(void);
bool check_platform_certificate(void);
bool upload_device_certificate_to_platform(void);
bool sync_certificate_with_platform(void);

/*============================================================================
 * Unified API Wrappers (inline, zero overhead)
 *============================================================================*/

static inline bool tesaiot_read_factory_uid(char *uid_hex, size_t uid_hex_len)
{
    return optiga_read_factory_uid(uid_hex, uid_hex_len);
}

static inline bool tesaiot_read_factory_certificate(char *cert_pem, uint16_t *cert_pem_length)
{
    return optiga_read_factory_certificate(cert_pem, cert_pem_length);
}

static inline bool tesaiot_generate_device_keypair(uint16_t key_oid, uint8_t *public_key_der,
                                                   uint16_t *public_key_der_len)
{
    return optiga_generate_device_keypair(key_oid, public_key_der, public_key_der_len);
}

static inline bool tesaiot_generate_csr_pem(uint16_t key_oid, const uint8_t *public_key_der,
                                            uint16_t public_key_der_len, const char *subject,
                                            char *csr_pem, size_t csr_pem_len)
{
    return optiga_generate_csr_pem(key_oid, public_key_der, public_key_der_len, subject,
                                   csr_pem, csr_pem_len);
}

static inline int tesaiot_publish_csr(uint8_t *csr, size_t csr_length, uint16_t target_oid,
                                      uint16_t trust_anchor_oid, uint32_t payload_version)
{
    return publish_csr(csr, csr_length, target_oid, trust_anchor_oid, payload_version);
}

static inline void tesaiot_publish_request_protected_update(void)
{
    publish_request_protected_update();
}

static inline optiga_lib_status_t tesaiot_protected_update(uint16_t trust_anchor_oid,
                                                           uint16_t target_key_oid,
                                                           uint16_t confidentiality_oid)
{
    return protected_update(trust_anchor_oid, target_key_oid, confidentiality_oid);
}

static inline optiga_lib_status_t tesaiot_test_metadata_operations(void)
{
    return test_metadata_operations();
}

/*============================================================================
 * Protected Update Helper Functions
 *============================================================================*/

optiga_lib_status_t tesaiot_read_lcso(uint8_t *lcso);
optiga_lib_status_t tesaiot_write_metadata(uint16_t oid, uint8_t *metadata, uint16_t length);
optiga_lib_status_t tesaiot_read_metadata(uint16_t oid, uint8_t *buffer, uint16_t *length);
optiga_lib_status_t tesaiot_write_data(uint16_t oid, const uint8_t *data, uint16_t length);
optiga_lib_status_t tesaiot_write_trust_anchor(uint16_t oid, const uint8_t *data, uint16_t length);
optiga_lib_status_t tesaiot_erase_data(uint16_t oid);
optiga_lib_status_t tesaiot_read_data(uint16_t oid, uint8_t *buffer, uint16_t *length);
optiga_lib_status_t tesaiot_verify_manifest_with_trustanchor(const uint8_t *manifest,
                                                              uint16_t manifest_len,
                                                              uint16_t trust_anchor_oid);
optiga_lib_status_t tesaiot_reset_optiga_session(volatile optiga_lib_status_t *status_ptr,
                                                  callback_handler_t callback);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_OPTIGA_H */
