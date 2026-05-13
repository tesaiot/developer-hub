/**
 * @file tesaiot_crypto.h
 * @brief TESAIoT v3.0.0 Crypto Utility Functions (Phase 1 + Phase 2)
 * @version 3.0.0
 * @date 2026-02-08
 *
 * Developer-friendly wrappers over OPTIGA Trust M crypto operations.
 * All keys stay inside OPTIGA hardware (CC EAL6+ certified).
 *
 * Phase 1 (CRITICAL):
 *   - tesaiot_random_generate()       — Hardware TRNG
 *   - tesaiot_secure_store_write/read() — Secure credential storage
 *   - tesaiot_aes_generate_key()      — Generate AES key in OPTIGA
 *   - tesaiot_aes_encrypt/decrypt()   — AES-CBC with hardware key
 *   - tesaiot_hmac_sha256()           — HMAC-SHA256 with hardware key
 *
 * Phase 2 (IMPORTANT):
 *   - tesaiot_ecdh_shared_secret()    — ECDH key agreement
 *   - tesaiot_hkdf_derive()           — HKDF-SHA256 key derivation
 *   - tesaiot_sign_data()             — Hash + ECDSA sign (convenience)
 *   - tesaiot_verify_data()           — Hash + ECDSA verify (convenience)
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 */

#ifndef TESAIOT_CRYPTO_H
#define TESAIOT_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Error Codes (extend existing TESAIOT_OPTIGA_ERROR_* range)
 *===========================================================================*/

#define TESAIOT_CRYPTO_ERROR_SLOT_RESERVED  (-5)  /**< Slot 4 (0xF1D4) is reserved */
#define TESAIOT_CRYPTO_ERROR_SIZE_EXCEEDED  (-6)  /**< Data exceeds slot capacity */
#define TESAIOT_CRYPTO_ERROR_KEY_NOT_FOUND  (-7)  /**< AES key not yet generated */

/*============================================================================
 * Constants
 *===========================================================================*/

/** AES symmetric key OID (session/symmetric key context) */
#define TESAIOT_AES_KEY_OID     0xE200

/** Maximum slot number for secure store */
#define TESAIOT_STORE_MAX_SLOT  9

/** Reserved slot (Protected Update shared secret) */
#define TESAIOT_STORE_RESERVED_SLOT  4

/*============================================================================
 * Phase 1: CRITICAL Functions
 *===========================================================================*/

/**
 * @brief Generate cryptographically secure random bytes using OPTIGA TRNG
 *
 * @param[out] buffer  Output buffer for random data (must not be NULL)
 * @param[in]  length  Number of random bytes to generate (8-256)
 * @return 0 on success, negative on error
 *
 * @note Uses OPTIGA Trust M hardware TRNG (CC EAL6+ certified)
 * @note Thread-safe: acquires OPTIGA mutex internally
 */
int tesaiot_random_generate(uint8_t *buffer, uint16_t length);

/**
 * @brief Store data in OPTIGA hardware-protected data object
 *
 * @param[in] slot    Storage slot:
 *                      0-3  -> OID 0xF1D0-0xF1D3 (max 140 bytes)
 *                      5-7  -> OID 0xF1D5-0xF1D7 (max 140 bytes)
 *                      8-9  -> OID 0xF1E0-0xF1E1 (max 1500 bytes)
 *                      Slot 4 (0xF1D4) BLOCKED — Reserved for Protected Update
 * @param[in] data    Data to store (must not be NULL)
 * @param[in] length  Data length
 * @return 0 on success, negative on error
 *         -5: TESAIOT_CRYPTO_ERROR_SLOT_RESERVED (slot 4)
 *         -6: TESAIOT_CRYPTO_ERROR_SIZE_EXCEEDED
 */
int tesaiot_secure_store_write(uint8_t slot, const uint8_t *data, uint16_t length);

/**
 * @brief Read data from OPTIGA hardware-protected data object
 *
 * @param[in]     slot    Storage slot (same mapping as write)
 * @param[out]    data    Output buffer (must not be NULL)
 * @param[in,out] length  In: buffer size, Out: actual data length
 * @return 0 on success, negative on error
 */
int tesaiot_secure_store_read(uint8_t slot, uint8_t *data, uint16_t *length);

/**
 * @brief Generate AES symmetric key inside OPTIGA (stored at OID 0xE200)
 *
 * @param[in] key_bits  Key size: 128, 192, or 256
 * @return 0 on success, negative on error
 *
 * @note Key is generated and stored in OPTIGA — never leaves hardware
 * @note Call once during setup; key persists across power cycles
 */
int tesaiot_aes_generate_key(uint16_t key_bits);

/**
 * @brief AES-CBC encrypt using hardware key at OID 0xE200
 *
 * @param[in]     plaintext   Input data
 * @param[in]     plain_len   Input length
 * @param[in]     iv          16-byte IV (NULL = auto-generate via TRNG)
 * @param[out]    iv_out      Output: actual IV used (16 bytes, may be same ptr as iv)
 * @param[out]    ciphertext  Output buffer
 * @param[in,out] cipher_len  In: buffer size, Out: actual ciphertext length
 * @return 0 on success, negative on error
 */
int tesaiot_aes_encrypt(const uint8_t *plaintext, uint16_t plain_len,
                        const uint8_t *iv, uint8_t *iv_out,
                        uint8_t *ciphertext, uint16_t *cipher_len);

/**
 * @brief AES-CBC decrypt using hardware key at OID 0xE200
 *
 * @param[in]     ciphertext  Encrypted data
 * @param[in]     cipher_len  Ciphertext length
 * @param[in]     iv          16-byte IV used during encryption
 * @param[out]    plaintext   Output buffer
 * @param[in,out] plain_len   In: buffer size, Out: actual plaintext length
 * @return 0 on success, negative on error
 */
int tesaiot_aes_decrypt(const uint8_t *ciphertext, uint16_t cipher_len,
                        const uint8_t *iv,
                        uint8_t *plaintext, uint16_t *plain_len);

/**
 * @brief Compute HMAC-SHA256 using secret key stored in OPTIGA
 *
 * @param[in]     secret_slot  Slot containing HMAC key (0-7, excluding 4)
 * @param[in]     data         Input data
 * @param[in]     data_len     Data length
 * @param[out]    mac          Output: 32-byte HMAC-SHA256
 * @param[in,out] mac_len      In: buffer size (>= 32), Out: actual MAC length
 * @return 0 on success, negative on error
 *
 * @note Secret key must be pre-provisioned in the slot
 */
int tesaiot_hmac_sha256(uint8_t secret_slot,
                        const uint8_t *data, uint16_t data_len,
                        uint8_t *mac, uint16_t *mac_len);

/*============================================================================
 * Phase 2: IMPORTANT Functions
 *===========================================================================*/

/**
 * @brief Derive ECDH shared secret using OPTIGA hardware key
 *
 * @param[in]     key_oid         Private key OID (use 0xE0F2 or 0xE0F3)
 *                                WARNING: 0xE0F0 and 0xE0F1 are rejected
 * @param[in]     peer_pubkey     Peer's ECC P-256 public key (DER format)
 * @param[in]     peer_pubkey_len Public key length
 * @param[out]    shared_secret   Output: 32-byte shared secret
 * @param[in,out] secret_len      In: buffer size (>= 32), Out: actual length
 * @return 0 on success, negative on error
 */
int tesaiot_ecdh_shared_secret(uint16_t key_oid,
                               const uint8_t *peer_pubkey, uint16_t peer_pubkey_len,
                               uint8_t *shared_secret, uint16_t *secret_len);

/**
 * @brief Derive key material using HKDF-SHA256 (RFC 5869)
 *
 * @param[in]  secret_oid   OID containing input keying material
 * @param[in]  salt         Optional salt (NULL = zero salt per RFC 5869)
 * @param[in]  salt_len     Salt length (0 if NULL)
 * @param[in]  info         Context/application-specific info string
 * @param[in]  info_len     Info length
 * @param[out] derived_key  Output: derived key material
 * @param[in]  key_len      Desired output key length (1-256)
 * @return 0 on success, negative on error
 */
int tesaiot_hkdf_derive(uint16_t secret_oid,
                        const uint8_t *salt, uint16_t salt_len,
                        const uint8_t *info, uint16_t info_len,
                        uint8_t *derived_key, uint16_t key_len);

/**
 * @brief Sign arbitrary data (SHA-256 hash + ECDSA sign in one call)
 *
 * @param[in]     key_oid    Signing key OID (0xE0F1 for device key)
 * @param[in]     data       Data to sign
 * @param[in]     data_len   Data length
 * @param[out]    signature  Output: DER-encoded ECDSA signature (max ~72 bytes)
 * @param[in,out] sig_len    In: buffer size (>= 80), Out: actual signature length
 * @return 0 on success, negative on error
 *
 * @note Convenience wrapper: internally calls tesaiot_optiga_hash() + tesaiot_optiga_sign()
 */
int tesaiot_sign_data(uint16_t key_oid,
                      const uint8_t *data, uint16_t data_len,
                      uint8_t *signature, uint16_t *sig_len);

/**
 * @brief Verify data signature (SHA-256 hash + ECDSA verify in one call)
 *
 * @param[in] pubkey_source  Public key source type for OPTIGA
 * @param[in] pubkey         Public key (DER format, NULL if using OID)
 * @param[in] pubkey_len     Public key length (0 if using OID)
 * @param[in] data           Original data
 * @param[in] data_len       Data length
 * @param[in] signature      DER-encoded ECDSA signature to verify
 * @param[in] sig_len        Signature length
 * @return 0 on success (signature valid), negative on error
 */
int tesaiot_verify_data(uint16_t pubkey_source,
                        const uint8_t *pubkey, uint16_t pubkey_len,
                        const uint8_t *data, uint16_t data_len,
                        const uint8_t *signature, uint16_t sig_len);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_CRYPTO_H */
