#include <string.h>
#include "psa/internal_trusted_storage.h"

/* ---- tiny RAM-only ITS store (bring-up only; not persistent) ---- */

#ifndef PSA_ITS_MAX_OBJECTS
#define PSA_ITS_MAX_OBJECTS   8  /* Reduced from 32 to save 6.7 KB RAM (98.88% usage crisis) */
#endif

#ifndef PSA_ITS_MAX_DATA_LEN
#define PSA_ITS_MAX_DATA_LEN  256
#endif

typedef struct {
    psa_storage_uid_t uid;
    size_t size;
    psa_storage_create_flags_t flags;
    uint8_t data[PSA_ITS_MAX_DATA_LEN];
    int used;
} its_slot_t;

static its_slot_t slots[PSA_ITS_MAX_OBJECTS];

/* helpers */
static int find_slot(psa_storage_uid_t uid) {
    for (int i = 0; i < PSA_ITS_MAX_OBJECTS; i++)
        if (slots[i].used && slots[i].uid == uid) return i;
    return -1;
}
static int find_free(void) {
    for (int i = 0; i < PSA_ITS_MAX_OBJECTS; i++)
        if (!slots[i].used) return i;
    return -1;
}

psa_status_t psa_its_set(psa_storage_uid_t uid,
                         size_t data_length,
                         const void *p_data,
                         psa_storage_create_flags_t create_flags)
{
    if (data_length == 0) {
    	int idx = find_slot(uid);
    	if (idx >= 0) { slots[idx].used = 0; slots[idx].size = 0; }
    	return PSA_SUCCESS;
    }
    if (data_length > PSA_ITS_MAX_DATA_LEN)
        return PSA_ERROR_INSUFFICIENT_STORAGE;

    int idx = find_slot(uid);
    if (idx < 0) {
        idx = find_free();
        if (idx < 0) return PSA_ERROR_INSUFFICIENT_STORAGE;
        slots[idx].uid  = uid;
        slots[idx].used = 1;
    }

    memcpy(slots[idx].data, p_data, data_length);
    slots[idx].size  = data_length;
    slots[idx].flags = create_flags;
    return PSA_SUCCESS;
}

psa_status_t psa_its_get(psa_storage_uid_t uid,
                         size_t offset,
                         size_t size,
                         void *p_data,
                         size_t *p_data_length)
{
    int idx = find_slot(uid);
    if (idx < 0 || slots[idx].size == 0) {
        if (p_data_length) *p_data_length = 0;
        return PSA_ERROR_DOES_NOT_EXIST;  // critical for psa_crypto_init()
    }
    if (offset > slots[idx].size) return PSA_ERROR_INVALID_ARGUMENT;

    size_t n = size;
    if (n > (slots[idx].size - offset)) n = slots[idx].size - offset;

    memcpy(p_data, slots[idx].data + offset, n);
    if (p_data_length) *p_data_length = n;
    return PSA_SUCCESS;
}

/* get_info */
psa_status_t psa_its_get_info(psa_storage_uid_t uid,
                              psa_storage_info_t *p_info)
{
    int idx = find_slot(uid);
    if (idx < 0 || slots[idx].size == 0) return PSA_ERROR_DOES_NOT_EXIST;
    p_info->size  = slots[idx].size;
    p_info->flags = slots[idx].flags;
    return PSA_SUCCESS;
}

/* remove */
psa_status_t psa_its_remove(psa_storage_uid_t uid)
{
    int idx = find_slot(uid);
    if (idx >= 0)
    {
    	slots[idx].used = 0;
    	slots[idx].size = 0;
    }
    return PSA_SUCCESS;
}
