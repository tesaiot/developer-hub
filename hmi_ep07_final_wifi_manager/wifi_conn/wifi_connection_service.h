#ifndef WIFI_CONNECTION_SERVICE_H
#define WIFI_CONNECTION_SERVICE_H

#include "wifi_profile_types.h"

#include "cy_result.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WIFI_CONN_STATE_IDLE = 0,
    WIFI_CONN_STATE_CONNECTING,
    WIFI_CONN_STATE_CONNECTED,
    WIFI_CONN_STATE_DISCONNECTING,
    WIFI_CONN_STATE_RECONNECT_WAIT,
    WIFI_CONN_STATE_ERROR
} wifi_conn_state_t;

typedef struct {
    wifi_conn_state_t state;
    bool initialized;
    bool connected;
    bool reconnect_enabled;
    bool retry_pending;
    bool have_profile;
    bool internet_ok;
    uint8_t retry_stage;
    uint8_t ping_fail_streak;
    uint32_t retry_wait_ms;
    uint32_t disconnect_reason;
    cy_rslt_t last_rslt;
    cy_rslt_t last_ping_rslt;
    int16_t rssi;
    char ssid[WIFI_PROFILE_SSID_MAX_LEN + 1U];
    char security[WIFI_PROFILE_SECURITY_MAX_LEN + 1U];
    char ip_addr[16];
} wifi_connection_snapshot_t;

bool wifi_connection_service_init(void);
bool wifi_connection_service_connect_profile(const wifi_profile_data_t *profile, bool user_initiated);
void wifi_connection_service_disconnect(void);
void wifi_connection_service_retry_now(void);
bool wifi_connection_service_get_snapshot(wifi_connection_snapshot_t *out_snapshot);
const char *wifi_connection_service_state_text(wifi_conn_state_t state);

#endif /* WIFI_CONNECTION_SERVICE_H */
