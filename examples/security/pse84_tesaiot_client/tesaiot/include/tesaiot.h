/**
 * @file tesaiot.h
 * @brief TESAIoT Library - Main Header (Group 6 Umbrella)
 * @copyright (c) 2025 TESAIoT AIoT Foundation Platform
 * @version 2.2
 *
 * TESAIoT Library for PSoC Edge + OPTIGA Trust M
 * Hardware-secured IoT device provisioning and certificate management.
 *
 * Features:
 * - CSR (Certificate Signing Request) workflow
 * - MQTT with mutual TLS authentication
 * - OPTIGA Trust M secure element integration
 * - Protected Update workflow
 * - SNTP time synchronization
 * - Hardware-bound license management
 *
 * Usage:
 * @code
 * #include "tesaiot.h"
 *
 * int main(void) {
 *     // Step 1: Verify license (MUST be called first)
 *     tesaiot_license_status_t lic = tesaiot_license_init();
 *     if (lic != TESAIOT_LICENSE_OK) {
 *         printf("License error: %s\n", tesaiot_license_status_str(lic));
 *         return -1;
 *     }
 *
 *     // Step 2: Use library functions
 *     tesaiot_optiga_init();
 *     // ...
 * }
 * @endcode
 *
 * License:
 * This library is hardware-bound to specific OPTIGA Trust M devices.
 * Contact TESAIoT for licensing: support@tesaiot.com
 *
 * ============================================================================
 * VERSION HISTORY:
 * v1.0 (2026-01-18): Initial creation.
 * v2.2 (2026-01-18): Merged tesaiot_license.h directly into this header.
 * ============================================================================
 */

#ifndef TESAIOT_H
#define TESAIOT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * VERSION INFO - For ABI compatibility checking
 *============================================================================*/

#define TESAIOT_VERSION_MAJOR   2
#define TESAIOT_VERSION_MINOR   8
#define TESAIOT_VERSION_PATCH   0
#define TESAIOT_VERSION_STRING  "2.8.0"

/** Version as single integer for comparison: 0x020100 = 2.1.0 */
#define TESAIOT_VERSION_INT     ((TESAIOT_VERSION_MAJOR << 16) | \
                                 (TESAIOT_VERSION_MINOR << 8) | \
                                 TESAIOT_VERSION_PATCH)

/*============================================================================
 * LICENSE STATUS CODES
 *============================================================================*/

/**
 * @brief License status codes
 */
typedef enum {
    TESAIOT_LICENSE_OK = 0,              /**< License valid, library unlocked */
    TESAIOT_LICENSE_NOT_INITIALIZED,     /**< License not yet checked */
    TESAIOT_LICENSE_INVALID_UID,         /**< Device UID does not match */
    TESAIOT_LICENSE_INVALID_KEY,         /**< Invalid license key signature */
    TESAIOT_LICENSE_INVALID_FORMAT,      /**< Invalid format (UID or key) */
    TESAIOT_LICENSE_NOT_CONFIGURED,      /**< UID or License Key not configured */
    TESAIOT_LICENSE_TRUSTM_ERROR,        /**< Failed to read OPTIGA Trust M */
    TESAIOT_LICENSE_EXPIRED              /**< License expired (future use) */
} tesaiot_license_status_t;

/*============================================================================
 * ERROR CODES
 *============================================================================*/

/**
 * @brief TESAIoT common error codes
 */
typedef enum {
    TESAIOT_OK = 0,                    /**< Success */
    TESAIOT_ERROR_NOT_LICENSED = -100, /**< Library not licensed */
    TESAIOT_ERROR_NOT_INITIALIZED,     /**< Module not initialized */
    TESAIOT_ERROR_INVALID_PARAM,       /**< Invalid parameter */
    TESAIOT_ERROR_TIMEOUT,             /**< Operation timeout */
    TESAIOT_ERROR_OPTIGA,              /**< OPTIGA Trust M error */
    TESAIOT_ERROR_MEMORY,              /**< Memory allocation failed */
    TESAIOT_ERROR_NETWORK,             /**< Network error */
    TESAIOT_ERROR_TLS,                 /**< TLS error */
    TESAIOT_ERROR_MQTT,                /**< MQTT error */
    TESAIOT_ERROR_INTERNAL             /**< Internal error */
} tesaiot_error_t;

/*============================================================================
 * LICENSE GATE MACROS - For enforcing license in functions
 *============================================================================*/

/**
 * @brief License enforcement gate - returns false if not licensed
 *
 * Use at the start of protected functions:
 * @code
 * bool my_protected_function(void) {
 *     TESAIOT_LICENSE_GATE_BOOL();
 *     // ... function implementation
 * }
 * @endcode
 */
#define TESAIOT_LICENSE_GATE_BOOL() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return false; \
        } \
    } while(0)

/**
 * @brief License enforcement gate - returns -1 if not licensed
 */
#define TESAIOT_LICENSE_GATE_INT() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return -1; \
        } \
    } while(0)

/**
 * @brief License enforcement gate - returns NULL if not licensed
 */
#define TESAIOT_LICENSE_GATE_PTR() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return NULL; \
        } \
    } while(0)

/**
 * @brief License enforcement gate - returns void (just exits)
 */
#define TESAIOT_LICENSE_GATE_VOID() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return; \
        } \
    } while(0)

/**
 * @brief License enforcement gate - returns custom value
 */
#define TESAIOT_LICENSE_GATE(ret_val) \
    do { \
        if (!tesaiot_is_licensed()) { \
            return (ret_val); \
        } \
    } while(0)

/**
 * @brief Macro to check license before function execution
 * Returns TESAIOT_ERROR_NOT_LICENSED if library is not licensed
 */
#define TESAIOT_CHECK_LICENSE() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return TESAIOT_ERROR_NOT_LICENSED; \
        } \
    } while(0)

/**
 * @brief Macro for void functions - just returns if not licensed
 */
#define TESAIOT_CHECK_LICENSE_VOID() \
    do { \
        if (!tesaiot_is_licensed()) { \
            return; \
        } \
    } while(0)

/*============================================================================
 * LICENSE API FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize and verify TESAIoT library license
 *
 * This function reads the OPTIGA Trust M Factory UID and verifies
 * it against the licensed UID embedded in this library.
 *
 * MUST be called once at startup before using any other tesaiot_* functions.
 * All other functions will fail if license is not verified.
 *
 * @return tesaiot_license_status_t License verification result
 *
 * @code
 * // Example usage
 * tesaiot_license_status_t status = tesaiot_license_init();
 * if (status != TESAIOT_LICENSE_OK) {
 *     printf("License error: %s\n", tesaiot_license_status_str(status));
 *     // Library functions will NOT work!
 * }
 * @endcode
 */
tesaiot_license_status_t tesaiot_license_init(void);

/**
 * @brief Check if library is currently licensed
 *
 * @return true if license is valid, false otherwise
 * @note All tesaiot_* functions check this internally and will fail if false
 */
bool tesaiot_is_licensed(void);

/**
 * @brief Get the device's OPTIGA Trust M Factory UID
 *
 * Useful for registration: customer reads UID and sends to TESAIoT
 * to receive a licensed library build.
 *
 * @param[out] uid_buf   Buffer to store UID (minimum 27 bytes)
 * @param[in,out] uid_len  Input: buffer size, Output: actual UID length
 * @return true on success, false on failure
 *
 * @note This function works even without valid license (for registration)
 */
bool tesaiot_get_device_uid(uint8_t* uid_buf, uint16_t* uid_len);

/**
 * @brief Print device UID to console (for registration)
 *
 * Prints the OPTIGA Trust M Factory UID in hex format.
 * Customer can send this to TESAIoT for license generation.
 *
 * @note This function works even without valid license (for registration)
 */
void tesaiot_print_device_uid(void);

/**
 * @brief Get license status string
 *
 * @param status License status code
 * @return Human-readable status string
 */
const char* tesaiot_license_status_str(tesaiot_license_status_t status);

/**
 * @brief Get library version
 *
 * @return Version as integer (e.g., 0x020000 for 2.0.0)
 */
uint32_t tesaiot_get_version(void);

/**
 * @brief Get library version string
 *
 * @return Version string (e.g., "2.0.0")
 */
const char* tesaiot_get_version_string(void);

/**
 * @brief Get error string for error code
 * @param error Error code
 * @return Human-readable error string
 */
const char* tesaiot_error_str(tesaiot_error_t error);

/*******************************************************************************
 * Configuration (includes debug settings, OID mappings, license config)
 ******************************************************************************/
#include "tesaiot_config.h"

/*******************************************************************************
 * Domain Headers (5 Groups)
 *
 * Users only need to include tesaiot.h - all domains available automatically:
 * 1. tesaiot_optiga_core.h  - Low-level OPTIGA hardware
 * 2. tesaiot_csr.h          - CSR workflow
 * 3. tesaiot_protected_update.h - Protected Update workflow
 * 4. tesaiot_optiga.h       - TESAIoT OPTIGA integration
 * 5. tesaiot_platform.h     - Standard Libs (MQTT, SNTP)
 ******************************************************************************/

/* Group 1: Core OPTIGA instance management */
#include "tesaiot_optiga_core.h"

/* Group 2: CSR Workflow */
#include "tesaiot_csr.h"

/* Group 3: Protected Update Workflow */
#include "tesaiot_protected_update.h"

/* Group 4: TESAIoT OPTIGA Trust M integration */
#include "tesaiot_optiga.h"

/* Group 5: Platform services (MQTT, SNTP) */
#include "tesaiot_platform.h"

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_H */
