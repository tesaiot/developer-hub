#include "wifi_connection_service.h"

#include "wifi_scan_service.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "cy_wcm.h"
#include "lwip/def.h"

#include <stdio.h>
#include <string.h>

#define WIFI_CONN_LOG_ENABLE              (1)
#define WIFI_CONN_TASK_STACK_WORDS        (2048U)
#define WIFI_CONN_TASK_PRIORITY           (configMAX_PRIORITIES - 3)
#define WIFI_CONN_QUEUE_LEN               (8U)
#define WIFI_CONN_CONNECTED_POLL_MS       (1000U)
#define WIFI_CONN_PING_INTERVAL_MS        (20000U)
#define WIFI_CONN_PING_TIMEOUT_MS         (1200U)
#define WIFI_CONN_PING_FAIL_THRESHOLD     (3U)

/* Commands are serialized through one queue to avoid overlapping WiFi operations. */
typedef enum {
    WIFI_CONN_CMD_CONNECT_PROFILE = 0,
    WIFI_CONN_CMD_DISCONNECT_USER,
    WIFI_CONN_CMD_RETRY_NOW,
    WIFI_CONN_CMD_EVENT_DISCONNECTED
} wifi_conn_cmd_type_t;

typedef struct {
    wifi_conn_cmd_type_t type;
    wifi_profile_data_t profile;
    bool user_initiated;
    uint32_t reason;
} wifi_conn_cmd_t;

typedef struct {
    bool initialized;
    QueueHandle_t cmd_queue;
    SemaphoreHandle_t lock;
    TaskHandle_t task;

    wifi_conn_state_t state;
    wifi_profile_data_t active_profile;
    bool have_profile;
    bool reconnect_enabled;
    bool manual_disconnect;

    bool retry_pending;
    uint8_t retry_stage;
    uint32_t retry_wait_ms;

    bool internet_ok;
    uint8_t ping_fail_streak;
    cy_rslt_t last_ping_rslt;
    TickType_t next_ping_tick;

    uint32_t disconnect_reason;
    cy_rslt_t last_rslt;
    int16_t rssi;
    char ssid[WIFI_PROFILE_SSID_MAX_LEN + 1U];
    char security[WIFI_PROFILE_SECURITY_MAX_LEN + 1U];
    char ip_addr[16];
} wifi_conn_ctx_t;

/* Single runtime context owned by wifi_conn_task. */
static wifi_conn_ctx_t s_ctx;

static void wifi_conn_log(const char *msg)
{
#if WIFI_CONN_LOG_ENABLE
    printf("[WIFI_CONN] %s\r\n", msg);
#else
    (void)msg;
#endif
}

static void wifi_conn_log_rslt(const char *tag, cy_rslt_t rslt)
{
#if WIFI_CONN_LOG_ENABLE
    printf("[WIFI_CONN] %s rslt=0x%08lx\r\n", tag, (unsigned long)rslt);
#else
    (void)tag;
    (void)rslt;
#endif
}

static const uint32_t s_reconnect_backoff_ms[] = { 1000U, 2000U, 5000U, 10000U };

const char *wifi_connection_service_state_text(wifi_conn_state_t state)
{
    switch(state) {
    case WIFI_CONN_STATE_IDLE:
        return "Idle";
    case WIFI_CONN_STATE_CONNECTING:
        return "Connecting";
    case WIFI_CONN_STATE_CONNECTED:
        return "Connected";
    case WIFI_CONN_STATE_DISCONNECTING:
        return "Disconnecting";
    case WIFI_CONN_STATE_RECONNECT_WAIT:
        return "Reconnect Wait";
    case WIFI_CONN_STATE_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

static cy_wcm_security_t wifi_conn_security_from_text(const char *security)
{
    if(security == NULL) {
        return CY_WCM_SECURITY_WPA2_AES_PSK;
    }

    if(strstr(security, "OPEN") != NULL) {
        return CY_WCM_SECURITY_OPEN;
    }

    if(strstr(security, "WPA3") != NULL) {
        return CY_WCM_SECURITY_WPA3_SAE;
    }

    if(strstr(security, "WPA2") != NULL) {
        return CY_WCM_SECURITY_WPA2_AES_PSK;
    }

    if(strstr(security, "WPA") != NULL) {
        return CY_WCM_SECURITY_WPA_AES_PSK;
    }

    return CY_WCM_SECURITY_WPA2_AES_PSK;
}

static bool wifi_conn_profile_is_valid(const wifi_profile_data_t *profile)
{
    cy_wcm_security_t sec;

    if((profile == NULL) || (profile->ssid[0] == '\0')) {
        return false;
    }

    sec = wifi_conn_security_from_text(profile->security);
    if((sec != CY_WCM_SECURITY_OPEN) && (profile->password[0] == '\0')) {
        return false;
    }

    return true;
}

static void wifi_conn_ipv4_to_string(const cy_wcm_ip_address_t *ip_addr, char *out, uint32_t out_len)
{
    uint32_t host_order;

    if((out == NULL) || (out_len == 0U)) {
        return;
    }

    (void)snprintf(out, out_len, "-");

    if((ip_addr == NULL) || (ip_addr->version != CY_WCM_IP_VER_V4)) {
        return;
    }

    host_order = lwip_ntohl(ip_addr->ip.v4);
    (void)snprintf(out,
                   out_len,
                   "%lu.%lu.%lu.%lu",
                   (unsigned long)((host_order >> 24) & 0xFFU),
                   (unsigned long)((host_order >> 16) & 0xFFU),
                   (unsigned long)((host_order >> 8) & 0xFFU),
                   (unsigned long)(host_order & 0xFFU));
}

static void wifi_conn_set_state_locked(wifi_conn_state_t state)
{
    s_ctx.state = state;
}

static void wifi_conn_reset_ping_monitor_locked(void)
{
    s_ctx.internet_ok = true;
    s_ctx.ping_fail_streak = 0U;
    s_ctx.last_ping_rslt = CY_RSLT_SUCCESS;
    s_ctx.next_ping_tick = xTaskGetTickCount();
}

/* Reconnect backoff: 1s -> 2s -> 5s -> 10s (cap at max stage). */
static bool wifi_conn_schedule_retry_locked(void)
{
    uint8_t idx;

    if(!s_ctx.reconnect_enabled || !s_ctx.have_profile) {
        return false;
    }

    idx = s_ctx.retry_stage;
    if(idx >= (uint8_t)(sizeof(s_reconnect_backoff_ms) / sizeof(s_reconnect_backoff_ms[0]))) {
        idx = (uint8_t)(sizeof(s_reconnect_backoff_ms) / sizeof(s_reconnect_backoff_ms[0])) - 1U;
    }

    s_ctx.retry_wait_ms = s_reconnect_backoff_ms[idx];
    if(s_ctx.retry_stage < ((uint8_t)(sizeof(s_reconnect_backoff_ms) / sizeof(s_reconnect_backoff_ms[0])) - 1U)) {
        s_ctx.retry_stage++;
    }

    s_ctx.retry_pending = true;
    s_ctx.internet_ok = false;
    wifi_conn_set_state_locked(WIFI_CONN_STATE_RECONNECT_WAIT);

#if WIFI_CONN_LOG_ENABLE
    printf("[WIFI_CONN] RECONNECT_SCHEDULED wait_ms=%lu stage=%u\r\n",
           (unsigned long)s_ctx.retry_wait_ms,
           (unsigned int)s_ctx.retry_stage);
#endif

    return true;
}

static void wifi_conn_refresh_connected_info_locked(void)
{
    cy_wcm_associated_ap_info_t ap_info;
    cy_wcm_ip_address_t ip_addr;

    if(s_ctx.state != WIFI_CONN_STATE_CONNECTED) {
        return;
    }

    if(0U == cy_wcm_is_connected_to_ap()) {
        (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");
        s_ctx.rssi = 0;
        s_ctx.internet_ok = false;

        if(!s_ctx.manual_disconnect) {
            (void)wifi_conn_schedule_retry_locked();
        } else {
            wifi_conn_set_state_locked(WIFI_CONN_STATE_IDLE);
        }
        return;
    }

    if(CY_RSLT_SUCCESS == cy_wcm_get_associated_ap_info(&ap_info)) {
        (void)strncpy(s_ctx.ssid, (const char *)ap_info.SSID, sizeof(s_ctx.ssid) - 1U);
        s_ctx.ssid[sizeof(s_ctx.ssid) - 1U] = '\0';
        s_ctx.rssi = ap_info.signal_strength;
    }

    if(CY_RSLT_SUCCESS == cy_wcm_get_ip_addr(CY_WCM_INTERFACE_TYPE_STA, &ip_addr)) {
        wifi_conn_ipv4_to_string(&ip_addr, s_ctx.ip_addr, sizeof(s_ctx.ip_addr));
    } else {
        (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");
    }
}

/* Ping gateway periodically to indicate internet reachability on Home page. */
static void wifi_conn_probe_internet_once(void)
{
    cy_wcm_ip_address_t gateway_addr;
    uint32_t elapsed_ms = 0U;
    cy_rslt_t result;
    bool state_changed_up = false;
    bool state_changed_down = false;

    (void)memset(&gateway_addr, 0, sizeof(gateway_addr));

    result = cy_wcm_get_gateway_ip_address(CY_WCM_INTERFACE_TYPE_STA, &gateway_addr);
    if(CY_RSLT_SUCCESS == result) {
        result = cy_wcm_ping(CY_WCM_INTERFACE_TYPE_STA,
                             &gateway_addr,
                             WIFI_CONN_PING_TIMEOUT_MS,
                             &elapsed_ms);
    }

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    if(s_ctx.state != WIFI_CONN_STATE_CONNECTED) {
        (void)xSemaphoreGive(s_ctx.lock);
        return;
    }

    s_ctx.last_ping_rslt = result;
    s_ctx.next_ping_tick = xTaskGetTickCount() + pdMS_TO_TICKS(WIFI_CONN_PING_INTERVAL_MS);

    if(CY_RSLT_SUCCESS == result) {
        if(!s_ctx.internet_ok) {
            state_changed_up = true;
        }
        s_ctx.internet_ok = true;
        s_ctx.ping_fail_streak = 0U;
    } else {
        if(s_ctx.ping_fail_streak < 0xFFU) {
            s_ctx.ping_fail_streak++;
        }

        if(s_ctx.ping_fail_streak >= WIFI_CONN_PING_FAIL_THRESHOLD) {
            if(s_ctx.internet_ok) {
                state_changed_down = true;
            }
            s_ctx.internet_ok = false;
        }
    }

    (void)xSemaphoreGive(s_ctx.lock);

    if(state_changed_up) {
#if WIFI_CONN_LOG_ENABLE
        printf("[WIFI_CONN] INTERNET_OK ping_ms=%lu\r\n", (unsigned long)elapsed_ms);
#endif
        return;
    }

    if(state_changed_down) {
        wifi_conn_log_rslt("INTERNET_DOWN", result);
    }
}

static void wifi_conn_attempt_connect(const char *tag)
{
    wifi_profile_data_t profile;
    cy_wcm_connect_params_t params;
    cy_wcm_ip_address_t ip_addr;
    cy_wcm_security_t sec;
    cy_rslt_t result;

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    if(!s_ctx.have_profile || !wifi_conn_profile_is_valid(&s_ctx.active_profile)) {
        s_ctx.last_rslt = CY_RSLT_TYPE_ERROR;
        s_ctx.internet_ok = false;
        wifi_conn_set_state_locked(WIFI_CONN_STATE_ERROR);
        (void)xSemaphoreGive(s_ctx.lock);
        wifi_conn_log("CONNECT_SKIP invalid profile");
        return;
    }

    profile = s_ctx.active_profile;
    s_ctx.retry_pending = false;
    s_ctx.retry_wait_ms = 0U;
    s_ctx.ping_fail_streak = 0U;
    s_ctx.last_ping_rslt = CY_RSLT_TYPE_ERROR;
    wifi_conn_set_state_locked(WIFI_CONN_STATE_CONNECTING);
    (void)xSemaphoreGive(s_ctx.lock);

#if WIFI_CONN_LOG_ENABLE
    printf("[WIFI_CONN] CONNECT_START tag=%s ssid=%s\r\n", tag, profile.ssid);
#endif

    (void)memset(&params, 0, sizeof(params));
    (void)memset(&ip_addr, 0, sizeof(ip_addr));

    sec = wifi_conn_security_from_text(profile.security);
    params.ap_credentials.security = sec;
    params.band = CY_WCM_WIFI_BAND_ANY;
    params.static_ip_settings = NULL;

    (void)strncpy((char *)params.ap_credentials.SSID,
                  profile.ssid,
                  sizeof(params.ap_credentials.SSID) - 1U);

    if(sec != CY_WCM_SECURITY_OPEN) {
        (void)strncpy((char *)params.ap_credentials.password,
                      profile.password,
                      sizeof(params.ap_credentials.password) - 1U);
    }

    result = cy_wcm_connect_ap(&params, &ip_addr);

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    s_ctx.last_rslt = result;

    if(CY_RSLT_SUCCESS == result) {
        s_ctx.manual_disconnect = false;
        s_ctx.reconnect_enabled = true;
        s_ctx.retry_pending = false;
        s_ctx.retry_stage = 0U;
        wifi_conn_set_state_locked(WIFI_CONN_STATE_CONNECTED);

        (void)strncpy(s_ctx.ssid, profile.ssid, sizeof(s_ctx.ssid) - 1U);
        s_ctx.ssid[sizeof(s_ctx.ssid) - 1U] = '\0';
        (void)strncpy(s_ctx.security, profile.security, sizeof(s_ctx.security) - 1U);
        s_ctx.security[sizeof(s_ctx.security) - 1U] = '\0';

        wifi_conn_ipv4_to_string(&ip_addr, s_ctx.ip_addr, sizeof(s_ctx.ip_addr));
        wifi_conn_reset_ping_monitor_locked();
        wifi_conn_refresh_connected_info_locked();

        (void)xSemaphoreGive(s_ctx.lock);
        wifi_conn_log("CONNECT_OK");
        return;
    }

    wifi_conn_set_state_locked(WIFI_CONN_STATE_ERROR);
    s_ctx.internet_ok = false;

    if(!s_ctx.manual_disconnect) {
        (void)wifi_conn_schedule_retry_locked();
    }

    (void)xSemaphoreGive(s_ctx.lock);
    wifi_conn_log_rslt("CONNECT_FAIL", result);
}

static void wifi_conn_handle_disconnect_event(uint32_t reason)
{
    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    s_ctx.disconnect_reason = reason;
    s_ctx.internet_ok = false;
    s_ctx.ping_fail_streak = 0U;

#if WIFI_CONN_LOG_ENABLE
    printf("[WIFI_CONN] DISCONNECTED_EVENT reason=%lu\r\n", (unsigned long)reason);
#endif

    if(s_ctx.manual_disconnect) {
        s_ctx.retry_pending = false;
        s_ctx.retry_wait_ms = 0U;
        s_ctx.reconnect_enabled = false;
        wifi_conn_set_state_locked(WIFI_CONN_STATE_IDLE);
        (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");
        s_ctx.rssi = 0;
        (void)xSemaphoreGive(s_ctx.lock);
        return;
    }

    if(!wifi_conn_schedule_retry_locked()) {
        wifi_conn_set_state_locked(WIFI_CONN_STATE_IDLE);
    }

    (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");
    s_ctx.rssi = 0;
    (void)xSemaphoreGive(s_ctx.lock);
}

static void wifi_conn_do_user_disconnect(void)
{
    cy_rslt_t result;

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    s_ctx.manual_disconnect = true;
    s_ctx.reconnect_enabled = false;
    s_ctx.retry_pending = false;
    s_ctx.retry_wait_ms = 0U;
    s_ctx.retry_stage = 0U;
    s_ctx.internet_ok = false;
    s_ctx.ping_fail_streak = 0U;
    wifi_conn_set_state_locked(WIFI_CONN_STATE_DISCONNECTING);
    (void)xSemaphoreGive(s_ctx.lock);

    result = cy_wcm_disconnect_ap();

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return;
    }

    s_ctx.last_rslt = result;
    wifi_conn_set_state_locked(WIFI_CONN_STATE_IDLE);
    (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");
    s_ctx.rssi = 0;
    (void)xSemaphoreGive(s_ctx.lock);

    if(CY_RSLT_SUCCESS == result) {
        wifi_conn_log("DISCONNECT_OK");
    } else {
        wifi_conn_log_rslt("DISCONNECT_FAIL", result);
    }
}

static void wifi_conn_process_cmd(const wifi_conn_cmd_t *cmd)
{
    if(cmd == NULL) {
        return;
    }

    switch(cmd->type) {
    case WIFI_CONN_CMD_CONNECT_PROFILE:
        if(pdTRUE == xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
            s_ctx.active_profile = cmd->profile;
            s_ctx.have_profile = true;
            s_ctx.manual_disconnect = false;
            s_ctx.reconnect_enabled = true;
            s_ctx.retry_pending = false;
            s_ctx.retry_stage = 0U;
            (void)xSemaphoreGive(s_ctx.lock);
        }

        wifi_conn_attempt_connect(cmd->user_initiated ? "USER" : "AUTO");
        break;

    case WIFI_CONN_CMD_DISCONNECT_USER:
        wifi_conn_do_user_disconnect();
        break;

    case WIFI_CONN_CMD_RETRY_NOW:
        if(pdTRUE == xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
            s_ctx.manual_disconnect = false;
            s_ctx.reconnect_enabled = true;
            s_ctx.retry_pending = false;
            s_ctx.retry_stage = 0U;
            (void)xSemaphoreGive(s_ctx.lock);
        }

        wifi_conn_attempt_connect("RETRY_NOW");
        break;

    case WIFI_CONN_CMD_EVENT_DISCONNECTED:
        wifi_conn_handle_disconnect_event(cmd->reason);
        break;

    default:
        break;
    }
}

/* WCM callback only posts commands/logs; UI updates happen via snapshot polling. */
static void wifi_conn_wcm_event_callback(cy_wcm_event_t event, cy_wcm_event_data_t *event_data)
{
    wifi_conn_cmd_t cmd;

    if((!s_ctx.initialized) || (s_ctx.cmd_queue == NULL)) {
        return;
    }

    switch(event) {
    case CY_WCM_EVENT_DISCONNECTED:
        (void)memset(&cmd, 0, sizeof(cmd));
        cmd.type = WIFI_CONN_CMD_EVENT_DISCONNECTED;
        cmd.reason = (event_data != NULL) ? (uint32_t)event_data->reason : 0U;
        (void)xQueueSend(s_ctx.cmd_queue, &cmd, 0U);
        break;

    case CY_WCM_EVENT_CONNECTED:
        wifi_conn_log("EVENT_CONNECTED");
        break;

    case CY_WCM_EVENT_RECONNECTED:
        wifi_conn_log("EVENT_RECONNECTED");
        break;

    case CY_WCM_EVENT_INITIATED_RETRY:
        wifi_conn_log("EVENT_INITIATED_RETRY");
        break;

    default:
        break;
    }
}

/* Worker task: process commands, run retry timer, refresh link info, run ping monitor. */
static void wifi_conn_task(void *arg)
{
    wifi_conn_cmd_t cmd;

    (void)arg;

    for(;;) {
        TickType_t wait_ticks = portMAX_DELAY;
        bool retry_pending;
        bool connected;
        uint32_t retry_wait_ms;

        if(pdTRUE == xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
            retry_pending = s_ctx.retry_pending;
            connected = (s_ctx.state == WIFI_CONN_STATE_CONNECTED);
            retry_wait_ms = s_ctx.retry_wait_ms;
            (void)xSemaphoreGive(s_ctx.lock);
        } else {
            retry_pending = false;
            connected = false;
            retry_wait_ms = 0U;
        }

        if(retry_pending) {
            wait_ticks = pdMS_TO_TICKS(retry_wait_ms);
        } else if(connected) {
            wait_ticks = pdMS_TO_TICKS(WIFI_CONN_CONNECTED_POLL_MS);
        }

        if(xQueueReceive(s_ctx.cmd_queue, &cmd, wait_ticks) == pdPASS) {
            wifi_conn_process_cmd(&cmd);
            continue;
        }

        if(pdTRUE == xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
            retry_pending = s_ctx.retry_pending;
            connected = (s_ctx.state == WIFI_CONN_STATE_CONNECTED);
            if(retry_pending) {
                s_ctx.retry_pending = false;
            }
            (void)xSemaphoreGive(s_ctx.lock);
        } else {
            retry_pending = false;
            connected = false;
        }

        if(retry_pending) {
            wifi_conn_attempt_connect("AUTO_RETRY");
            continue;
        }

        if(connected) {
            bool ping_due = false;
            TickType_t now = xTaskGetTickCount();

            if(pdTRUE == xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
                if((s_ctx.state == WIFI_CONN_STATE_CONNECTED) &&
                   ((int32_t)(now - s_ctx.next_ping_tick) >= 0)) {
                    ping_due = true;
                }
                wifi_conn_refresh_connected_info_locked();
                (void)xSemaphoreGive(s_ctx.lock);
            }

            if(ping_due) {
                wifi_conn_probe_internet_once();
            }
        }
    }
}

bool wifi_connection_service_init(void)
{
    cy_rslt_t result;

    if(s_ctx.initialized) {
        return true;
    }

    (void)memset(&s_ctx, 0, sizeof(s_ctx));
    wifi_conn_set_state_locked(WIFI_CONN_STATE_IDLE);
    s_ctx.internet_ok = false;
    s_ctx.last_ping_rslt = CY_RSLT_TYPE_ERROR;
    s_ctx.next_ping_tick = xTaskGetTickCount() + pdMS_TO_TICKS(WIFI_CONN_PING_INTERVAL_MS);
    (void)snprintf(s_ctx.ip_addr, sizeof(s_ctx.ip_addr), "-");

    result = wifi_scan_service_preinit();
    if(CY_RSLT_SUCCESS != result) {
        wifi_conn_log_rslt("INIT_FAIL_RADIO", result);
        return false;
    }

    s_ctx.lock = xSemaphoreCreateMutex();
    s_ctx.cmd_queue = xQueueCreate(WIFI_CONN_QUEUE_LEN, sizeof(wifi_conn_cmd_t));

    if((s_ctx.lock == NULL) || (s_ctx.cmd_queue == NULL)) {
        wifi_conn_log("INIT_FAIL_RSRC");
        return false;
    }

    result = cy_wcm_register_event_callback(wifi_conn_wcm_event_callback);
    if(CY_RSLT_SUCCESS != result) {
        wifi_conn_log_rslt("INIT_FAIL_EVT", result);
        return false;
    }

    if(pdPASS != xTaskCreate(wifi_conn_task,
                             "wifi_conn_task",
                             WIFI_CONN_TASK_STACK_WORDS,
                             NULL,
                             WIFI_CONN_TASK_PRIORITY,
                             &s_ctx.task)) {
        wifi_conn_log("INIT_FAIL_TASK");
        return false;
    }

    s_ctx.initialized = true;
    wifi_conn_log("INIT_OK");

    return true;
}

bool wifi_connection_service_connect_profile(const wifi_profile_data_t *profile, bool user_initiated)
{
    wifi_conn_cmd_t cmd;

    if((!s_ctx.initialized) || (profile == NULL) || !wifi_conn_profile_is_valid(profile)) {
        return false;
    }

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.type = WIFI_CONN_CMD_CONNECT_PROFILE;
    cmd.profile = *profile;
    cmd.user_initiated = user_initiated;

    if(pdPASS != xQueueSend(s_ctx.cmd_queue, &cmd, 0U)) {
        wifi_conn_log("CONNECT_QUEUE_FULL");
        return false;
    }

    return true;
}

void wifi_connection_service_disconnect(void)
{
    wifi_conn_cmd_t cmd;

    if((!s_ctx.initialized) || (s_ctx.cmd_queue == NULL)) {
        return;
    }

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.type = WIFI_CONN_CMD_DISCONNECT_USER;
    (void)xQueueSend(s_ctx.cmd_queue, &cmd, 0U);
}

void wifi_connection_service_retry_now(void)
{
    wifi_conn_cmd_t cmd;

    if((!s_ctx.initialized) || (s_ctx.cmd_queue == NULL)) {
        return;
    }

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.type = WIFI_CONN_CMD_RETRY_NOW;
    (void)xQueueSend(s_ctx.cmd_queue, &cmd, 0U);
}

bool wifi_connection_service_get_snapshot(wifi_connection_snapshot_t *out_snapshot)
{
    if((out_snapshot == NULL) || (!s_ctx.initialized) || (s_ctx.lock == NULL)) {
        return false;
    }

    if(pdTRUE != xSemaphoreTake(s_ctx.lock, portMAX_DELAY)) {
        return false;
    }

    (void)memset(out_snapshot, 0, sizeof(*out_snapshot));
    out_snapshot->initialized = s_ctx.initialized;
    out_snapshot->state = s_ctx.state;
    out_snapshot->connected = (s_ctx.state == WIFI_CONN_STATE_CONNECTED);
    out_snapshot->reconnect_enabled = s_ctx.reconnect_enabled;
    out_snapshot->retry_pending = s_ctx.retry_pending;
    out_snapshot->have_profile = s_ctx.have_profile;
    out_snapshot->internet_ok = s_ctx.internet_ok;
    out_snapshot->retry_stage = s_ctx.retry_stage;
    out_snapshot->ping_fail_streak = s_ctx.ping_fail_streak;
    out_snapshot->retry_wait_ms = s_ctx.retry_wait_ms;
    out_snapshot->disconnect_reason = s_ctx.disconnect_reason;
    out_snapshot->last_rslt = s_ctx.last_rslt;
    out_snapshot->last_ping_rslt = s_ctx.last_ping_rslt;
    out_snapshot->rssi = s_ctx.rssi;

    (void)strncpy(out_snapshot->ssid, s_ctx.ssid, sizeof(out_snapshot->ssid) - 1U);
    (void)strncpy(out_snapshot->security, s_ctx.security, sizeof(out_snapshot->security) - 1U);
    (void)strncpy(out_snapshot->ip_addr, s_ctx.ip_addr, sizeof(out_snapshot->ip_addr) - 1U);

    (void)xSemaphoreGive(s_ctx.lock);
    return true;
}


