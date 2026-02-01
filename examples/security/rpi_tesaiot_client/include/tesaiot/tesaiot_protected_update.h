/**
 * @file tesaiot_protected_update.h
 * @brief TESAIoT Protected Update Workflow Public API
 * @version 2.0
 * @date 2026-01-23
 *
 * Secure firmware/data update using CBOR COSE_Sign1 manifest
 * Implements OPTIGA Trust M Protected Update with Trust Anchor verification
 *
 * Architecture: Library-first design (PSoC E84 reference pattern)
 */

#ifndef TESAIOT_PROTECTED_UPDATE_H
#define TESAIOT_PROTECTED_UPDATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Protected Update States
 *---------------------------------------------------------------------------*/

/**
 * @brief Protected Update Workflow State Machine States
 */
typedef enum {
    TESAIOT_PU_STATE_IDLE = 0,                /**< Waiting for manifest */
    TESAIOT_PU_STATE_RECEIVE_MANIFEST,        /**< Receiving CBOR manifest */
    TESAIOT_PU_STATE_VERIFY_MANIFEST,         /**< Verifying manifest signature */
    TESAIOT_PU_STATE_RECEIVE_FRAGMENTS,       /**< Receiving data fragments */
    TESAIOT_PU_STATE_PROCESS_FRAGMENTS,       /**< Processing fragments */
    TESAIOT_PU_STATE_DONE,                    /**< Update completed successfully */
    TESAIOT_PU_STATE_ERROR                    /**< Update failed */
} tesaiot_pu_state_t;

/*----------------------------------------------------------------------------
 * Protected Update Configuration
 *---------------------------------------------------------------------------*/

/**
 * @brief Protected Update Configuration Structure
 */
typedef struct {
    const char *device_id;                    /**< Device ID (from Trust M UID) */
    uint16_t trust_anchor_oid;                /**< Trust Anchor OID (default: 0xE0E8) */
    uint16_t secret_oid;                      /**< Secret OID for encryption (default: 0xF1D4) */
    bool verify_version;                      /**< Enable version downgrade protection */
    uint32_t current_version;                 /**< Current firmware/data version */
} tesaiot_pu_config_t;

/*----------------------------------------------------------------------------
 * Protected Update API
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize Protected Update workflow
 *
 * @param config Pointer to PU configuration structure
 * @return 0 on success, negative error code on failure
 *         -1: Invalid config parameter
 */
int tesaiot_pu_workflow_init(const tesaiot_pu_config_t *config);

/**
 * @brief Execute Protected Update with manifest and fragments
 *
 * Performs complete Protected Update operation:
 * 1. Parse CBOR COSE_Sign1 manifest
 * 2. Verify manifest signature with Trust Anchor
 * 3. Process data fragments
 * 4. Write to target OID via Protected Update API
 *
 * @param manifest CBOR-encoded COSE_Sign1 manifest
 * @param manifest_len Length of manifest
 * @param fragments Array of fragment pointers
 * @param fragment_lens Array of fragment lengths
 * @param num_fragments Number of fragments
 * @return 0 on success, negative error code on failure
 *
 * @note This is a blocking call
 * @note Manifest signature is verified with Trust Anchor at OID 0xE0E8
 * @note Target OID is specified in manifest payload
 */
int tesaiot_protected_update_execute(
    const uint8_t *manifest, uint16_t manifest_len,
    const uint8_t **fragments, uint16_t *fragment_lens,
    uint8_t num_fragments
);

/**
 * @brief Run complete Protected Update workflow (blocking wrapper)
 *
 * Convenience function that runs entire PU workflow from MQTT reception
 *
 * @return 0 on success, negative error code on failure
 *
 * @note Receives manifest and fragments via MQTT
 * @note Connects using device certificate (not factory cert)
 */
int tesaiot_run_protected_update_workflow(void);

/**
 * @brief Get current Protected Update state
 *
 * @return Current state of PU workflow state machine
 */
tesaiot_pu_state_t tesaiot_pu_workflow_get_state(void);

/**
 * @brief Get human-readable state name
 *
 * @param state State to convert to string
 * @return Pointer to static string with state name
 */
const char *tesaiot_pu_workflow_get_state_name(tesaiot_pu_state_t state);

/**
 * @brief Set default target OID for Protected Update
 *
 * Sets the default OID to write data to. This can be overridden by
 * the target_oid field in the manifest JSON from the platform.
 *
 * @param target_oid Target OID (0xE0E1-0xE0E3, default: 0xE0E2)
 *
 * @note If set to 0, the OID from manifest JSON will be used
 * @note This must be called before tesaiot_run_protected_update_workflow()
 */
void tesaiot_pu_set_default_target_oid(uint16_t target_oid);

/**
 * @brief Check if Trust Anchor is provisioned
 *
 * Reads OID 0xE0E8 and checks if it contains valid public key data
 *
 * @return true if Trust Anchor is provisioned, false otherwise
 */
bool tesaiot_pu_is_trust_anchor_provisioned(void);

/**
 * @brief Provision Trust Anchor public key manually
 *
 * Writes SPKI DER-encoded ECC public key to OID 0xE0E8
 * Used for one-time provisioning before Protected Update
 *
 * @param pubkey_der SPKI DER-encoded public key (91 bytes for P-256)
 * @param pubkey_len Length of public key
 * @return 0 on success, negative error code on failure
 *
 * @note Platform sends public key via MQTT topic: device/{id}/commands/trust_anchor
 * @note Format: ECC P-256 public key in SPKI DER format (SubjectPublicKeyInfo)
 */
int tesaiot_pu_provision_trust_anchor(const uint8_t *pubkey_der, uint16_t pubkey_len);

/*----------------------------------------------------------------------------
 * Protected Update Constants
 *---------------------------------------------------------------------------*/

/** Maximum manifest size (CBOR COSE_Sign1) */
#define TESAIOT_PU_MANIFEST_MAX_SIZE 1024

/** Maximum fragment size (increased to handle full certificates ~1KB) */
#define TESAIOT_PU_FRAGMENT_MAX_SIZE 1024

/** Maximum number of fragments */
#define TESAIOT_PU_MAX_FRAGMENTS 64

/** Protected Update timeout (milliseconds) */
#define TESAIOT_PU_TIMEOUT_MS 30000

/*----------------------------------------------------------------------------
 * Protected Update Error Codes
 *---------------------------------------------------------------------------*/

/**
 * @defgroup PU_ERRORS Protected Update Error Codes
 * @{
 */

#define TESAIOT_PU_SUCCESS                      0x0000  /**< Operation successful */
#define TESAIOT_PU_ERROR_INVALID_PARAM          -1      /**< Invalid parameter */
#define TESAIOT_PU_ERROR_NOT_INIT               -2      /**< PU not initialized */
#define TESAIOT_PU_ERROR_INVALID_MANIFEST       -3      /**< Invalid CBOR manifest */
#define TESAIOT_PU_ERROR_SIGNATURE_VERIFY       -4      /**< Signature verification failed */
#define TESAIOT_PU_ERROR_INVALID_FRAGMENT       -5      /**< Invalid fragment */
#define TESAIOT_PU_ERROR_VERSION_DOWNGRADE      -6      /**< Version downgrade detected */
#define TESAIOT_PU_ERROR_TARGET_OID_WRITE       -7      /**< Target OID write failed */
#define TESAIOT_PU_ERROR_TRUST_ANCHOR_READ      -8      /**< Trust Anchor read failed */
#define TESAIOT_PU_ERROR_OPTIGA_OP              -9      /**< OPTIGA operation failed */

/** @} */

/*----------------------------------------------------------------------------
 * OID Definitions (Protected Update Exclusive)
 *---------------------------------------------------------------------------*/

/**
 * @defgroup PU_OIDS Protected Update OIDs
 * @{
 */

/** Trust Anchor OID (ECC public key for manifest verification) */
#define TESAIOT_PU_OID_TRUST_ANCHOR 0xE0E8

/** Secret OID (AES-128-CCM key for decryption) */
#define TESAIOT_PU_OID_SECRET 0xF1D4

/** Protected Update Key Slot OID */
#define TESAIOT_PU_OID_KEY_SLOT 0xE0F2

/** Protected Update Metadata Slot OID */
#define TESAIOT_PU_OID_METADATA_SLOT 0xE0F3

/** @} */

/*----------------------------------------------------------------------------
 * Payload Type Definitions
 *---------------------------------------------------------------------------*/

/**
 * @brief Protected Update Payload Types
 */
typedef enum {
    TESAIOT_PU_PAYLOAD_TYPE_DATA = 0x01,      /**< Raw data update */
    TESAIOT_PU_PAYLOAD_TYPE_KEY = 0x02,       /**< Key update */
    TESAIOT_PU_PAYLOAD_TYPE_METADATA = 0x03,  /**< Metadata update */
    TESAIOT_PU_PAYLOAD_TYPE_CERT = 0x04       /**< Certificate update */
} tesaiot_pu_payload_type_t;

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_PROTECTED_UPDATE_H */
