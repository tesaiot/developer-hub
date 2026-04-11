#include "wifi_scan_service.h"

#include "FreeRTOS.h"
#include "cy_sd_host.h"
#include "cy_sysint.h"
#include "cy_wcm.h"
#include "cybsp.h"
#include "cycfg_peripherals.h"
#include "mtb_hal_gpio.h"
#include "mtb_hal_sdio.h"
#include "semphr.h"

#include <stdio.h>
#include <string.h>

#define WIFI_SCAN_LOG_ENABLE               (1)
#define WIFI_SCAN_HIDDEN_SSID_TEXT         "<hidden>"
#define APP_SDIO_INTERRUPT_PRIORITY        (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY   (2U)
#define APP_SDIO_FREQUENCY_HZ              (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK            (64U)

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
#include "cy_syspm.h"
#endif

static mtb_hal_sdio_t s_sdio_instance;
static cy_stc_sd_host_context_t s_sdhc_host_context;
static cy_wcm_config_t s_wcm_config;

static SemaphoreHandle_t s_wifi_init_mutex;
static bool s_sdio_inited;
static bool s_wcm_inited;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
static cy_stc_syspm_callback_params_t sdhc_ds_params = {
    .context = &s_sdhc_host_context,
    .base = CYBSP_WIFI_SDIO_HW,
};
static cy_stc_syspm_callback_t sdhc_deep_sleep_cb = {
    .callback = Cy_SD_Host_DeepSleepCallback,
    .skipMode = 0U,
    .type = CY_SYSPM_DEEPSLEEP,
    .callbackParams = &sdhc_ds_params,
    .prevItm = NULL,
    .nextItm = NULL,
    .order = 1U,
};
#endif

static void wifi_scan_log(const char *msg)
{
#if WIFI_SCAN_LOG_ENABLE
    printf("[WIFI_LIST] %s\r\n", msg);
#else
    CY_UNUSED_PARAMETER(msg);
#endif
}

static void wifi_scan_log_rslt(const char *tag, cy_rslt_t rslt)
{
#if WIFI_SCAN_LOG_ENABLE
    printf("[WIFI_LIST] %s rslt=0x%08lx\r\n", tag, (unsigned long)rslt);
#else
    CY_UNUSED_PARAMETER(tag);
    CY_UNUSED_PARAMETER(rslt);
#endif
}

static const char *wifi_scan_security_to_text(cy_wcm_security_t security)
{
    switch(security) {
        case CY_WCM_SECURITY_OPEN:
            return "OPEN";
        case CY_WCM_SECURITY_WEP_PSK:
            return "WEP-PSK";
        case CY_WCM_SECURITY_WEP_SHARED:
            return "WEP-SHARED";
        case CY_WCM_SECURITY_WPA_TKIP_PSK:
            return "WPA-TKIP-PSK";
        case CY_WCM_SECURITY_WPA_AES_PSK:
            return "WPA-AES-PSK";
        case CY_WCM_SECURITY_WPA_MIXED_PSK:
            return "WPA-MIXED-PSK";
        case CY_WCM_SECURITY_WPA2_AES_PSK:
            return "WPA2-AES-PSK";
        case CY_WCM_SECURITY_WPA2_TKIP_PSK:
            return "WPA2-TKIP-PSK";
        case CY_WCM_SECURITY_WPA2_MIXED_PSK:
            return "WPA2-MIXED-PSK";
        case CY_WCM_SECURITY_WPA2_FBT_PSK:
            return "WPA2-FBT-PSK";
        case CY_WCM_SECURITY_WPA3_SAE:
            return "WPA3-SAE";
        case CY_WCM_SECURITY_WPA2_WPA_AES_PSK:
            return "WPA2-WPA-AES-PSK";
        case CY_WCM_SECURITY_WPA2_WPA_MIXED_PSK:
            return "WPA2-WPA-MIXED-PSK";
        case CY_WCM_SECURITY_WPA3_WPA2_PSK:
            return "WPA3-WPA2-PSK";
        case CY_WCM_SECURITY_WPA_TKIP_ENT:
            return "WPA-TKIP-ENT";
        case CY_WCM_SECURITY_WPA_AES_ENT:
            return "WPA-AES-ENT";
        case CY_WCM_SECURITY_WPA_MIXED_ENT:
            return "WPA-MIXED-ENT";
        case CY_WCM_SECURITY_WPA2_TKIP_ENT:
            return "WPA2-TKIP-ENT";
        case CY_WCM_SECURITY_WPA2_AES_ENT:
            return "WPA2-AES-ENT";
        case CY_WCM_SECURITY_WPA2_MIXED_ENT:
            return "WPA2-MIXED-ENT";
        case CY_WCM_SECURITY_WPA2_FBT_ENT:
            return "WPA2-FBT-ENT";
        case CY_WCM_SECURITY_IBSS_OPEN:
            return "IBSS-OPEN";
        case CY_WCM_SECURITY_WPS_SECURE:
            return "WPS-SECURE";
        default:
            return "UNKNOWN";
    }
}

static bool wifi_scan_is_ssid_printable(const uint8_t *ssid)
{
    size_t len = strnlen((const char *)ssid, CY_WCM_MAX_SSID_LEN);

    for(size_t i = 0U; i < len; i++) {
        if((ssid[i] < 0x20U) || (ssid[i] > 0x7EU)) {
            return false;
        }
    }

    return true;
}

static void wifi_scan_sort_by_rssi_desc(wifi_scan_service_t *service)
{
    for(uint16_t i = 1U; i < service->ap_count; i++) {
        wifi_scan_ap_t key = service->aps[i];
        int32_t j = (int32_t)i - 1;

        while((j >= 0) && (service->aps[j].rssi < key.rssi)) {
            service->aps[j + 1] = service->aps[j];
            j--;
        }

        service->aps[j + 1] = key;
    }
}

static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&s_sdio_instance);
}

static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&s_wcm_config.wifi_host_wake_pin);
}

static cy_rslt_t wifi_scan_sdio_init(void)
{
    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;

    cy_stc_sysint_t sdio_intr_cfg = {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY,
    };
    cy_stc_sysint_t host_wake_intr_cfg = {
        .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
        .intrPriority = APP_HOST_WAKE_INTERRUPT_PRIORITY,
    };

    cy_en_sysint_status_t int_status = Cy_SysInt_Init(&sdio_intr_cfg, sdio_interrupt_handler);
    if(CY_SYSINT_SUCCESS != int_status) {
        return CY_RSLT_TYPE_ERROR;
    }

    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    result = mtb_hal_sdio_setup(&s_sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config, NULL, &s_sdhc_host_context);
    if(CY_RSLT_SUCCESS != result) {
        return result;
    }

    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW, CYBSP_WIFI_SDIO_sdio_hal_config.host_config, &s_sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    sdio_hal_cfg.frequencyhal_hz = APP_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = SDHC_SDIO_64BYTES_BLOCK;
    mtb_hal_sdio_configure(&s_sdio_instance, &sdio_hal_cfg);

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
    Cy_SysPm_RegisterCallback(&sdhc_deep_sleep_cb);
#endif

    mtb_hal_gpio_setup(&s_wcm_config.wifi_wl_pin, CYBSP_WIFI_WL_REG_ON_PORT_NUM, CYBSP_WIFI_WL_REG_ON_PIN);
    mtb_hal_gpio_setup(&s_wcm_config.wifi_host_wake_pin, CYBSP_WIFI_HOST_WAKE_PORT_NUM, CYBSP_WIFI_HOST_WAKE_PIN);

    int_status = Cy_SysInt_Init(&host_wake_intr_cfg, host_wake_interrupt_handler);
    if(CY_SYSINT_SUCCESS != int_status) {
        return CY_RSLT_TYPE_ERROR;
    }

    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);

    return CY_RSLT_SUCCESS;
}

static cy_rslt_t wifi_scan_radio_init_once(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if(NULL == s_wifi_init_mutex) {
        s_wifi_init_mutex = xSemaphoreCreateMutex();
        if(NULL == s_wifi_init_mutex) {
            return CY_RSLT_TYPE_ERROR;
        }
    }

    if(pdTRUE != xSemaphoreTake(s_wifi_init_mutex, portMAX_DELAY)) {
        return CY_RSLT_TYPE_ERROR;
    }

    if(!s_sdio_inited) {
        result = wifi_scan_sdio_init();
        if(CY_RSLT_SUCCESS != result) {
            (void)xSemaphoreGive(s_wifi_init_mutex);
            return result;
        }
        s_sdio_inited = true;
    }

    if(!s_wcm_inited) {
        s_wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;
        s_wcm_config.wifi_interface_instance = &s_sdio_instance;

        result = cy_wcm_init(&s_wcm_config);
        if(CY_RSLT_SUCCESS != result) {
            (void)xSemaphoreGive(s_wifi_init_mutex);
            return result;
        }

        s_wcm_inited = true;
    }

    (void)xSemaphoreGive(s_wifi_init_mutex);
    return result;
}

static void wifi_scan_callback(cy_wcm_scan_result_t *result_ptr, void *user_data, cy_wcm_scan_status_t status)
{
    wifi_scan_service_t *service = (wifi_scan_service_t *)user_data;

    if(service == NULL) {
        return;
    }

    if((status == CY_WCM_SCAN_INCOMPLETE) && (result_ptr != NULL)) {
        if((service->ap_count < WIFI_SCAN_MAX_APS) && wifi_scan_is_ssid_printable(result_ptr->SSID)) {
            wifi_scan_ap_t *entry;
            size_t ssid_len;

            taskENTER_CRITICAL();
            entry = &service->aps[service->ap_count];
            (void)memset(entry, 0, sizeof(*entry));

            ssid_len = strnlen((const char *)result_ptr->SSID, CY_WCM_MAX_SSID_LEN);
            if(ssid_len > 0U) {
                (void)memcpy(entry->ssid, result_ptr->SSID, ssid_len);
                entry->ssid[ssid_len] = '\0';
            } else {
                (void)snprintf(entry->ssid, sizeof(entry->ssid), "%s", WIFI_SCAN_HIDDEN_SSID_TEXT);
            }

            entry->rssi = result_ptr->signal_strength;
            (void)snprintf(entry->security,
                           sizeof(entry->security),
                           "%s",
                           wifi_scan_security_to_text(result_ptr->security));

            service->ap_count++;
            taskEXIT_CRITICAL();
        }

        return;
    }

    if(status == CY_WCM_SCAN_COMPLETE) {
        taskENTER_CRITICAL();
        wifi_scan_sort_by_rssi_desc(service);
        service->scan_done_pending = true;
        taskEXIT_CRITICAL();
    }
}

void wifi_scan_service_init(wifi_scan_service_t *service)
{
    if(service == NULL) {
        return;
    }

    (void)memset(service, 0, sizeof(*service));
    service->radio_ready = s_wcm_inited;
    wifi_scan_log("INIT");
}

cy_rslt_t wifi_scan_service_preinit(void)
{
    bool already_ready = s_wcm_inited;
    cy_rslt_t result = wifi_scan_radio_init_once();

    /* Log RADIO_READY only on the transition to ready state. */
    if(CY_RSLT_SUCCESS == result) {
        if(!already_ready) {
            wifi_scan_log("RADIO_READY");
        }
    } else {
        wifi_scan_log_rslt("RADIO_INIT_FAIL", result);
    }

    return result;
}
bool wifi_scan_service_start(wifi_scan_service_t *service)
{
    cy_rslt_t result;

    if(service == NULL) {
        return false;
    }

    if(service->scanning) {
        return false;
    }

    if(!service->radio_ready) {
        result = wifi_scan_service_preinit();
        if(CY_RSLT_SUCCESS != result) {
            service->last_rslt = (uint32_t)result;
            return false;
        }

        service->radio_ready = true;
    }

    taskENTER_CRITICAL();
    service->ap_count = 0U;
    service->scan_done_pending = false;
    service->scan_error_pending = false;
    service->scanning = true;
    taskEXIT_CRITICAL();

    result = cy_wcm_start_scan(wifi_scan_callback, service, NULL);
    if(CY_RSLT_SUCCESS != result) {
        taskENTER_CRITICAL();
        service->scanning = false;
        service->scan_error_pending = true;
        service->last_rslt = (uint32_t)result;
        taskEXIT_CRITICAL();

        wifi_scan_log_rslt("SCAN_START_FAIL", result);
        return false;
    }

    wifi_scan_log("SCAN_START");
    return true;
}

bool wifi_scan_service_process(wifi_scan_service_t *service)
{
    bool done;
    bool error;

    if(service == NULL) {
        return false;
    }

    taskENTER_CRITICAL();
    done = service->scan_done_pending;
    error = service->scan_error_pending;
    service->scan_done_pending = false;
    service->scan_error_pending = false;
    taskEXIT_CRITICAL();

    if(done) {
        service->scanning = false;
        service->scan_sequence++;

#if WIFI_SCAN_LOG_ENABLE
        printf("[WIFI_LIST] SCAN_DONE count=%u seq=%u\r\n",
               (unsigned int)service->ap_count,
               (unsigned int)service->scan_sequence);
#endif

        return true;
    }

    if(error) {
        service->scanning = false;
        wifi_scan_log_rslt("SCAN_ERROR", (cy_rslt_t)service->last_rslt);
        return true;
    }

    return false;
}

const wifi_scan_ap_t *wifi_scan_service_get_list(const wifi_scan_service_t *service, uint16_t *count)
{
    if(count != NULL) {
        *count = (service != NULL) ? service->ap_count : 0U;
    }

    return (service != NULL) ? service->aps : NULL;
}





