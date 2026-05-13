/**
 * @file tesaiot.h
 * @brief TESAIoT Library Public API for Raspberry Pi / Embedded Linux
 * @version 1.0
 * @date 2026-01-22
 *
 * @description
 * This library provides license verification and device authentication
 * for TESAIoT IoT Platform. It verifies that the device has a valid
 * license key from TESAIoT Platform before allowing access to CSR
 * workflow and Protected Update services.
 *
 * @dependencies
 * - OPTIGA Trust M library (libtrustm)
 * - mbedTLS library (libmbedtls, libmbedx509, libmbedcrypto)
 * - pthread
 *
 * @usage
 * 1. Configure tesaiot_config.h with device UID and license key
 * 2. Link application with libtesaiot.a
 * 3. Call tesaiot_init() at application startup
 * 4. Call tesaiot_verify_license() to verify license
 * 5. Call tesaiot_deinit() at application shutdown
 */

#ifndef TESAIOT_H
#define TESAIOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================
 * Version Information
 *===========================================================================*/

#define TESAIOT_VERSION_MAJOR   1
#define TESAIOT_VERSION_MINOR   0
#define TESAIOT_VERSION_PATCH   0

/*============================================================================
 * Return Codes (errno convention)
 *===========================================================================*/

/**
 * @brief Return codes follow Linux kernel errno convention
 *
 * Success: 0
 * Error: -EXXX (negative errno value)
 *
 * Common error codes:
 * - -EINVAL: Invalid argument
 * - -EACCES: License verification failed
 * - -EIO: I/O error (Trust M communication)
 * - -ENOMEM: Out of memory
 * - -ETIMEDOUT: Operation timeout
 */

/*============================================================================
 * License Status
 *===========================================================================*/

/**
 * @brief License verification status
 */
typedef enum {
    TESAIOT_LICENSE_VALID = 0,          /**< License is valid */
    TESAIOT_LICENSE_INVALID_UID,        /**< UID mismatch */
    TESAIOT_LICENSE_INVALID_SIGNATURE,  /**< Signature verification failed */
    TESAIOT_LICENSE_TRUSTM_ERROR,       /**< Trust M communication error */
    TESAIOT_LICENSE_PARSE_ERROR,        /**< License key parsing error */
    TESAIOT_LICENSE_NOT_VERIFIED        /**< License not yet verified */
} tesaiot_license_status_t;

/*============================================================================
 * Device Information
 *===========================================================================*/

/**
 * @brief Device information structure
 */
typedef struct {
    char uid[55];                       /**< Device UID (54 hex chars + null) */
    const char *device_id;              /**< Device ID (UUID from config) */
    char license_key[256];              /**< License key (Base64) */
    tesaiot_license_status_t status;    /**< License verification status */
    bool initialized;                   /**< Library initialization status */
} tesaiot_device_info_t;

/*============================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief Initialize TESAIoT library
 *
 * @details
 * - Initializes OPTIGA Trust M
 * - Initializes mbedTLS
 * - Prepares license verification system
 *
 * @return 0 on success, negative errno on failure
 *
 * @note Must be called before any other tesaiot_* functions
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_init(void);

/**
 * @brief Deinitialize TESAIoT library
 *
 * @details
 * - Releases OPTIGA Trust M resources
 * - Frees mbedTLS contexts
 * - Clears sensitive data from memory
 *
 * @return 0 on success, negative errno on failure
 *
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_deinit(void);

/**
 * @brief Reinitialize OPTIGA Trust M (preserving license status)
 *
 * @details
 * Full OPTIGA reinit cycle: close application → destroy instances →
 * recreate instances → reopen application. License status is preserved.
 *
 * Use after MQTT disconnect with trustm_provider to reset OPTIGA I2C state.
 *
 * @return 0 on success, negative errno on failure
 *
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_reinit(void);

/**
 * @brief Verify device license
 *
 * @details
 * Verification process:
 * 1. Read actual UID from Trust M OID 0xE0C2
 * 2. Compare with configured UID (constant-time comparison)
 * 3. Parse license key from Base64
 * 4. Verify ECDSA signature using TESAIoT Platform public key
 *
 * @return 0 on success (license valid), negative errno on failure
 *
 * @retval 0: License valid, device authorized
 * @retval -EACCES: License verification failed
 * @retval -EIO: Trust M communication error
 * @retval -EINVAL: Invalid license format
 *
 * @note This function MUST be called and return 0 before using CSR or PU workflows
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_verify_license(void);

/**
 * @brief Get device information
 *
 * @param[out] info Pointer to device info structure
 * @return 0 on success, negative errno on failure
 *
 * @retval 0: Success
 * @retval -EINVAL: NULL pointer
 * @retval -EAGAIN: Library not initialized
 *
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_get_device_info(tesaiot_device_info_t *info);

/**
 * @brief Get license verification status
 *
 * @return License verification status
 *
 * @note Thread-safe (uses internal mutex)
 */
tesaiot_license_status_t tesaiot_get_license_status(void);

/**
 * @brief Read Trust M UID directly
 *
 * @param[out] uid_hex Buffer to store UID (55 bytes minimum: 54 hex + null)
 * @param[in] uid_hex_size Size of uid_hex buffer
 * @return 0 on success, negative errno on failure
 *
 * @retval 0: Success
 * @retval -EINVAL: NULL pointer or buffer too small
 * @retval -EIO: Trust M read error
 *
 * @note Thread-safe (uses internal mutex)
 */
int tesaiot_read_trustm_uid(char *uid_hex, size_t uid_hex_size);

/**
 * @brief Get library version
 *
 * @param[out] major Major version
 * @param[out] minor Minor version
 * @param[out] patch Patch version
 * @return 0 on success, negative errno on failure
 */
int tesaiot_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

/*============================================================================
 * Logging Control (Optional)
 *===========================================================================*/

/**
 * @brief Log levels
 * @note If tesaiot_utils.h is included first, this is already defined
 */
#ifndef TESAIOT_LOG_LEVEL_DEFINED
#define TESAIOT_LOG_LEVEL_DEFINED
typedef enum {
    TESAIOT_LOG_LEVEL_ERROR = 0,
    TESAIOT_LOG_LEVEL_WARN,
    TESAIOT_LOG_LEVEL_INFO,
    TESAIOT_LOG_LEVEL_DEBUG
} tesaiot_log_level_t;
#endif

/**
 * @brief Set log level
 *
 * @param level Log level
 */
void tesaiot_set_log_level(tesaiot_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_H */
