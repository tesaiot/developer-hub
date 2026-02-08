/**
 * @file tesaiot_optiga.h
 * @brief TESAIoT OPTIGA Trust M Wrapper Public API
 * @version 2.0
 * @date 2026-01-23
 *
 * Synchronous wrapper for OPTIGA Trust M asynchronous operations
 * Uses pthread-based synchronization for blocking API
 *
 * Architecture: Library-first design (PSoC E84 reference pattern)
 */

#ifndef TESAIOT_OPTIGA_H
#define TESAIOT_OPTIGA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 * Initialization and Deinitialization
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize OPTIGA Trust M
 *
 * Initializes PAL layer, creates OPTIGA crypt/util instances,
 * and opens OPTIGA application
 *
 * @return 0 on success, negative error code on failure
 *         -1: General initialization failure
 *
 * @note This function must be called before any other OPTIGA operations
 * @note Uses I2C address 0x30 (default for OPTIGA Trust M)
 */
int tesaiot_optiga_init(void);

/**
 * @brief Deinitialize OPTIGA Trust M
 *
 * Closes OPTIGA application, destroys instances, deinitializes PAL
 *
 * @return 0 on success
 *
 * @note Safe to call multiple times
 */
int tesaiot_optiga_deinit(void);

/**
 * @brief Check if OPTIGA is initialized
 *
 * @return true if initialized, false otherwise
 */
bool tesaiot_optiga_is_initialized(void);

/**
 * @brief Get last OPTIGA error code
 *
 * @return OPTIGA library status code from last operation
 */
uint16_t tesaiot_optiga_get_last_error(void);

/*----------------------------------------------------------------------------
 * Cryptographic Operations
 *---------------------------------------------------------------------------*/

/**
 * @brief Generate ECC keypair and store in OPTIGA
 *
 * Generates ECC NIST P-256 keypair with SIGN usage.
 * Private key is stored in specified OID (never exported).
 * Public key is returned in DER format.
 *
 * @param oid OID for private key storage (e.g., 0xE0F1 for CSR key)
 * @param pubkey Buffer to receive public key (DER format, min 100 bytes)
 * @param pubkey_len Pointer to buffer size (input: buffer size, output: actual length)
 * @return 0 on success, -1 on failure
 *
 * @note Keypair generation takes ~1-2 seconds
 * @note This is a blocking call (pthread synchronization)
 */
int tesaiot_optiga_generate_keypair(uint16_t oid, uint8_t *pubkey, uint16_t *pubkey_len);

/**
 * @brief Calculate SHA-256 hash using OPTIGA
 *
 * @param data Data to hash
 * @param data_len Length of data
 * @param hash Buffer to receive hash (must be at least 32 bytes)
 * @param hash_len Pointer to hash length (output: always 32 for SHA-256)
 * @return 0 on success, -1 on failure
 *
 * @note This is a blocking call (pthread synchronization)
 */
int tesaiot_optiga_hash(const uint8_t *data, uint16_t data_len, uint8_t *hash, uint16_t *hash_len);

/**
 * @brief Sign data using OPTIGA ECC private key
 *
 * Generates ECDSA signature using private key stored at specified OID
 *
 * @param oid OID containing private key (e.g., 0xE0F1)
 * @param data Data to sign (typically 32-byte hash)
 * @param data_len Length of data
 * @param signature Buffer to receive signature (DER format, min 72 bytes)
 * @param signature_len Pointer to buffer size (input: buffer size, output: actual length)
 * @return 0 on success, -1 on failure
 *
 * @note Signature format is DER-encoded ECDSA-Sig-Value (RFC 3279)
 * @note This is a blocking call (pthread synchronization)
 */
int tesaiot_optiga_sign(uint16_t oid, const uint8_t *data, uint16_t data_len,
                         uint8_t *signature, uint16_t *signature_len);

/**
 * @brief Verify ECDSA signature using OPTIGA
 *
 * Verifies signature using public key stored at specified OID or provided data
 *
 * @param pubkey_source Public key source (OID or data)
 * @param pubkey Public key data (NULL if using OID)
 * @param pubkey_len Length of public key data (0 if using OID)
 * @param digest Digest to verify (typically SHA-256, 32 bytes)
 * @param digest_len Length of digest
 * @param signature Signature to verify (DER format)
 * @param signature_len Length of signature
 * @return 0 on success (signature valid), -1 on failure or invalid signature
 *
 * @note Used by Protected Update workflow for Trust Anchor verification
 */
int tesaiot_optiga_verify(uint16_t pubkey_source, const uint8_t *pubkey, uint16_t pubkey_len,
                           const uint8_t *digest, uint16_t digest_len,
                           const uint8_t *signature, uint16_t signature_len);

/*----------------------------------------------------------------------------
 * Data Operations
 *---------------------------------------------------------------------------*/

/**
 * @brief Write data to OPTIGA data object
 *
 * Writes data to specified OID using ERASE_AND_WRITE mode
 *
 * @param oid Target OID (e.g., 0xE0E1 for device certificate)
 * @param data Data to write
 * @param data_len Length of data (max depends on OID capacity)
 * @return 0 on success, -1 on failure
 *
 * @note Check OID metadata for write permissions (LcsO)
 * @note This is a blocking call (pthread synchronization)
 */
int tesaiot_optiga_write_data(uint16_t oid, const uint8_t *data, uint16_t data_len);

/**
 * @brief Read data from OPTIGA data object
 *
 * @param oid Source OID
 * @param offset Offset to start reading from
 * @param data Buffer to receive data
 * @param data_len Pointer to buffer size (input: buffer size, output: actual bytes read)
 * @return 0 on success, -1 on failure
 *
 * @note This is a blocking call (pthread synchronization)
 */
int tesaiot_optiga_read_data(uint16_t oid, uint16_t offset, uint8_t *data, uint16_t *data_len);

/**
 * @brief Read device UID from OPTIGA
 *
 * Reads unique device identifier from OID 0xE0C2 and converts to hex string
 *
 * @param uid_hex Buffer to receive UID as hex string (min 128 bytes)
 * @param uid_hex_len Length of uid_hex buffer
 * @return 0 on success, -1 on failure
 *
 * @note UID is typically 27 bytes (54 hex characters + null terminator)
 * @note UID is factory-programmed and read-only
 */
int tesaiot_optiga_read_uid(char *uid_hex, size_t uid_hex_len);

/*----------------------------------------------------------------------------
 * Protected Update Operations
 *---------------------------------------------------------------------------*/

/**
 * @brief Start Protected Update operation
 *
 * Initiates Protected Update sequence with manifest verification
 *
 * @param manifest CBOR COSE_Sign1 manifest
 * @param manifest_len Length of manifest
 * @return 0 on success, -1 on failure
 *
 * @note Used by Protected Update workflow
 * @note Verifies manifest signature with Trust Anchor (OID 0xE0E9)
 */
int tesaiot_optiga_protected_update_start(const uint8_t *manifest, uint16_t manifest_len);

/**
 * @brief Continue Protected Update with fragment
 *
 * Processes one fragment in Protected Update sequence
 *
 * @param fragment Fragment data (CBOR encoded)
 * @param fragment_len Length of fragment
 * @return 0 on success, -1 on failure
 *
 * @note Call after tesaiot_optiga_protected_update_start()
 * @note Can be called multiple times for multi-fragment updates
 */
int tesaiot_optiga_protected_update_continue(const uint8_t *fragment, uint16_t fragment_len);

/**
 * @brief Finalize Protected Update operation
 *
 * Completes Protected Update sequence with final fragment
 *
 * @param fragment Final fragment data
 * @param fragment_len Length of final fragment
 * @return 0 on success, -1 on failure
 *
 * @note This is the last call in Protected Update sequence
 * @note Target OID will be updated upon success
 */
int tesaiot_optiga_protected_update_final(const uint8_t *fragment, uint16_t fragment_len);

/*----------------------------------------------------------------------------
 * OPTIGA Constants
 *---------------------------------------------------------------------------*/

/** OPTIGA I2C address */
#define TESAIOT_OPTIGA_I2C_ADDR 0x30

/** Maximum public key size (DER format, ECC P-256) */
#define TESAIOT_OPTIGA_PUBKEY_MAX_LEN 100

/** Maximum signature size (DER format, ECC P-256) */
#define TESAIOT_OPTIGA_SIGNATURE_MAX_LEN 80

/** SHA-256 hash length */
#define TESAIOT_OPTIGA_SHA256_LEN 32

/** Maximum data object size (varies by OID) */
#define TESAIOT_OPTIGA_DATA_MAX_LEN 1728

/** Device UID binary length (typical) */
#define TESAIOT_OPTIGA_UID_BIN_LEN 27

/** Device UID hex string length (54 chars + null) */
#define TESAIOT_OPTIGA_UID_HEX_LEN 55

/*----------------------------------------------------------------------------
 * OPTIGA OID Definitions
 *---------------------------------------------------------------------------*/

/**
 * @defgroup OPTIGA_OIDS OPTIGA Trust M Object IDs (OIDs)
 * @{
 */

/*============================================================================
 * OID Defaults (can be overridden by client's tesaiot_config.h)
 *
 * To customize OID addresses, define them in your tesaiot_config.h
 * BEFORE including any tesaiot headers:
 *
 *   #define TESAIOT_OID_DEVICE_CERT 0xE0E3  // Use different OID
 *   #include <tesaiot.h>
 *
 * @warning OID 0xE0E1 was locked on 2026-01-30. Default is now 0xE0E2.
 *===========================================================================*/

/** Trust M Device UID (Read-Only, Factory) */
#ifndef TESAIOT_OID_TRUSTM_UID
#define TESAIOT_OID_TRUSTM_UID 0xE0C2
#endif

/** Factory Certificate (Read-Only, Factory) - Bootstrap TLS */
#ifndef TESAIOT_OID_FACTORY_CERT
#define TESAIOT_OID_FACTORY_CERT 0xE0E0
#endif

/** Factory Private Key (Sign-Only, Factory) - Bootstrap TLS */
#ifndef TESAIOT_OID_FACTORY_KEY
#define TESAIOT_OID_FACTORY_KEY 0xE0F0
#endif

/** CSR Private Key (Sign-Only, LcsO=0x07 || PU update) - CSR Workflow */
#ifndef TESAIOT_OID_CSR_KEY
#define TESAIOT_OID_CSR_KEY 0xE0F1
#endif

/**
 * Device Certificate (Read/Write, LcsO=0x07 || PU update) - Shared
 * Developer can override by defining TESAIOT_OID_DEVICE_CERT in their config.
 * Valid OIDs: 0xE0E1 (default), 0xE0E2, 0xE0E3
 */
#ifndef TESAIOT_OID_DEVICE_CERT
#define TESAIOT_OID_DEVICE_CERT 0xE0E1  /* Standard default */
#endif

/** TESAIoT CA Chain (Read Always, LcsO=0x07 || PU update) - CSR Workflow */
#ifndef TESAIOT_OID_CA_CHAIN
#define TESAIOT_OID_CA_CHAIN 0xE0E9
#endif

/** Trust Anchor (Read Always, LcsO=0x01, writable) - Protected Update
 *  Changed to 0xE0E9: 0xE0E8 is locked (LcsO=0x07) with Infineon test cert */
#ifndef TESAIOT_OID_TRUST_ANCHOR
#define TESAIOT_OID_TRUST_ANCHOR 0xE0E9
#endif

/** Secret (Read NEVER, LcsO=0x07 || PU update) - Protected Update AES Key */
#ifndef TESAIOT_OID_SECRET
#define TESAIOT_OID_SECRET 0xF1D4
#endif

/** Protected Update Key Slot (LcsO=0x07 || PU update) */
#ifndef TESAIOT_OID_PU_KEY_SLOT
#define TESAIOT_OID_PU_KEY_SLOT 0xE0F2
#endif

/** Protected Update Metadata Slot (LcsO=0x07 || PU update) */
#ifndef TESAIOT_OID_PU_METADATA_SLOT
#define TESAIOT_OID_PU_METADATA_SLOT 0xE0F3
#endif

/** @} */

/*----------------------------------------------------------------------------
 * Error Codes
 *---------------------------------------------------------------------------*/

/**
 * @defgroup OPTIGA_ERRORS OPTIGA Error Codes
 * @{
 */

#define TESAIOT_OPTIGA_SUCCESS 0x0000          /**< Operation successful */
#define TESAIOT_OPTIGA_ERROR_INVALID_PARAM -1  /**< Invalid parameter */
#define TESAIOT_OPTIGA_ERROR_NOT_INIT -2       /**< OPTIGA not initialized */
#define TESAIOT_OPTIGA_ERROR_OPERATION -3      /**< Operation failed */
#define TESAIOT_OPTIGA_ERROR_TIMEOUT -4        /**< Operation timeout */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_OPTIGA_H */
