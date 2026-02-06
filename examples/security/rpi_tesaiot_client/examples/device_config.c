/**
 * @file device_config.c
 * @brief Device-specific configuration (credentials from TESAIoT Platform)
 *
 * This file defines the device-specific globals required by libtesaiot.a
 * Values are from credentials/factory_metadata.json
 */

/**
 * @brief Device ID (UUID) from TESAIoT Platform
 */
const char *g_tesaiot_device_id = "a8a1c4f3-42ae-41e2-b6d3-1f74cc3d5b31";

/**
 * @brief Device UID from OPTIGA Trust M (OID 0xE0C2)
 */
const char *g_tesaiot_device_uid = "CD16339301001C000500000A01BB820003009C002C801010712440";

/**
 * @brief License Key from TESAIoT Platform
 * Note: License verification is optional - set to empty string if not using license feature
 */
const char *g_tesaiot_license_key = "";
