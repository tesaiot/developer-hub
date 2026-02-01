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
 */
#define TESAIOT_DEVICE_UID      "<YOUR_TRUST_M_UID>"

/**
 * Your License Key (ECDSA signature as Base64 string)
 */
#define TESAIOT_LICENSE_KEY     "<YOUR_DEVICE_LICENSE_KEY>"

/*============================================================================
 * END OF CONFIGURATION
 *============================================================================*/

#endif /* TESAIOT_LICENSE_CONFIG_H */
