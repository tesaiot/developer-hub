#ifndef WIFI_SCAN_TYPES_H
#define WIFI_SCAN_TYPES_H

#include <stdint.h>

#define WIFI_SCAN_MAX_APS           (12U)
#define WIFI_SCAN_SSID_MAX_LEN      (32U)
#define WIFI_SCAN_SECURITY_MAX_LEN  (15U)

typedef struct {
    char ssid[WIFI_SCAN_SSID_MAX_LEN + 1U];
    int16_t rssi;
    char security[WIFI_SCAN_SECURITY_MAX_LEN + 1U];
} wifi_scan_ap_t;

#endif /* WIFI_SCAN_TYPES_H */