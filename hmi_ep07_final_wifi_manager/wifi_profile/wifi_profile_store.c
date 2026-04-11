#include "wifi_profile_store.h"

#include "cy_pdl.h"
#include "cymem_CM55_0.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WIFI_PROFILE_LOG_ENABLE         (1)
#define WIFI_PROFILE_MAGIC              (0x57465031UL) /* "WFP1" */
#define WIFI_PROFILE_VERSION            (1U)
#define WIFI_PROFILE_VALID              (1U)
#define WIFI_PROFILE_SLOT_SIZE          (256U)
#define WIFI_PROFILE_PAYLOAD_LEN        (WIFI_PROFILE_SSID_MAX_LEN + 1U + \
                                         WIFI_PROFILE_PASSWORD_MAX_LEN + 1U + \
                                         WIFI_PROFILE_SECURITY_MAX_LEN + 1U + 1U)

#define WIFI_PROFILE_USER_START         ((uint32_t)CYMEM_CM55_0_user_nvm_C_START)
#define WIFI_PROFILE_USER_SIZE          ((uint32_t)CYMEM_CM55_0_user_nvm_SIZE)
#define WIFI_PROFILE_PRIMARY_ADDR       (WIFI_PROFILE_USER_START + WIFI_PROFILE_USER_SIZE - WIFI_PROFILE_SLOT_SIZE)
#define WIFI_PROFILE_LEGACY_ADDR        (WIFI_PROFILE_USER_START)

/* Fixed-size record for one WiFi profile in CM55 user_nvm. */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t payload_len;
    uint32_t crc32;
    uint8_t valid;
    uint8_t reserved0[3];
    char ssid[WIFI_PROFILE_SSID_MAX_LEN + 1U];
    char password[WIFI_PROFILE_PASSWORD_MAX_LEN + 1U];
    char security[WIFI_PROFILE_SECURITY_MAX_LEN + 1U];
    uint8_t auto_connect;
    uint8_t reserved1[3];
} wifi_profile_record_t;

typedef char wifi_profile_record_size_check[(sizeof(wifi_profile_record_t) <= WIFI_PROFILE_SLOT_SIZE) ? 1 : -1];

static void wifi_profile_log(const char *msg)
{
#if WIFI_PROFILE_LOG_ENABLE
    printf("[PROFILE] %s\r\n", msg);
#else
    CY_UNUSED_PARAMETER(msg);
#endif
}

static void wifi_profile_log_rslt(const char *tag, uint32_t addr, long st)
{
#if WIFI_PROFILE_LOG_ENABLE
    printf("[PROFILE] %s st=%ld addr=0x%08lx\r\n", tag, st, (unsigned long)addr);
#else
    CY_UNUSED_PARAMETER(tag);
    CY_UNUSED_PARAMETER(addr);
    CY_UNUSED_PARAMETER(st);
#endif
}

static uint32_t wifi_profile_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    if(data == NULL) {
        return crc;
    }

    for(uint32_t i = 0U; i < len; i++) {
        crc ^= (uint32_t)data[i];

        for(uint32_t j = 0U; j < 8U; j++) {
            if((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

/* CRC covers only payload fields, not the record header. */
static uint32_t wifi_profile_crc32(const wifi_profile_record_t *rec)
{
    uint32_t crc = 0xFFFFFFFFUL;

    if(rec == NULL) {
        return 0U;
    }

    crc = wifi_profile_crc32_update(crc, (const uint8_t *)rec->ssid, sizeof(rec->ssid));
    crc = wifi_profile_crc32_update(crc, (const uint8_t *)rec->password, sizeof(rec->password));
    crc = wifi_profile_crc32_update(crc, (const uint8_t *)rec->security, sizeof(rec->security));
    crc = wifi_profile_crc32_update(crc, (const uint8_t *)&rec->auto_connect, sizeof(rec->auto_connect));

    return ~crc;
}

static bool wifi_profile_read_slot(uint32_t addr, uint8_t *out)
{
    cy_en_rram_status_t st;

    if(out == NULL) {
        return false;
    }

    st = Cy_RRAM_TSReadByteArray(RRAMC0, addr, out, WIFI_PROFILE_SLOT_SIZE);
    if(st != CY_RRAM_SUCCESS) {
        wifi_profile_log_rslt("READ_FAIL", addr, (long)st);
        return false;
    }

    return true;
}

static bool wifi_profile_write_slot(uint32_t addr, const uint8_t *in)
{
    cy_en_rram_status_t st;

    if(in == NULL) {
        return false;
    }

    st = Cy_RRAM_NvmWriteByteArray(RRAMC0, addr, (uint8_t *)in, WIFI_PROFILE_SLOT_SIZE);
    if(st != CY_RRAM_SUCCESS) {
        wifi_profile_log_rslt("WRITE_FAIL", addr, (long)st);
        return false;
    }

    return true;
}

static bool wifi_profile_is_erased(const uint8_t *block)
{
    if(block == NULL) {
        return true;
    }

    for(uint32_t i = 0U; i < WIFI_PROFILE_SLOT_SIZE; i++) {
        if(block[i] != 0xFFU) {
            return false;
        }
    }

    return true;
}

static void wifi_profile_make_record(wifi_profile_record_t *rec, const wifi_profile_data_t *profile)
{
    if((rec == NULL) || (profile == NULL)) {
        return;
    }

    (void)memset(rec, 0, sizeof(*rec));

    rec->magic = WIFI_PROFILE_MAGIC;
    rec->version = WIFI_PROFILE_VERSION;
    rec->payload_len = WIFI_PROFILE_PAYLOAD_LEN;
    rec->valid = WIFI_PROFILE_VALID;

    (void)strncpy(rec->ssid, profile->ssid, sizeof(rec->ssid) - 1U);
    (void)strncpy(rec->password, profile->password, sizeof(rec->password) - 1U);
    (void)strncpy(rec->security, profile->security, sizeof(rec->security) - 1U);
    rec->ssid[sizeof(rec->ssid) - 1U] = '\0';
    rec->password[sizeof(rec->password) - 1U] = '\0';
    rec->security[sizeof(rec->security) - 1U] = '\0';

    rec->auto_connect = profile->auto_connect ? 1U : 0U;
    rec->crc32 = wifi_profile_crc32(rec);
}

static bool wifi_profile_parse_record(const wifi_profile_record_t *rec,
                                      wifi_profile_data_t *out_profile,
                                      uint32_t addr)
{
    uint32_t expected_crc;

    if((rec == NULL) || (out_profile == NULL)) {
        return false;
    }

    if((rec->magic == 0xFFFFFFFFUL) || (rec->valid == 0xFFU)) {
        return false;
    }

    if((rec->magic != WIFI_PROFILE_MAGIC) ||
       (rec->version != WIFI_PROFILE_VERSION) ||
       (rec->valid != WIFI_PROFILE_VALID) ||
       (rec->payload_len != WIFI_PROFILE_PAYLOAD_LEN)) {
#if WIFI_PROFILE_LOG_ENABLE
        printf("[PROFILE] HEADER_FAIL addr=0x%08lx magic=0x%08lx ver=%u valid=%u len=%u\r\n",
               (unsigned long)addr,
               (unsigned long)rec->magic,
               (unsigned int)rec->version,
               (unsigned int)rec->valid,
               (unsigned int)rec->payload_len);
#endif
        return false;
    }

    expected_crc = wifi_profile_crc32(rec);
    if(expected_crc != rec->crc32) {
#if WIFI_PROFILE_LOG_ENABLE
        printf("[PROFILE] CRC_FAIL addr=0x%08lx expected=0x%08lx actual=0x%08lx\r\n",
               (unsigned long)addr,
               (unsigned long)expected_crc,
               (unsigned long)rec->crc32);
#endif
        return false;
    }

    (void)memset(out_profile, 0, sizeof(*out_profile));
    (void)strncpy(out_profile->ssid, rec->ssid, sizeof(out_profile->ssid) - 1U);
    (void)strncpy(out_profile->password, rec->password, sizeof(out_profile->password) - 1U);
    (void)strncpy(out_profile->security, rec->security, sizeof(out_profile->security) - 1U);
    out_profile->ssid[sizeof(out_profile->ssid) - 1U] = '\0';
    out_profile->password[sizeof(out_profile->password) - 1U] = '\0';
    out_profile->security[sizeof(out_profile->security) - 1U] = '\0';
    out_profile->auto_connect = (rec->auto_connect != 0U);

    return true;
}

static bool wifi_profile_load_from_addr(uint32_t addr, wifi_profile_data_t *out_profile)
{
    uint8_t block[WIFI_PROFILE_SLOT_SIZE];
    wifi_profile_record_t rec;

    if(out_profile == NULL) {
        return false;
    }

    if(!wifi_profile_read_slot(addr, block)) {
        return false;
    }

    if(wifi_profile_is_erased(block)) {
        return false;
    }

    (void)memset(&rec, 0, sizeof(rec));
    (void)memcpy(&rec, block, sizeof(rec));

    return wifi_profile_parse_record(&rec, out_profile, addr);
}

bool wifi_profile_store_load(wifi_profile_data_t *out_profile)
{
    if(out_profile == NULL) {
        return false;
    }

    if(wifi_profile_load_from_addr(WIFI_PROFILE_PRIMARY_ADDR, out_profile)) {
        wifi_profile_log("LOAD_OK");
        return true;
    }

    if((WIFI_PROFILE_PRIMARY_ADDR != WIFI_PROFILE_LEGACY_ADDR) &&
       wifi_profile_load_from_addr(WIFI_PROFILE_LEGACY_ADDR, out_profile)) {
        wifi_profile_log("LOAD_OK_LEGACY");
        return true;
    }

    wifi_profile_log("LOAD_EMPTY");
    return false;
}

bool wifi_profile_store_save(const wifi_profile_data_t *profile)
{
    uint8_t block[WIFI_PROFILE_SLOT_SIZE];
    uint8_t verify_block[WIFI_PROFILE_SLOT_SIZE];
    uint8_t current_block[WIFI_PROFILE_SLOT_SIZE];
    wifi_profile_record_t rec;

    if(profile == NULL) {
        return false;
    }

    (void)memset(block, 0xFF, sizeof(block));
    wifi_profile_make_record(&rec, profile);
    (void)memcpy(block, &rec, sizeof(rec));

    /* Skip write if content is unchanged to reduce NVM wear. */
    if(wifi_profile_read_slot(WIFI_PROFILE_PRIMARY_ADDR, current_block)) {
        if(0 == memcmp(current_block, block, sizeof(block))) {
            wifi_profile_log("SAVE_SKIP_SAME");
            return true;
        }
    }

    if(!wifi_profile_write_slot(WIFI_PROFILE_PRIMARY_ADDR, block)) {
        return false;
    }

    if(!wifi_profile_read_slot(WIFI_PROFILE_PRIMARY_ADDR, verify_block)) {
        return false;
    }

    if(0 != memcmp(verify_block, block, sizeof(block))) {
        wifi_profile_log("SAVE_VERIFY_FAIL");
        return false;
    }

    wifi_profile_log("SAVE_OK");
    return true;
}

bool wifi_profile_store_clear(void)
{
    uint8_t block[WIFI_PROFILE_SLOT_SIZE];
    bool ok_primary;
    bool ok_legacy = true;

    (void)memset(block, 0xFF, sizeof(block));

    ok_primary = wifi_profile_write_slot(WIFI_PROFILE_PRIMARY_ADDR, block);
    if(WIFI_PROFILE_PRIMARY_ADDR != WIFI_PROFILE_LEGACY_ADDR) {
        ok_legacy = wifi_profile_write_slot(WIFI_PROFILE_LEGACY_ADDR, block);
    }

    if(ok_primary && ok_legacy) {
        wifi_profile_log("CLEAR_OK");
    } else {
        wifi_profile_log("CLEAR_FAIL");
    }

    return (ok_primary && ok_legacy);
}
