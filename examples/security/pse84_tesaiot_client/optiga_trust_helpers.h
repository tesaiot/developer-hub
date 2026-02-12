/******************************************************************************
* File Name:   optiga_trust_helpers.h
*
* Description: This file contains helping fucntions to read a certificate or 
*              change some default parameter
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/******************************************************************************
* TESAIoT Platform Extensions
*******************************************************************************
* This file has been extended through collaboration between:
* - Infineon Technologies AG (OPTIGA Trust M expertise)
* - TESAIoT Platform Developer Team (AIoT platform integration)
*
* Extensions added:
* - Certificate validation and expiry checking functions
* - CSR generation with OPTIGA Trust M integration
* - Protected Update data set helpers
* - TESAIoT Platform-specific certificate lifecycle management
*
* Original work Copyright 2020-2025 Cypress Semiconductor Corporation (Infineon)
* Extensions developed jointly by Infineon Technologies AG and TESAIoT Team
*
* This file is part of the TESAIoT AIoT Foundation Platform, developed in
* collaboration with Infineon Technologies AG for PSoC Edge E84 + OPTIGA Trust M.
*
* Contact: Wiroon Sriborrirux <sriborrirux@gmail.com>
*******************************************************************************/

#ifndef OPTIGA_TRUST_HELPERS_H_
#define OPTIGA_TRUST_HELPERS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "include/common/optiga_lib_common.h"
#include "include/optiga_crypt.h"
#include "include/optiga_util.h"
#include "include/pal/pal_os_timer.h"

/**
 * \brief Specifies the structure for protected update manifest and fragment configuration
 */
typedef struct optiga_protected_update_manifest_fragment_configuration {
    /// Manifest version.
    uint8_t manifest_version;
    /// Pointer to a buffer where manifest data is stored.
    const uint8_t *manifest_data;
    /// Manifest length
    uint16_t manifest_length;
    /// Pointer to a buffer where continue fragment data is stored.
    const uint8_t *continue_fragment_data;
    /// Continue fragment length
    uint16_t continue_fragment_length;
    /// Pointer to a buffer where final fragment data is stored.
    const uint8_t *final_fragment_data;
    /// Final fragment length
    uint16_t final_fragment_length;
} optiga_protected_update_manifest_fragment_configuration_t;

/**
 * \brief Specifies the structure for protected update data configuration
 */
typedef struct optiga_protected_update_data_configuration {
    /// Target OID
    uint16_t target_oid;
    /// Target OID metadata
    const uint8_t *target_oid_metadata;
    /// Target OID metadata length
    uint16_t target_oid_metadata_length;
    /// Pointer to a buffer where continue fragment data is stored.
    const optiga_protected_update_manifest_fragment_configuration_t *data_config;
    /// Pointer to a protected update example string.
    const char *set_prot_example_string;
} optiga_protected_update_data_configuration_t;



void read_certificate_from_optiga(uint16_t optiga_oid, char * cert_pem, uint16_t * cert_pem_length);

void read_trust_anchor_from_optiga(uint16_t oid, char * cert_pem, uint16_t * cert_pem_length);

void write_data_object (uint16_t oid, const uint8_t * p_data, uint16_t length);

void optiga_trust_init(void);

void optiga_util_callback(void *context, optiga_lib_status_t return_status);

/* Global async status - can be polled by other modules for operation completion */
extern volatile optiga_lib_status_t optiga_lib_status;

void trustm_close(optiga_util_t *util);

optiga_lib_status_t trustm_gen_ecc_keypair(uint16_t optiga_key_id, optiga_ecc_curve_t curve_id,uint8_t key_usage, bool export_private, uint8_t * pub, uint16_t *pub_len);

optiga_lib_status_t trustm_ecdsa_sign(optiga_key_id_t oid, const uint8_t *digest, uint16_t digest_len, uint8_t *sig_raw, uint16_t *sig_raw_length);

bool optiga_read_factory_uid(char *uid_hex, size_t uid_hex_len);

bool optiga_read_factory_certificate(char *cert_pem, uint16_t *cert_pem_length);

bool optiga_read_device_certificate(char *cert_pem, uint16_t *cert_pem_length);

bool trustm_use_factory_certificate(void);

bool trustm_use_device_certificate(void);

bool optiga_generate_device_keypair(uint16_t key_oid, uint8_t *public_key_der, uint16_t *public_key_der_len);

/*******************************************************************************
 * TESAIoT Platform Extensions - Types and Functions
 *******************************************************************************
 * REFACTORING NOTE (2026-01-18):
 * The following types and functions (lines 139-217) are TESAIoT extensions
 * added to this Infineon file. They are candidates for extraction to a
 * separate file: tesaiot_cert_utils.h (planned for v0.9)
 *
 * TESAIoT-specific additions:
 * - cert_validation_result_t struct
 * - cert_source_t enum
 * - optiga_check_certificate_validity()
 * - optiga_get_cert_days_until_expiry()
 * - optiga_generate_csr_pem()
 *
 * See: IMPROVEMENTS/2026.01/01_header_organization_analysis.md
 ******************************************************************************/

/**
 * @brief Certificate validation result structure
 *
 * Contains comprehensive validation information about a certificate
 * including validity status, expiry date, and subject information.
 */
typedef struct {
    bool is_valid;              ///< true if certificate is currently valid
    bool is_expired;            ///< true if certificate has expired
    bool cert_exists;           ///< true if certificate found in OID
    uint32_t days_until_expiry; ///< Days remaining until expiry (0 if expired)
    time_t valid_from;          ///< Certificate valid from timestamp
    time_t valid_to;            ///< Certificate valid to timestamp
    char subject[256];          ///< Certificate subject DN
    char issuer[256];           ///< Certificate issuer DN
} cert_validation_result_t;

/**
 * @brief Certificate source type for MQTT TLS connection
 */
typedef enum {
    CERT_SOURCE_DEVICE = 0xE0E1,    ///< TESAIoT Platform Certificate (preferred)
    CERT_SOURCE_FACTORY = 0xE0E0    ///< Infineon Factory Certificate (fallback)
} cert_source_t;

/**
 * @brief Check certificate validity and extract information
 *
 * Reads certificate from OPTIGA Trust M and validates it against current time.
 * Extracts subject, issuer, validity dates and calculates days until expiry.
 *
 * @param oid Certificate OID (0xE0E0 for Factory, 0xE0E1 for Device)
 * @param result Pointer to validation result structure (output)
 * @return true if certificate readable and parseable, false on error
 *
 * @note Requires RTC or NTP time synchronization for accurate validation
 * @note Uses mbedtls_x509_crt_parse() for X.509 certificate parsing
 */
bool optiga_check_certificate_validity(uint16_t oid, cert_validation_result_t *result);

/**
 * @brief Get days until certificate expiry
 *
 * Convenience function to quickly check certificate expiry status.
 *
 * @param oid Certificate OID (0xE0E0 or 0xE0E1)
 * @return Days until expiry, 0 if expired or error
 */
uint32_t optiga_get_cert_days_until_expiry(uint16_t oid);

bool optiga_generate_csr_pem(uint16_t key_oid, const uint8_t *public_key_der, uint16_t public_key_der_len, const char *subject, char *csr_pem, size_t csr_pem_len);

optiga_lib_status_t write_device_certificate_and_verify();
optiga_lib_status_t protected_update(uint16_t trust_anchor_oid, uint16_t target_key_oid, uint16_t confidentiality_oid);

/**
 * Test metadata operations for debugging certificate write issues
 *
 * Performs 5 comprehensive diagnostic tests:
 * - TEST 1: Read metadata from OID 0xE0E1 (device certificate slot)
 * - TEST 2: Test write metadata to OID 0xF1D0 (user data OID)
 * - TEST 3: Display OID 0xE0E1 specification (access conditions from datasheet)
 * - TEST 4: Read Global Lifecycle State (OID 0xE0C0) to check if device is locked
 * - TEST 5: Read and display current device certificate from OID 0xE0E1
 *
 * Certificate Display includes:
 * - Subject CN (Common Name)
 * - Issuer CN
 * - Valid From/To dates
 * - Serial Number
 * - Certificate size
 * - First 64 bytes of DER data
 *
 * Usage: Run before CSR workflow to see OLD certificate, then after to see NEW certificate
 *
 * @return OPTIGA_LIB_SUCCESS on success, error code otherwise
 */
optiga_lib_status_t test_metadata_operations(void);

#endif /* OPTIGA_TRUST_HELPERS_H_ */
