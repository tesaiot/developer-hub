#ifndef WIFI_SCAN_SERVICE_H
#define WIFI_SCAN_SERVICE_H

#include "wifi_scan_types.h"
#include "cy_result.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    wifi_scan_ap_t aps[WIFI_SCAN_MAX_APS];
    uint16_t ap_count;
    uint16_t scan_sequence;
    bool scanning;
    bool radio_ready;
    volatile bool scan_done_pending;
    volatile bool scan_error_pending;
    uint32_t last_rslt;
} wifi_scan_service_t;

void wifi_scan_service_init(wifi_scan_service_t *service);
cy_rslt_t wifi_scan_service_preinit(void);
bool wifi_scan_service_start(wifi_scan_service_t *service);
bool wifi_scan_service_process(wifi_scan_service_t *service);
const wifi_scan_ap_t *wifi_scan_service_get_list(const wifi_scan_service_t *service, uint16_t *count);

#endif /* WIFI_SCAN_SERVICE_H */
