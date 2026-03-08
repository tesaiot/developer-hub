/**
 * @file mqtt_device_config_data.c
 * @brief MQTT Device Configuration Data Binding
 *
 * This file provides device-specific configuration values that are bound at
 * link time. Customer compiles this file with their own values from
 * mqtt_client_config.h.
 *
 * Architecture:
 * - Library contains extern declarations (no hardcoded values)
 * - Customer compiles this file with their DEVICE_ID and API_KEY
 * - Linker binds customer values to library at link time
 *
 * Similar to tesaiot_license_data.c pattern.
 */

#include "mqtt_client_config.h"

/*============================================================================
 * Device Configuration Binding
 *
 * These values are compiled from customer's mqtt_client_config.h
 *============================================================================*/

/**
 * MQTT Device ID (UUID format)
 * Used for topic construction: device/{device_id}/commands/...
 */
const char* mqtt_device_id = DEVICE_ID;

/**
 * MQTT API Key for authentication
 * Used as MQTT username for broker connection
 */
const char* mqtt_api_key = API_KEY;

/**
 * Factory UID from OPTIGA Trust M
 * Hardware identity of the device
 */
const char* mqtt_factory_uid = FACTORY_UID;
