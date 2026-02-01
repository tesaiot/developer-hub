/**
 * @file tesaiot_protected_update.h
 * @brief TESAIoT Protected Update Workflow - Umbrella Header (Group 3)
 * @version 2.1
 * @date 2026-01-18
 *
 * Part of TESAIoT Firmware SDK
 *
 * This umbrella header provides access to Protected Update operations:
 * - Full protected update workflow (with TESAIoT Platform)
 * - Isolated testing mode (without platform connectivity)
 *
 * Domain Group: Protected Update Workflow (Secure certificate/key updates)
 *
 * Usage:
 * @code
 * #include "tesaiot_protected_update.h"
 *
 * // Full workflow with TESAIoT Platform
 * tesaiot_protected_update_start();
 *
 * // Or isolated testing (Menu 8)
 * tesaiot_protected_update_isolated_run();
 * @endcode
 *
 * @note Protected Update uses OPTIGA's secure update mechanism with
 *       signed manifests and encrypted payloads from TESAIoT Platform.
 */

#ifndef TESAIOT_PROTECTED_UPDATE_H
#define TESAIOT_PROTECTED_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Protected Update Workflow (Full - with Platform)
 *---------------------------------------------------------------------------*/

/**
 * @brief Execute Protected Update workflow for certificate renewal (Option A).
 *
 * This function implements certificate renewal using the existing keypair
 * from Menu 4 (OID 0xE0F1). It does NOT generate a new keypair.
 *
 * The workflow:
 * 1. Reads device UID
 * 2. Requests certificate renewal from TESAIoT Platform
 * 3. Platform generates new certificate for existing public key
 * 4. Platform sends Protected Update manifest + fragment
 * 5. Device applies Protected Update to 0xE0E1 (certificate slot)
 *
 * @note Requires Menu 4 (CSR Workflow) to be executed first.
 */
void tesaiot_run_protected_update_workflow(void);

/*----------------------------------------------------------------------------
 * Protected Update Isolated (Testing - without Platform)
 *---------------------------------------------------------------------------*/

/**
 * @brief Run Protected Update Isolated Test (Menu 8).
 *
 * Provides interactive sub-menu:
 *   1) Verify OPTIGA Configuration (read-only diagnostic)
 *   2) Run Protected Update Workflow (5-step test)
 *   0) Back to Main Menu
 *
 * Useful for factory testing and diagnostics without network.
 */
void tesaiot_run_protected_update_isolated_test(void);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_PROTECTED_UPDATE_H */
