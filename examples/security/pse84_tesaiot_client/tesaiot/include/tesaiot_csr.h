/**
 * @file tesaiot_csr.h
 * @brief TESAIoT CSR Workflow - Umbrella Header (Group 2)
 * @version 2.1
 * @date 2026-01-18
 *
 * Part of TESAIoT Firmware SDK
 *
 * This umbrella header provides access to Certificate Signing Request workflow:
 * - CSR generation state machine
 * - Keypair generation in OPTIGA
 * - CSR submission via MQTT
 * - Certificate reception and storage
 *
 * Domain Group: CSR Workflow (Certificate provisioning)
 *
 * Usage:
 * @code
 * #include "tesaiot_csr.h"
 *
 * tesaiot_csr_workflow_config_t config = {
 *     .device_id = "device_uid_here"
 * };
 * tesaiot_csr_workflow_init(&config);
 * tesaiot_csr_workflow_run();
 * @endcode
 *
 * @note This workflow generates a new keypair in OPTIGA Trust M,
 *       creates a CSR, and submits it to TESAIoT Platform for signing.
 */

#ifndef TESAIOT_CSR_H
#define TESAIOT_CSR_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Types
 *---------------------------------------------------------------------------*/

/**
 * CSR Workflow States
 */
typedef enum {
    TESAIOT_CSR_STATE_IDLE = 0,
    TESAIOT_CSR_STATE_GENERATE_KEYPAIR,
    TESAIOT_CSR_STATE_GENERATE_CSR,
    TESAIOT_CSR_STATE_CONNECT_MQTT,
    TESAIOT_CSR_STATE_PUBLISH_CSR,
    TESAIOT_CSR_STATE_WAIT_CERTIFICATE,
    TESAIOT_CSR_STATE_VALIDATE_CERT,
    TESAIOT_CSR_STATE_WRITE_TO_OPTIGA,
    TESAIOT_CSR_STATE_DONE,
    TESAIOT_CSR_STATE_ERROR,
} tesaiot_csr_state_t;

/**
 * CSR Workflow Configuration
 */
typedef struct {
    const char *device_id;   /**< Device unique identifier (e.g. UID) */
} tesaiot_csr_workflow_config_t;

/*----------------------------------------------------------------------------
 * Public API Functions
 *---------------------------------------------------------------------------*/

/**
 * Initialize CSR workflow
 * @param config Configuration structure
 * @return 0 on success, negative on error
 */
int tesaiot_csr_workflow_init(const tesaiot_csr_workflow_config_t *config);

/**
 * Get current workflow state
 * @return Current state
 */
tesaiot_csr_state_t tesaiot_csr_workflow_get_state(void);

/**
 * Start CSR workflow (transition to first state)
 * @return 0 on success, negative on error
 */
int tesaiot_csr_workflow_start(void);

/**
 * Run one iteration of CSR workflow state machine
 * @return 0 on success, negative on error, 1 if workflow complete
 */
int tesaiot_csr_workflow_run(void);

/**
 * Run complete CSR workflow (legacy wrapper)
 * @return 0 on success, negative on error
 */
int tesaiot_run_csr_workflow(void);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_CSR_H */
