#include "psa/crypto.h"
#include "psa/crypto_se_driver.h"
#include "mbedtls/private_access.h"
#include "mbedtls/psa_util.h"
#include "include/optiga_crypt.h"
#include "include/common/optiga_lib_common.h"
#include "optiga_trust_helpers.h"

/* Default key OID for signing - can be changed dynamically via optiga_psa_set_signing_key_oid() */
#define OPTIGA_KEY_OID_SIGNING_DEFAULT		OPTIGA_KEY_ID_E0F0

/* Current active key OID for TLS signing (dynamic selection for certificate fallback) */
static optiga_key_id_t g_optiga_signing_key_oid = OPTIGA_KEY_ID_E0F1;

#define OPTIGA_MAX_ACTIVE_KEYS 		4

#define PSA_KEY_LOCATION_OPTIGA		((psa_key_location_t) 1)

// No key generation needed only attach the OID
#define OPTIGA_TLS_ATTACH_ONLY		1

typedef struct {
	int used;
	psa_key_slot_number_t slot;
	optiga_key_id_t oid;
	uint8_t pub[65];
	uint16_t pub_len;

} optiga_slot_map_t;

static optiga_slot_map_t g_optiga_map[OPTIGA_MAX_ACTIVE_KEYS];

/* Forward declaration for map_find - needed by optiga_psa_set_signing_key_oid */
static optiga_slot_map_t* map_find(psa_key_slot_number_t slot);

/**
 * @brief Set the OPTIGA key OID to use for TLS signing
 * @param oid Key OID (0xE0F0 for factory cert, 0xE0F1 for TESAIoT cert)
 *
 * This function updates BOTH:
 * - The global variable for new key generation
 * - Any existing slot 0 mapping (for runtime certificate switching)
 *
 * Must match the certificate being used:
 * - Certificate 0xE0E0 (factory) -> Key 0xE0F0
 * - Certificate 0xE0E1 (TESAIoT) -> Key 0xE0F1
 */
void optiga_psa_set_signing_key_oid(optiga_key_id_t oid)
{
    printf("[PSA-SE] Setting signing key OID to 0x%04X\n", (unsigned int)oid);
    g_optiga_signing_key_oid = oid;

    /* Also update slot 0 if it already exists (runtime certificate switch) */
    optiga_slot_map_t *slot0 = map_find(0);
    if (slot0 != NULL && slot0->oid != oid)
    {
        printf("[PSA-SE] Remapping slot 0: 0x%04X -> 0x%04X\n",
               (unsigned int)slot0->oid, (unsigned int)oid);
        slot0->oid = oid;
    }
}

/**
 * @brief Get the currently configured OPTIGA signing key OID
 * @return Current key OID
 */
optiga_key_id_t optiga_psa_get_signing_key_oid(void)
{
    return g_optiga_signing_key_oid;
}

/**
 * @brief Clear all PSA key slots (reset g_optiga_map)
 *
 * This function preserves slot 0 as-is to maintain mbedTLS PSA key handle validity.
 * MQTT test and CSR workflow both use Factory Certificate (0xE0F0), so no reset needed.
 * Only clears other slots (1-3) if they were used.
 *
 * Note: DO NOT reset slot 0 OID - mbedTLS PSA key handle is cached and expects
 * the slot to remain valid with same OID. Resetting causes TLS handshake failure.
 */
void optiga_psa_clear_all_slots(void)
{
    printf("[PSA-SE] Preserving slot 0 (OID 0x%04X) for TLS reuse\n",
           g_optiga_map[0].used ? (unsigned int)g_optiga_map[0].oid : 0);

    /* Clear other slots (1-3) only */
    for (int i = 1; i < OPTIGA_MAX_ACTIVE_KEYS; i++)
    {
        if (g_optiga_map[i].used)
        {
            printf("[PSA-SE] Clearing slot %d\n", i);
            g_optiga_map[i].used = 0;
            g_optiga_map[i].pub_len = 0;
        }
    }

    /* Keep g_optiga_signing_key_oid unchanged - next connection will set it */
}

static psa_status_t optiga_psa_init (psa_drv_se_context_t *ctx, void *persistent_data, psa_key_location_t location);
static psa_status_t optiga_p_allocate(psa_drv_se_context_t *ctx, void *persistent_data, const psa_key_attributes_t *attr, psa_key_creation_method_t method, psa_key_slot_number_t *key_slot);
static psa_status_t optiga_p_destroy(psa_drv_se_context_t *ctx, void *persistent_data, psa_key_slot_number_t key_slot);
static optiga_slot_map_t* map_find(psa_key_slot_number_t slot);
static optiga_slot_map_t* map_put(psa_key_slot_number_t slot, optiga_key_id_t oid, const uint8_t *pub, uint16_t pub_len);
static psa_status_t optiga_psa_generate_key(psa_drv_se_context_t *drv_context, psa_key_slot_number_t key_slot, const psa_key_attributes_t *attributes, uint8_t *pubkey, size_t pubkey_size, size_t *pubkey_length);
static psa_status_t optiga_psa_export_key(psa_drv_se_context_t *drv_context, psa_key_slot_number_t key, uint8_t *p_data, size_t data_size, size_t *p_data_length);
static psa_status_t optiga_psa_sign(psa_drv_se_context_t *ctx, psa_key_slot_number_t key,
		psa_algorithm_t alg, const uint8_t *hash, size_t hash_length, uint8_t *sig,
		size_t sig_size, size_t *sig_len);


static const psa_drv_se_key_management_t OPTIGA_KEY_MANAGEMENT = {
		.MBEDTLS_PRIVATE(p_allocate) = optiga_p_allocate,
		.MBEDTLS_PRIVATE(p_generate) = optiga_psa_generate_key,
		.MBEDTLS_PRIVATE(p_destroy) = optiga_p_destroy,
		.MBEDTLS_PRIVATE(p_export_public) = optiga_psa_export_key,
};

static const psa_drv_se_asymmetric_t OPTIGA_SE_ASYM = {
		.MBEDTLS_PRIVATE(p_sign) = optiga_psa_sign,
		.MBEDTLS_PRIVATE(p_verify) = NULL,
		.MBEDTLS_PRIVATE(p_encrypt) = NULL,
		.MBEDTLS_PRIVATE(p_decrypt) = NULL,
};

static const psa_drv_se_t optiga_psa_driver = {
		.MBEDTLS_PRIVATE(hal_version) = PSA_DRV_SE_HAL_VERSION,
		.MBEDTLS_PRIVATE(persistent_data_size) = 0,
		.MBEDTLS_PRIVATE(p_init) = optiga_psa_init,
		.MBEDTLS_PRIVATE(key_management) = &OPTIGA_KEY_MANAGEMENT,
		.MBEDTLS_PRIVATE(asymmetric) = &OPTIGA_SE_ASYM,

};


static psa_status_t optiga_p_allocate(psa_drv_se_context_t *ctx, void *persistent_data, const psa_key_attributes_t *attr, psa_key_creation_method_t method, psa_key_slot_number_t *key_slot)
{
	(void) ctx;
	(void) persistent_data;
	(void) method;

	/* ECC P-256 key pairs are supported for now */
	// Find free slot
	for (psa_key_slot_number_t slot = 0; slot < OPTIGA_MAX_ACTIVE_KEYS; slot++) {
		if (map_find(slot) == NULL) {
			*key_slot = slot;
			return PSA_SUCCESS;
		}
	}

	return PSA_ERROR_INSUFFICIENT_MEMORY;
}

static psa_status_t optiga_p_destroy(psa_drv_se_context_t *ctx, void *persistent_data, psa_key_slot_number_t key_slot) {
	(void) ctx;
	(void) persistent_data;
	optiga_slot_map_t *m = map_find(key_slot);
	if (m) { m->used = 0; m->pub_len = 0; }
	return PSA_SUCCESS;
}

static optiga_slot_map_t* map_find(psa_key_slot_number_t slot) {
	for (int i = 0; i <OPTIGA_MAX_ACTIVE_KEYS; i++)
	{
		if (g_optiga_map[i].used && g_optiga_map[i].slot == slot)
			return &g_optiga_map[i];
	}
	return NULL;
}

static optiga_slot_map_t* map_put(psa_key_slot_number_t slot,
		optiga_key_id_t oid, const uint8_t *pub, uint16_t pub_len) {
	optiga_slot_map_t *p = map_find(slot);
	if (!p) {
		for (int i = 0; i < OPTIGA_MAX_ACTIVE_KEYS; i++) {
			if (!g_optiga_map[i].used) {
				p = &g_optiga_map[i];
				break;
			}
		}
		if (!p) return NULL; // table full
	}

	p->used = 1;
	p->slot = slot;
	p->oid = oid;

	if (pub_len > sizeof (p->pub)) pub_len = sizeof(p->pub);
	p->pub_len = pub_len;
	for(uint16_t i = 0; i < pub_len; i++) {
		p->pub[i] = pub[i];
	}
	return p;
}

static psa_status_t optiga_psa_init (psa_drv_se_context_t *ctx, void *persistent_data, psa_key_location_t location)
{
	(void)ctx;
	(void)persistent_data;
	(void)location;
	return PSA_SUCCESS;
}

psa_status_t optiga_psa_register(void) {
	return psa_register_se_driver(PSA_KEY_LOCATION_OPTIGA, &optiga_psa_driver);
}


/* Generate opaque key */
static psa_status_t optiga_psa_generate_key(psa_drv_se_context_t *drv_context,
	    psa_key_slot_number_t key_slot,
	    const psa_key_attributes_t *attributes,
	    uint8_t *pubkey, size_t pubkey_size, size_t *pubkey_length)
{
	(void) drv_context; (void) key_slot;

#ifndef OPTIGA_TLS_ATTACH_ONLY
	psa_key_type_t type = psa_get_key_type(attributes);
	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type) ||
			PSA_KEY_TYPE_ECC_GET_FAMILY(type) != PSA_ECC_FAMILY_SECP_R1 ||
			psa_get_key_bits(attributes) != 256)
		return PSA_ERROR_NOT_SUPPORTED;

	/* Map PSA usage to OPTIGA usage bits */
	psa_key_usage_t usage = psa_get_key_usage_flags(attributes);
	uint8_t optiga_usage = 0;

	if (usage & (PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE))
		optiga_usage |= OPTIGA_KEY_USAGE_SIGN;
	if (usage & PSA_KEY_USAGE_DERIVE)
		optiga_usage |= OPTIGA_KEY_USAGE_KEY_AGREEMENT;

	optiga_key_id_t oid = g_optiga_signing_key_oid;
	uint8_t tmp_pub[100];
	uint8_t *out_buf = pubkey;

	uint16_t out_len = (uint16_t) pubkey_size;


	out_buf = tmp_pub;
	out_len = sizeof(tmp_pub);

	optiga_lib_status_t status = trustm_gen_ecc_keypair(oid,
			OPTIGA_ECC_CURVE_NIST_P_256, optiga_usage, false,
			out_buf, &out_len);

	if (status != OPTIGA_LIB_SUCCESS)
		return PSA_ERROR_HARDWARE_FAILURE;


	if (!map_put(key_slot, oid, out_buf, out_len))
		return PSA_ERROR_INSUFFICIENT_MEMORY;
#else
	printf("[PSA-SE] Attaching key slot %lu to OID 0x%04X (no key generation)\n",
		   (unsigned long)key_slot, (unsigned int)g_optiga_signing_key_oid);
	map_put(key_slot, g_optiga_signing_key_oid, NULL, 0);
#endif
	return PSA_SUCCESS;
}

static psa_status_t optiga_psa_export_key(psa_drv_se_context_t *drv_context,
        psa_key_slot_number_t key,
        uint8_t *p_data,
        size_t data_size,
        size_t *p_data_length)
{
	(void) drv_context;
	optiga_slot_map_t *p = map_find(key);

	if (!p) return PSA_ERROR_DOES_NOT_EXIST;

	if (data_size < p->pub_len)
	{
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	for(uint16_t i= 0; i < p->pub_len; i++) {
		p_data[i] = p->pub[i];
	}
	*p_data_length = p->pub_len;
	return PSA_SUCCESS;
}

static psa_status_t optiga_psa_sign(psa_drv_se_context_t *ctx, psa_key_slot_number_t key,
		psa_algorithm_t alg, const uint8_t *hash, size_t hash_length, uint8_t *sig,
		size_t sig_size, size_t *sig_len)
{
	(void) ctx;

	// Only adding ECSA woth SHA-256 on P-256 now
	if (alg != PSA_ALG_ECDSA(PSA_ALG_SHA_256))
		return PSA_ERROR_NOT_SUPPORTED;

	if (hash_length != 32)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (sig_size < 64)
		return PSA_ERROR_BUFFER_TOO_SMALL;

	optiga_slot_map_t *map = map_find(key);

	if (!map) {
		printf("[PSA-Sign] ERROR: No key map found for slot %lu\n", (unsigned long)key);
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	/* DEBUG: Show which OID is being used for signing */
	printf("[PSA-Sign] Using Key OID 0x%04X for TLS CertificateVerify (slot=%lu)\n",
		   (unsigned int)map->oid, (unsigned long)key);

	uint16_t out = (uint16_t)sig_size;
	optiga_lib_status_t status = trustm_ecdsa_sign(map->oid,
			hash, (uint16_t) hash_length, sig, &out);

	if (status != OPTIGA_LIB_SUCCESS)
		return PSA_ERROR_HARDWARE_FAILURE;

	if (sig_len) *sig_len = (size_t)out;
	return PSA_SUCCESS;


}
