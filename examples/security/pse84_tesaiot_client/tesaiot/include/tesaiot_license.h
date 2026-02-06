/**
 * @file tesaiot_license.h
 * @brief TESAIoT Library License Public API
 * @copyright (c) 2025 TESAIoT AIoT Foundation Platform
 */

#ifndef TESAIOT_LICENSE_H
#define TESAIOT_LICENSE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Version Information
 *============================================================================*/

#define TESAIOT_VERSION_MAJOR 2
#define TESAIOT_VERSION_MINOR 8
#define TESAIOT_VERSION_PATCH 0
#define TESAIOT_VERSION_STRING "2.8.0"
#define TESAIOT_VERSION_INT     ((TESAIOT_VERSION_MAJOR << 16) | \
                                  (TESAIOT_VERSION_MINOR << 8) | \
                                   TESAIOT_VERSION_PATCH)

/*============================================================================
 * License Status
 *============================================================================*/

typedef enum {
    TESAIOT_LICENSE_OK = 0,               /* License valid */
    TESAIOT_LICENSE_ERROR_OPTIGA = -1,    /* OPTIGA hardware error */
    TESAIOT_LICENSE_INVALID_CONFIG = -2,  /* Invalid configuration */
    TESAIOT_LICENSE_INVALID_UID = -3,     /* UID mismatch */
    TESAIOT_LICENSE_INVALID_KEY = -4,     /* Invalid license key */
    TESAIOT_LICENSE_NOT_INITIALIZED = -5  /* Not initialized yet */
} tesaiot_license_status_t;

/*============================================================================
 * Public API
 *============================================================================*/

/**
 * @brief Initialize and verify TESAIoT license
 *
 * This function:
 * 1. Reads device UID from OPTIGA Trust M (OID 0xE0C2)
 * 2. Verifies UID matches configured value (from tesaiot_device_uid)
 * 3. Verifies ECDSA signature of UID (from tesaiot_license_key)
 *
 * @return tesaiot_license_status_t status code
 */
tesaiot_license_status_t tesaiot_license_init(void);

/**
 * @brief Check if license is valid
 *
 * @return true if license is valid, false otherwise
 */
bool tesaiot_license_is_valid(void);

/**
 * @brief Get human-readable status string
 *
 * @param status License status code
 * @return Const string describing the status
 */
const char* tesaiot_license_status_str(tesaiot_license_status_t status);

/**
 * @brief Print device UID (for registration)
 *
 * Prints the device's OPTIGA Trust M UID to console.
 * Use this to obtain the UID for registering on TESAIoT Platform.
 */
void tesaiot_print_device_uid(void);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_LICENSE_H */
