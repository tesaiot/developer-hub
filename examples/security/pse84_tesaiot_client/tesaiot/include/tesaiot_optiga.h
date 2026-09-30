/**
 * @file tesaiot_optiga.h
 * @brief TESAIoT OPTIGA Trust M integration — the part of the prebuilt
 *        library's interface that this example application uses.
 *
 * Part of TESAIoT Firmware SDK.
 *
 * Only the entry points this example calls are declared here. The state
 * values keep the numbers the prebuilt libtesaiot.a was built with, so the
 * interface is binary-compatible with it.
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
 * Workflow progress reporting
 *---------------------------------------------------------------------------*/

/** Workflow steps the example reports. Values are fixed by the library ABI. */
typedef enum
{
    TRUSTM_STATE_PROCESSING_JSON_BUNDLE   = 8,
    TRUSTM_STATE_WRITING_TRUST_ANCHOR     = 9,
    TRUSTM_STATE_VERIFYING_MANIFEST       = 10,
    TRUSTM_STATE_PROTECTED_UPDATE_SUCCESS = 12,
    TRUSTM_STATE_PROTECTED_UPDATE_FAILED  = 13
} trustm_state_t;

/**
 * Report a workflow step to the library.
 * @param step    the step now entered
 * @param code    optional status code (may be NULL)
 * @param message optional detail text (may be NULL)
 */
void
trustm_update_state(trustm_state_t step,
                    const char *code,
                    const char *message);

/** Correlation id of the platform request the library is tracking. */
const char *
trustm_current_correlation_id(void);

/*----------------------------------------------------------------------------
 * MQTT certificate selection
 *---------------------------------------------------------------------------*/

/** OID of the certificate the MQTT TLS session should present. */
uint16_t
tesaiot_select_mqtt_certificate(void);

/** true while the session runs on the fallback (factory) certificate. */
bool
tesaiot_is_using_fallback_certificate(void);

/** Ask the library to start a CSR renewal; true if one was started. */
bool
tesaiot_auto_trigger_csr_renewal(void);

/** Clear the library's fallback-certificate bookkeeping. */
void
tesaiot_reset_fallback_state(void);

/*----------------------------------------------------------------------------
 * Convenience wrappers over this example's optiga_trust_helpers
 *---------------------------------------------------------------------------*/

static inline bool
tesaiot_read_factory_uid(char *uid_hex, size_t uid_hex_len)
{
    return optiga_read_factory_uid(uid_hex, uid_hex_len);
}

static inline bool
tesaiot_read_factory_certificate(char *cert_pem, uint16_t *cert_pem_length)
{
    return optiga_read_factory_certificate(cert_pem, cert_pem_length);
}

static inline optiga_lib_status_t
tesaiot_test_metadata_operations(void)
{
    return test_metadata_operations();
}

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_OPTIGA_H */
