/**
 * @file tesaiot_csr.h
 * @brief TESAIoT CSR Workflow Public API
 * @version 2.0
 * @date 2026-01-23
 *
 * Certificate Signing Request (CSR) workflow for TESAIoT Platform
 * Implements 8-state CSR enrollment process with OPTIGA Trust M
 *
 * Architecture: Library-first design (PSoC E84 reference pattern)
 */

#ifndef TESAIOT_CSR_H
#define TESAIOT_CSR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * CSR Workflow States
 *---------------------------------------------------------------------------*/

/**
 * @brief CSR Workflow State Machine States
 *
 * States based on PSoC Edge E84 v2.1.0 reference implementation
 */
typedef enum {
    TESAIOT_CSR_STATE_IDLE = 0,              /**< Waiting for start command */
    TESAIOT_CSR_STATE_GENERATE_KEYPAIR,      /**< Generating ECC P-256 keypair at OID 0xE0F1 */
    TESAIOT_CSR_STATE_GENERATE_CSR,          /**< Generating CSR with device UID */
    TESAIOT_CSR_STATE_CONNECT_MQTT,          /**< Connecting to MQTT broker (Factory Cert) */
    TESAIOT_CSR_STATE_PUBLISH_CSR,           /**< Publishing CSR to device/{id}/commands/csr */
    TESAIOT_CSR_STATE_WAIT_CERTIFICATE,      /**< Waiting for certificate from platform */
    TESAIOT_CSR_STATE_DONE,                  /**< Workflow completed successfully */
    TESAIOT_CSR_STATE_ERROR                  /**< Workflow failed */
} tesaiot_csr_state_t;

/*----------------------------------------------------------------------------
 * CSR Workflow Configuration
 *---------------------------------------------------------------------------*/

/** Maximum lengths for configuration strings */
#define TESAIOT_CSR_MAX_DEVICE_ID_LEN    64
#define TESAIOT_CSR_MAX_DEVICE_UID_LEN   128   /**< Trust M UID (54 hex chars + null) */
#define TESAIOT_CSR_MAX_BROKER_URL_LEN   256
#define TESAIOT_CSR_MAX_PATH_LEN         256
#define TESAIOT_CSR_MAX_PUBKEY_LEN       100
#define TESAIOT_CSR_MAX_CSR_LEN          1024
#define TESAIOT_CSR_MAX_CERT_LEN         2048

/**
 * @brief CSR Workflow Configuration Structure
 *
 * Configuration parameters for CSR workflow initialization
 *
 * PSoC E84 Reference Pattern:
 * - device_id (UUID): Used for MQTT topics and username
 * - device_uid (Trust M UID): Used for MQTT client identifier
 */
typedef struct {
    char device_id[TESAIOT_CSR_MAX_DEVICE_ID_LEN];       /**< Device UUID for topic routing and MQTT username */
    char device_uid[TESAIOT_CSR_MAX_DEVICE_UID_LEN];     /**< Trust M UID (OID 0xE0C2) for MQTT client identifier */
    char mqtt_broker_url[TESAIOT_CSR_MAX_BROKER_URL_LEN]; /**< MQTT broker URL (e.g., "ssl://mqtt.tesaiot.com:8883") */
    char ca_cert_path[TESAIOT_CSR_MAX_PATH_LEN];         /**< Path to CA certificate PEM file */
    char factory_cert_path[TESAIOT_CSR_MAX_PATH_LEN];    /**< Path to factory certificate PEM file */
    char factory_key_path[TESAIOT_CSR_MAX_PATH_LEN];     /**< Path to factory key file */
    uint16_t target_oid;                                  /**< Target OID for device certificate (0xE0E1-0xE0E3, default: 0xE0E2) */
} tesaiot_csr_workflow_config_t;

/**
 * @brief CSR Workflow Context (Stack Allocated)
 *
 * Internal context for CSR workflow (~700 bytes)
 * Allocated on stack to reduce heap pressure during TLS handshake
 */
typedef struct {
    uint8_t public_key_der[TESAIOT_CSR_MAX_PUBKEY_LEN];  /**< Generated public key (DER format) */
    uint16_t pubkey_len;                                  /**< Actual public key length */
    char csr_pem[TESAIOT_CSR_MAX_CSR_LEN];               /**< Generated CSR in PEM format */
    char certificate_pem[TESAIOT_CSR_MAX_CERT_LEN];      /**< Received certificate in PEM format */
    bool certificate_received;                            /**< Flag indicating certificate reception */
    int certificate_write_status;                         /**< Status of certificate write to OID 0xE0E1 */
} tesaiot_csr_context_t;

/**
 * @brief Certificate Reception Status
 *
 * Status information for certificate reception
 */
typedef struct {
    const char *pem_cert;    /**< Pointer to PEM certificate */
    size_t pem_len;          /**< Length of PEM certificate */
    int write_status;        /**< Write status (0=success) */
} tesaiot_cert_reception_status_t;

/*----------------------------------------------------------------------------
 * Public API Functions
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize CSR workflow with configuration
 *
 * @param config Pointer to workflow configuration structure
 * @return 0 on success, negative error code on failure
 *         -1: Invalid config parameter
 *         -2: Missing device_id
 */
int tesaiot_csr_workflow_init(const tesaiot_csr_workflow_config_t *config);

/**
 * @brief Start CSR workflow
 *
 * Transitions state machine from IDLE to GENERATE_KEYPAIR
 *
 * @return 0 on success, negative error code on failure
 *         -1: Workflow not initialized
 *         -2: Workflow already started
 */
int tesaiot_csr_workflow_start(void);

/**
 * @brief Run one iteration of CSR workflow state machine
 *
 * Non-blocking function that advances state machine by one step
 * Should be called repeatedly until workflow completes
 *
 * @param context Pointer to stack-allocated context
 * @return 1 if workflow completed successfully,
 *         0 if workflow should continue (call again),
 *         negative error code on failure
 */
int tesaiot_csr_workflow_run(tesaiot_csr_context_t *context);

/**
 * @brief Get current workflow state
 *
 * @return Current state of CSR workflow state machine
 */
tesaiot_csr_state_t tesaiot_csr_workflow_get_state(void);

/**
 * @brief Get human-readable state name
 *
 * @param state State to convert to string
 * @return Pointer to static string with state name
 */
const char *tesaiot_csr_workflow_get_state_name(tesaiot_csr_state_t state);

/**
 * @brief Run complete CSR workflow (blocking wrapper)
 *
 * Convenience function that runs entire CSR workflow from start to finish.
 * This is a blocking call that may take 30-60 seconds to complete.
 *
 * @param config Pointer to CSR workflow configuration
 * @return 0 on success, negative error code on failure
 *
 * @note This function allocates ~700 bytes of context on the stack
 * @note Uses Factory Certificate for initial MQTT connection
 */
int tesaiot_csr_workflow_run_blocking(const tesaiot_csr_workflow_config_t *config);

/**
 * @brief Notify CSR workflow of certificate reception (internal)
 *
 * Called by subscriber thread when certificate is received from platform
 *
 * @param pem_cert Pointer to certificate in PEM format
 * @param pem_len Length of PEM certificate
 * @return 0 on success, negative on failure
 *
 * @note This is an internal API called by MQTT subscriber thread
 */
int tesaiot_csr_workflow_notify_certificate(const char *pem_cert, size_t pem_len);

/*----------------------------------------------------------------------------
 * CSR Workflow Constants
 *---------------------------------------------------------------------------*/

/** Maximum number of state machine iterations (safety limit) */
#define TESAIOT_CSR_MAX_ITERATIONS 1000

/** Certificate wait timeout (milliseconds) */
#define TESAIOT_CSR_CERT_WAIT_TIMEOUT 60000

/** State machine delay between iterations (milliseconds) */
#define TESAIOT_CSR_SM_DELAY 100

/** Maximum device UID length (hex string with null terminator) */
#define TESAIOT_UID_MAX_LEN 128

/*----------------------------------------------------------------------------
 * OID Definitions (CSR Workflow)
 * Note: Primary definitions are in tesaiot_config.h
 *       These are fallback definitions with #ifndef guards
 *---------------------------------------------------------------------------*/

/** CSR Private Key OID (ECC P-256 keypair storage) */
#ifndef TESAIOT_OID_CSR_KEY
#define TESAIOT_OID_CSR_KEY 0xE0F1
#endif

/**
 * Device Certificate OID (target for certificate write)
 * @warning OID 0xE0E1 was accidentally locked with Change:NEV on 2026-01-30.
 *          Now using 0xE0E2 for device certificate storage.
 */
#ifndef TESAIOT_OID_DEVICE_CERT
#define TESAIOT_OID_DEVICE_CERT 0xE0E2  /* Changed from 0xE0E1 (locked) */
#endif

/** TESAIoT CA Chain OID (for certificate verification) */
#ifndef TESAIOT_OID_CA_CHAIN
#define TESAIOT_OID_CA_CHAIN 0xE0E9
#endif

/** Factory Certificate OID (for bootstrap TLS) */
#ifndef TESAIOT_OID_FACTORY_CERT
#define TESAIOT_OID_FACTORY_CERT 0xE0E0
#endif

/** Factory Key OID (for bootstrap TLS) */
#ifndef TESAIOT_OID_FACTORY_KEY
#define TESAIOT_OID_FACTORY_KEY 0xE0F0
#endif

/** Trust M UID OID (unique device identifier) */
#ifndef TESAIOT_OID_TRUSTM_UID
#define TESAIOT_OID_TRUSTM_UID 0xE0C2
#endif

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_CSR_H */
