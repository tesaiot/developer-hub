#ifndef WIFI_PROFILE_TYPES_H
#define WIFI_PROFILE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define WIFI_PROFILE_SSID_MAX_LEN       (32U)
#define WIFI_PROFILE_PASSWORD_MAX_LEN   (64U)
#define WIFI_PROFILE_SECURITY_MAX_LEN   (15U)

typedef struct {
    char ssid[WIFI_PROFILE_SSID_MAX_LEN + 1U];
    char password[WIFI_PROFILE_PASSWORD_MAX_LEN + 1U];
    char security[WIFI_PROFILE_SECURITY_MAX_LEN + 1U];
    bool auto_connect;
} wifi_profile_data_t;

#endif /* WIFI_PROFILE_TYPES_H */