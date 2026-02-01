/*
 * cryptography_examples.h
 *
 *
 *
 */
#ifndef CRYPTOGRAPHY_CRYPTOGRAPHY_EXAMPLES_H_
#define CRYPTOGRAPHY_CRYPTOGRAPHY_EXAMPLES_H_

#include "cy_pdl.h"
#include "mtb_hal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "examples/optiga/include/optiga_example.h"
#include "include/common/optiga_lib_logger.h"
#include "include/pal/pal.h"


/*******************************************************************************
 * Example Configurations
 *******************************************************************************/
// Default scenario can be overridden at runtime via MQTT scenario command
#define TRUSTM_DEFAULT_SCENARIO 1

// needed for scenario 1 (use tesaiot_config.h values if available)
#ifndef TRUST_ANCHOR_OID
#define TRUST_ANCHOR_OID 0xE0E8
#endif
#ifndef CONFIDENTIALITY_OID
#define CONFIDENTIALITY_OID 0xF1D4
#endif

//needed for scenario 2
#ifndef DEVICE_CERTIFICATE_OID
#define DEVICE_CERTIFICATE_OID 0xE0E1
#endif
#ifndef ROOT_CA_OID
#define ROOT_CA_OID 0xE0E9
#endif

//needed for both scenarios
#ifndef TARGET_KEY_OID
#define TARGET_KEY_OID 0xE0F1
#endif


/*******************************************************************************
 * Macros
 *******************************************************************************/

#define EXAMPLE_MODULE "[optiga example]  : "

#define MESSAGE(msg) \
        optiga_lib_print_message(msg, EXAMPLE_MODULE, \
        OPTIGA_LIB_LOGGER_COLOR_LIGHT_GREEN);

#define ERROR(msg) \
        optiga_lib_print_message(msg, OPTIGA_SHELL_MODULE, \
        OPTIGA_LIB_LOGGER_COLOR_YELLOW);

#define TAB "                    "

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
extern void cryptography_examples(void);

extern void example_optiga_init(void);
extern void example_optiga_deinit(void);
extern void example_optiga_crypt_hash(void);
extern void example_optiga_crypt_ecc_generate_keypair(void);
extern void example_optiga_crypt_ecdsa_sign(void);
extern void example_optiga_crypt_ecdsa_verify(void);
extern void example_optiga_crypt_ecdh(void);
extern void example_optiga_crypt_random(void);
extern void example_optiga_crypt_tls_prf_sha256(void);
extern void example_optiga_crypt_rsa_generate_keypair(void);
extern void example_optiga_crypt_rsa_sign(void);
extern void example_optiga_crypt_rsa_verify(void);
extern void example_optiga_crypt_rsa_decrypt_and_export(void);
extern void example_optiga_crypt_rsa_decrypt_and_store(void);
extern void example_optiga_crypt_rsa_encrypt_message(void);
extern void example_optiga_crypt_rsa_encrypt_session(void);
extern void example_optiga_crypt_hash_data(void);
extern void example_optiga_crypt_symmetric_encrypt_decrypt_ecb(void);
extern void example_optiga_crypt_symmetric_encrypt_decrypt_cbc(void);
extern void example_optiga_crypt_symmetric_encrypt_cbcmac(void);
extern void example_optiga_crypt_hmac(void);
extern void example_optiga_crypt_hkdf(void);
extern void example_optiga_crypt_symmetric_generate_key(void);
extern void example_optiga_hmac_verify_with_authorization_reference(void);
extern void example_optiga_crypt_clear_auto_state(void);

extern void example_optiga_util_protected_update(void);

#endif /* TESA_CRYPTOGRAPHY_CRYPTOGRAPHY_EXAMPLES_H_ */
