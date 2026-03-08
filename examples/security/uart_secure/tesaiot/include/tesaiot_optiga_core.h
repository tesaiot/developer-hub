/**
 * @file tesaiot_optiga_core.h
 * @brief TESAIoT Core OPTIGA Trust M - Umbrella Header (Group 1)
 * @version 2.1
 * @date 2026-01-18
 *
 * Part of TESAIoT Firmware SDK
 *
 * This umbrella header provides access to low-level OPTIGA Trust M operations:
 * - OPTIGA instance manager (singleton pattern, thread-safe)
 * - Mutex-protected access to hardware
 *
 * Domain Group: Core Optiga Trust M (Low-level hardware access)
 *
 * Usage:
 * @code
 * #include "tesaiot_optiga_core.h"
 *
 * // Initialize OPTIGA instance at boot
 * optiga_manager_init(callback, context);
 *
 * // Thread-safe access
 * optiga_util_t *me = optiga_manager_acquire();
 * // ... use OPTIGA ...
 * optiga_manager_release();
 * @endcode
 *
 * @note This is for low-level OPTIGA access. For TESAIoT certificate
 *       operations, use tesaiot_optiga.h instead.
 */

#ifndef TESAIOT_OPTIGA_CORE_H
#define TESAIOT_OPTIGA_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Required Includes for OPTIGA Types
 *---------------------------------------------------------------------------*/
#include "include/optiga_util.h"
#include "include/optiga_crypt.h"
#include "include/common/optiga_lib_common.h"
#include "FreeRTOS.h"
#include "semphr.h"

/*----------------------------------------------------------------------------
 * OPTIGA Instance Manager API
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize the persistent OPTIGA Trust M instance
 *
 * Must be called once at system startup (e.g., in subscriber_task initialization).
 * Creates a single global OPTIGA instance that will be reused for all operations.
 *
 * @param callback Async operation completion callback (required)
 * @param context Caller context for callback (optional, can be NULL)
 * @return true if initialization successful, false otherwise
 */
bool optiga_manager_init(callback_handler_t callback, void *context);

/**
 * @brief Get the persistent OPTIGA instance (thread-safe)
 *
 * Acquires mutex lock and returns the global OPTIGA instance.
 * MUST call optiga_manager_release() after use to release mutex.
 *
 * Example usage:
 *   optiga_util_t *me = optiga_manager_acquire();
 *   if (me) {
 *       // Use me for OPTIGA operations
 *       optiga_util_write_data(me, ...);
 *       optiga_manager_release();
 *   }
 *
 * @return Pointer to OPTIGA instance, or NULL if not initialized
 */
optiga_util_t* optiga_manager_acquire(void);

/**
 * @brief Lock the OPTIGA mutex for exclusive access (thread-safe)
 *
 * Use this when you need to create your own temporary crypt instance.
 * Pattern: lock -> create crypt -> use -> destroy crypt -> unlock
 * MUST call optiga_manager_unlock() after use to release mutex.
 *
 * @return true if lock acquired, false on timeout or error
 */
bool optiga_manager_lock(void);

/**
 * @brief Unlock the OPTIGA mutex (thread-safe)
 *
 * MUST be called after optiga_manager_lock() to release mutex lock.
 */
void optiga_manager_unlock(void);

/**
 * @brief Release the OPTIGA instance mutex (thread-safe)
 *
 * MUST be called after optiga_manager_acquire() to release mutex lock.
 * Failure to call this will cause deadlock!
 */
void optiga_manager_release(void);

/**
 * @brief Check if OPTIGA instance is initialized
 *
 * @return true if instance is ready, false otherwise
 */
bool optiga_manager_is_initialized(void);

/**
 * @brief Cleanup OPTIGA instance at system shutdown (optional)
 *
 * Destroys the global OPTIGA instance and releases resources.
 * Typically only called at system shutdown, not during normal operation.
 */
void optiga_manager_cleanup(void);

/**
 * @brief Create a temporary optiga_crypt_t instance for crypto operations
 *
 * MUST be called after optiga_manager_lock().
 * Caller MUST destroy the instance with optiga_crypt_destroy() before unlocking.
 *
 * Usage pattern:
 * @code
 *   optiga_manager_lock();
 *   optiga_crypt_t *crypt = optiga_manager_create_crypt(callback, context);
 *   // ... use crypt for crypto operations ...
 *   optiga_crypt_destroy(crypt);
 *   optiga_manager_unlock();
 * @endcode
 *
 * @param callback  Async completion callback
 * @param context   Callback context (can be NULL)
 * @return optiga_crypt_t* or NULL on error
 */
optiga_crypt_t* optiga_manager_create_crypt(callback_handler_t callback, void *context);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_OPTIGA_CORE_H */
