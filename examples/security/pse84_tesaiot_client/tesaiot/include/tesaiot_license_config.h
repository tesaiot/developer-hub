/**
 * @file tesaiot_license_config.h
 * @brief TESAIoT License Configuration
 *
 * This file contains your device's license credentials.
 */

#ifndef TESAIOT_LICENSE_CONFIG_H
#define TESAIOT_LICENSE_CONFIG_H

/*============================================================================
 * LICENSE CONFIGURATION
 *============================================================================*/

/**
 * Your OPTIGA Trust M UID (27 bytes as 54 hex characters)
 *
 * TODO: Replace with your device UID (read from Menu Option 1)
 * Example: "CD16339301001C000500000A01BB820003004000AE801010712440"
 */
#define TESAIOT_DEVICE_UID      "YOUR_DEVICE_UID_HERE"

/**
 * Your License Key (ECDSA signature as Base64 string)
 *
 * TODO: Contact TESAIoT support to obtain your license key
 * Example: "MEUCIEjUX7JFWXDpOTXbKLxiNpj5X22d+FFhIGqsZe5UMsZ0AiEA..."
 */
#define TESAIOT_LICENSE_KEY     "YOUR_LICENSE_KEY_HERE"

/*============================================================================
 * END OF CONFIGURATION
 *============================================================================*/

#endif /* TESAIOT_LICENSE_CONFIG_H */
