/**
 * @file tesaiot_crypto.h
 * @brief TESAIoT Developer Crypto Utilities - Public API (Group 7)
 * @version 3.0.0
 * @date 2026-02-08
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Developer-facing cryptographic utility functions wrapping OPTIGA Trust M
 * hardware capabilities. All functions are license-gated and thread-safe.
 *
 * Features:
 * - Hardware TRNG (True Random Number Generator)
 * - Secure credential storage (slot-based OID mapping)
 * - AES-CBC symmetric encryption (key stays in OPTIGA hardware)
 * - HMAC-SHA256 message authentication
 * - ECDH key agreement (P-256)
 * - HKDF-SHA256 key derivation (RFC 5869)
 * - SHA-256 hardware hash
 * - ECDSA data signing (hash + sign composite)
 * - Monotonic counters (anti-replay)
 * - Health check (device diagnostics)
 *
 * Usage:
 * @code
 * #include "tesaiot.h"  // Automatically includes tesaiot_crypto.h
 *
 * // Generate random bytes
 * uint8_t random_buf[32];
 * int rc = tesaiot_random_generate(random_buf, 32);
 *
 * // Store credential in OPTIGA
 * rc = tesaiot_secure_store_write(0, my_data, data_len);
 *
 * // Encrypt with AES
 * rc = tesaiot_aes_generate_key(256);
 * rc = tesaiot_aes_encrypt(plaintext, 16, NULL, iv_out, ciphertext, &ct_len);
 * @endcode
 *
 * @note All functions require valid license (tesaiot_license_init() must succeed first)
 * @note All functions are thread-safe via optiga_manager mutex
 * @note Keys never leave OPTIGA hardware - only results are exported
 */

#ifndef TESAIOT_CRYPTO_H
#define TESAIOT_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Phase 1: Critical Functions
 *============================================================================*/

/**
 * @brief Generate cryptographically secure random bytes from OPTIGA TRNG (CC EAL6+)
 *
 * Uses OPTIGA Trust M hardware True Random Number Generator.
 * Output is suitable for cryptographic keys, nonces, and IVs.
 *
 * @param[out] buffer   Output buffer for random data
 * @param[in]  length   Number of bytes to generate (8-256, OPTIGA hardware limit)
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if buffer is NULL or length out of range
 * @return TESAIOT_ERROR_NOT_INITIALIZED if OPTIGA not initialized
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_random_generate(uint8_t *buffer, uint16_t length);

/**
 * @brief Store data in OPTIGA hardware-protected data object
 *
 * Writes data to a safe developer-accessible OID via slot mapping.
 * Slot 4 (OID 0xF1D4) is RESERVED for Protected Update shared secret.
 *
 * Slot mapping:
 *   0-3   -> OID 0xF1D0-0xF1D3  (max 140 bytes each)
 *   4     -> RESERVED (returns TESAIOT_ERROR_RESERVED_OID)
 *   5-11  -> OID 0xF1D5-0xF1DB  (max 140 bytes each)
 *   12    -> OID 0xF1E0          (max 1500 bytes)
 *   13    -> OID 0xF1E1          (max 1500 bytes)
 *
 * @param[in] slot    Storage slot (0-13, except 4)
 * @param[in] data    Data to store
 * @param[in] length  Data length (slot 0-11: max 140 bytes, slot 12-13: max 1500 bytes)
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if slot > 13 or length exceeds slot capacity
 * @return TESAIOT_ERROR_RESERVED_OID if slot == 4
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_secure_store_write(uint8_t slot, const uint8_t *data, uint16_t length);

/**
 * @brief Read data from OPTIGA hardware-protected data object
 *
 * Reads data from a developer-accessible OID via slot mapping.
 * Same slot mapping as tesaiot_secure_store_write().
 *
 * @param[in]     slot    Storage slot (0-13, except 4)
 * @param[out]    data    Output buffer for read data
 * @param[in,out] length  Input: buffer size, Output: actual data length
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if slot > 13 or data/length is NULL
 * @return TESAIOT_ERROR_RESERVED_OID if slot == 4
 * @return TESAIOT_ERROR_BUFFER_TOO_SMALL if buffer too small
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_secure_store_read(uint8_t slot, uint8_t *data, uint16_t *length);

/**
 * @brief Generate AES key inside OPTIGA (stored at OID 0xE200, never leaves hardware)
 *
 * The generated key is stored in OPTIGA session key OID 0xE200.
 * Key material never leaves the secure element.
 *
 * @param[in] key_bits  Key size: 128, 192, or 256
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if key_bits is not 128, 192, or 256
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_aes_generate_key(uint16_t key_bits);

/**
 * @brief AES-CBC encrypt using hardware key in OPTIGA (OID 0xE200)
 *
 * Encrypts plaintext using AES-CBC mode with key stored in OPTIGA.
 * Call tesaiot_aes_generate_key() first to create the key.
 *
 * @param[in]     plaintext   Input data (MUST be multiple of 16 bytes - caller pads)
 * @param[in]     plain_len   Input length (must be multiple of 16)
 * @param[in]     iv          16-byte IV (NULL = auto-generate from TRNG)
 * @param[out]    iv_out      Output: 16-byte IV used (required if iv=NULL, can be same as iv)
 * @param[out]    ciphertext  Output buffer (must be >= plain_len)
 * @param[in,out] cipher_len  Input: buffer size, Output: actual ciphertext length
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid or plain_len not multiple of 16
 * @return TESAIOT_ERROR_BUFFER_TOO_SMALL if cipher buffer too small
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_aes_encrypt(const uint8_t *plaintext, uint16_t plain_len,
                        const uint8_t *iv, uint8_t *iv_out,
                        uint8_t *ciphertext, uint16_t *cipher_len);

/**
 * @brief AES-CBC decrypt using hardware key in OPTIGA (OID 0xE200)
 *
 * Decrypts ciphertext using AES-CBC mode with key stored in OPTIGA.
 * Must use same key that was used for encryption.
 *
 * @param[in]     ciphertext  Encrypted data (multiple of 16 bytes)
 * @param[in]     cipher_len  Ciphertext length (must be multiple of 16)
 * @param[in]     iv          16-byte IV used during encryption
 * @param[out]    plaintext   Output buffer (must be >= cipher_len)
 * @param[in,out] plain_len   Input: buffer size, Output: actual plaintext length
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_aes_decrypt(const uint8_t *ciphertext, uint16_t cipher_len,
                        const uint8_t *iv,
                        uint8_t *plaintext, uint16_t *plain_len);

/**
 * @brief Compute HMAC-SHA256 using key stored in OPTIGA
 *
 * HMAC key must be pre-stored in OPTIGA data object via
 * tesaiot_secure_store_write(). Key never leaves hardware.
 *
 * @param[in]     secret_slot  Slot number where HMAC key is stored (0-13, except 4)
 * @param[in]     data         Input data to authenticate
 * @param[in]     data_len     Data length
 * @param[out]    mac          Output: 32-byte HMAC-SHA256 value
 * @param[in,out] mac_len      Input: buffer size (>= 32), Output: actual length (32)
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid or secret_slot invalid
 * @return TESAIOT_ERROR_RESERVED_OID if secret_slot == 4
 * @return TESAIOT_ERROR_BUFFER_TOO_SMALL if mac buffer < 32 bytes
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_hmac_sha256(uint8_t secret_slot,
                        const uint8_t *data, uint16_t data_len,
                        uint8_t *mac, uint16_t *mac_len);

/*============================================================================
 * Phase 2: Important Functions
 *============================================================================*/

/**
 * @brief ECDH shared secret derivation using OPTIGA private key
 *
 * Derives a shared secret using ECDH with OPTIGA-stored private key
 * and a peer's public key. Private key never leaves hardware.
 *
 * Recommended OIDs: 0xE0F2 (Application Key) or 0xE0F3 (Spare Key)
 *
 * @warning Using 0xE0F0 or 0xE0F1 will work but is NOT recommended
 *          as they are reserved for mTLS operations.
 *
 * @param[in]     key_oid         Private key OID (0xE0F0-0xE0F3)
 * @param[in]     peer_pubkey     Peer's public key (uncompressed P-256: 65 bytes)
 * @param[in]     peer_pubkey_len Public key length
 * @param[out]    shared_secret   Output: 32-byte shared secret (P-256)
 * @param[in,out] secret_len      Input: buffer size (>= 32), Output: actual length
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid or key_oid not valid
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_ecdh_shared_secret(uint16_t key_oid,
                               const uint8_t *peer_pubkey, uint16_t peer_pubkey_len,
                               uint8_t *shared_secret, uint16_t *secret_len);

/**
 * @brief HKDF-SHA256 key derivation (RFC 5869)
 *
 * Derives key material from input keying material stored in OPTIGA
 * using HKDF-SHA256 (extract-then-expand).
 *
 * @param[in]  secret_oid   OID containing input keying material in OPTIGA
 * @param[in]  salt         Optional salt (NULL = zero-length salt per RFC 5869)
 * @param[in]  salt_len     Salt length (0 if salt is NULL)
 * @param[in]  info         Application-specific context info
 * @param[in]  info_len     Info length
 * @param[out] derived_key  Output: derived key material
 * @param[in]  key_len      Desired output key length in bytes
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_hkdf_derive(uint16_t secret_oid,
                        const uint8_t *salt, uint16_t salt_len,
                        const uint8_t *info, uint16_t info_len,
                        uint8_t *derived_key, uint16_t key_len);

/**
 * @brief Compute SHA-256 hash using OPTIGA hardware
 *
 * Computes SHA-256 digest of arbitrary-length data using OPTIGA
 * Trust M hardware accelerator.
 *
 * @param[in]     data      Input data
 * @param[in]     data_len  Data length
 * @param[out]    hash      Output: 32-byte SHA-256 hash
 * @param[in,out] hash_len  Input: buffer size (>= 32), Output: actual length (32)
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid
 * @return TESAIOT_ERROR_BUFFER_TOO_SMALL if hash buffer < 32 bytes
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_optiga_hash(const uint8_t *data, uint16_t data_len,
                        uint8_t *hash, uint16_t *hash_len);

/**
 * @brief SHA-256 hash + ECDSA sign in one call (convenience wrapper)
 *
 * Composite function: hashes data with SHA-256, then signs the digest
 * with ECDSA using the specified OPTIGA private key.
 *
 * @param[in]     key_oid     Signing key OID (0xE0F0-0xE0F3)
 * @param[in]     data        Data to sign (arbitrary length)
 * @param[in]     data_len    Data length
 * @param[out]    signature   Output: DER-encoded ECDSA signature
 * @param[in,out] sig_len     Input: buffer size (>= 80), Output: actual signature length
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if params invalid or key_oid not valid
 * @return TESAIOT_ERROR_BUFFER_TOO_SMALL if signature buffer too small
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_sign_data(uint16_t key_oid,
                      const uint8_t *data, uint16_t data_len,
                      uint8_t *signature, uint16_t *sig_len);

/*============================================================================
 * Phase 3: Nice to Have Functions
 *============================================================================*/

/**
 * @brief Read monotonic counter value
 *
 * Reads the current value of an OPTIGA monotonic counter.
 * Counter OID mapping: counter_id 0-3 -> OID 0xE120-0xE123
 *
 * @param[in]  counter_id  Counter ID (0-3)
 * @param[out] value       Output: current counter value
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if counter_id > 3 or value is NULL
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_counter_read(uint8_t counter_id, uint32_t *value);

/**
 * @brief Increment monotonic counter
 *
 * Increments an OPTIGA monotonic counter by the specified step.
 * Counter can only go up (monotonic) - cannot be decremented or reset.
 *
 * @warning NVM write limit: ~600,000 increments per counter lifetime.
 *          Use sparingly in production applications.
 *
 * @param[in] counter_id  Counter ID (0-3, maps to OID 0xE120-0xE123)
 * @param[in] step        Increment step (1 or more)
 * @return TESAIOT_OK on success
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if counter_id > 3 or step == 0
 * @return TESAIOT_ERROR_OPTIGA on hardware error
 */
int tesaiot_counter_increment(uint8_t counter_id, uint32_t step);

/**
 * @brief Health check report structure
 */
typedef struct {
    bool optiga_ok;          /**< OPTIGA hardware reachable */
    bool factory_cert_ok;    /**< Factory cert (0xE0E0) valid and not expired */
    bool device_cert_ok;     /**< Device cert (0xE0E1) valid and not expired */
    bool license_ok;         /**< License verified */
    bool mqtt_ok;            /**< MQTT connected */
    bool time_synced;        /**< NTP time synced */
    uint32_t cert_days_left; /**< Days until device cert expiry (0 if expired/unknown) */
    uint8_t  lcso_value;     /**< Lifecycle State Object value */
} tesaiot_health_report_t;

/**
 * @brief Run comprehensive device health check
 *
 * Performs health checks on all major subsystems and populates
 * the report structure. This is a read-only diagnostic function.
 *
 * @param[out] report  Output: health check results
 * @return TESAIOT_OK on success (even if some checks fail - check individual fields)
 * @return TESAIOT_ERROR_NOT_LICENSED if license invalid
 * @return TESAIOT_ERROR_INVALID_PARAM if report is NULL
 */
int tesaiot_health_check(tesaiot_health_report_t *report);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_CRYPTO_H */
