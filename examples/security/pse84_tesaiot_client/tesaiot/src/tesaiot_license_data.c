/**
 * @file tesaiot_license_data.c
 * @brief TESAIoT License Data Binding (Customer Compiles This File)
 * @copyright (c) 2025 TESAIoT AIoT Foundation Platform
 *
 * IMPORTANT: This file binds your license credentials to the TESAIoT Library.
 *
 * WORKFLOW:
 * 1. Edit tesaiot_license_config.h - set your UID and License Key
 * 2. Build your project - this file is automatically compiled
 * 3. Library verification logic (IP-protected) uses these values
 *
 * DO NOT MODIFY THIS FILE - Edit tesaiot_license_config.h instead.
 */

#include "tesaiot_license_config.h"

/**
 * Device UID from OPTIGA Trust M (27 bytes as 54 hex characters)
 *
 * This value is provided to TESAIoT Library's license verification.
 * Set via TESAIOT_DEVICE_UID in tesaiot_license_config.h
 */
const char* tesaiot_device_uid = TESAIOT_DEVICE_UID;

/**
 * License Key from TESAIoT Platform (Base64-encoded ECDSA signature)
 *
 * This signature proves your device is authorized to use TESAIoT Library.
 * Set via TESAIOT_LICENSE_KEY in tesaiot_license_config.h
 */
const char* tesaiot_license_key = TESAIOT_LICENSE_KEY;
