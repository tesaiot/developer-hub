/*
 * TESAIoT Platform - OTA HTTPS Client Implementation
 *
 * Copyright (c) 2026 TESAIoT Platform (TESA)
 * Licensed under the Apache License, Version 2.0
 *
 * This file implements the OTA client for embedded devices.
 * Uses libcurl for HTTPS requests. Replace with platform-specific
 * HTTP client for bare-metal or RTOS environments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "ota_client.h"

/* =============================================================================
 * Internal Helpers
 * ============================================================================= */

/* Buffer for HTTP response */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} response_buffer_t;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    response_buffer_t *buf = (response_buffer_t *)userp;

    /* Grow buffer if needed */
    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->size + realsize + 1) {
            new_cap = buf->size + realsize + 1024;
        }
        char *new_data = realloc(buf->data, new_cap);
        if (new_data == NULL) {
            return 0;
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';

    return realsize;
}

/* Simple JSON string parser */
static int parse_json_string(const char *json, const char *key, char *out, size_t out_sz)
{
    char search_key[128];
    const char *start, *end;
    size_t len;

    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    start = strstr(json, search_key);
    if (!start) return -1;

    /* Find the value after ": */
    start = strchr(start, ':');
    if (!start) return -1;
    start++;

    /* Skip whitespace */
    while (*start == ' ' || *start == '\t') start++;

    if (*start == '"') {
        /* String value */
        start++;
        end = strchr(start, '"');
        if (!end) return -1;
    } else {
        /* Non-string value */
        end = start;
        while (*end && *end != ',' && *end != '}' && *end != ' ') end++;
    }

    len = end - start;
    if (len >= out_sz) len = out_sz - 1;
    memcpy(out, start, len);
    out[len] = '\0';

    return 0;
}

static int parse_json_int(const char *json, const char *key)
{
    char buf[32];
    if (parse_json_string(json, key, buf, sizeof(buf)) == 0) {
        return atoi(buf);
    }
    return 0;
}

/* =============================================================================
 * API Implementation
 * ============================================================================= */

ota_error_t ota_init(ota_client_t *ctx,
                     const char *device_id,
                     const char *version,
                     const char *platform)
{
    if (!ctx || !device_id || !version || !platform) {
        return OTA_ERR_PARSE;
    }

    memset(ctx, 0, sizeof(ota_client_t));

    strncpy(ctx->device_id, device_id, sizeof(ctx->device_id) - 1);
    strncpy(ctx->current_version, version, sizeof(ctx->current_version) - 1);
    strncpy(ctx->platform, platform, sizeof(ctx->platform) - 1);

    /* Default server configuration */
    strncpy(ctx->server_host, OTA_SERVER_HOST, sizeof(ctx->server_host) - 1);
    ctx->server_port = OTA_SERVER_PORT;
    ctx->use_tls = OTA_USE_TLS;
    ctx->use_mtls = OTA_USE_MTLS;

    ctx->state = OTA_STATE_IDLE;
    ctx->last_error = OTA_OK;
    ctx->has_pending_job = 0;

    /* Initialize libcurl */
    curl_global_init(CURL_GLOBAL_ALL);

    OTA_LOG("Initialized OTA client");
    OTA_LOG("  Device ID: %s", ctx->device_id);
    OTA_LOG("  Version: %s", ctx->current_version);
    OTA_LOG("  Platform: %s", ctx->platform);

    return OTA_OK;
}

ota_error_t ota_set_server(ota_client_t *ctx,
                           const char *host,
                           int port,
                           int use_tls)
{
    if (!ctx || !host) return OTA_ERR_PARSE;

    strncpy(ctx->server_host, host, sizeof(ctx->server_host) - 1);
    ctx->server_port = port;
    ctx->use_tls = use_tls;

    OTA_LOG("Server: %s://%s:%d",
            use_tls ? "https" : "http", host, port);

    return OTA_OK;
}

ota_error_t ota_set_auth_token(ota_client_t *ctx, const char *token)
{
    if (!ctx || !token) return OTA_ERR_PARSE;

    strncpy(ctx->auth_token, token, sizeof(ctx->auth_token) - 1);
    ctx->use_mtls = 0;

    OTA_LOG("Auth token configured");

    return OTA_OK;
}

ota_error_t ota_set_mtls(ota_client_t *ctx,
                         const char *ca_cert,
                         const char *client_cert,
                         const char *client_key)
{
    if (!ctx) return OTA_ERR_PARSE;

    if (ca_cert) {
        strncpy(ctx->ca_cert_path, ca_cert, sizeof(ctx->ca_cert_path) - 1);
    }
    if (client_cert) {
        strncpy(ctx->client_cert_path, client_cert, sizeof(ctx->client_cert_path) - 1);
    }
    if (client_key) {
        strncpy(ctx->client_key_path, client_key, sizeof(ctx->client_key_path) - 1);
    }

    ctx->use_mtls = (client_cert && client_key) ? 1 : 0;

    OTA_LOG("mTLS %s", ctx->use_mtls ? "enabled" : "disabled");

    return OTA_OK;
}

ota_error_t ota_check_for_update(ota_client_t *ctx)
{
    CURL *curl;
    CURLcode res;
    char url[MAX_URL_SIZE];
    char endpoint[128];
    struct curl_slist *headers = NULL;
    response_buffer_t response = {0};
    long http_code = 0;

    if (!ctx) return OTA_ERR_PARSE;

    ctx->state = OTA_STATE_CHECKING;
    ctx->has_pending_job = 0;

    /* Build URL */
    snprintf(endpoint, sizeof(endpoint), OTA_JOB_ENDPOINT, ctx->device_id);
    snprintf(url, sizeof(url), "%s://%s:%d%s",
             ctx->use_tls ? "https" : "http",
             ctx->server_host, ctx->server_port, endpoint);

    OTA_LOG("Checking for update: %s", url);

    /* Initialize response buffer */
    response.capacity = 4096;
    response.data = malloc(response.capacity);
    if (!response.data) {
        ctx->last_error = OTA_ERR_MEMORY;
        ctx->state = OTA_STATE_IDLE;
        return OTA_ERR_MEMORY;
    }
    response.size = 0;

    curl = curl_easy_init();
    if (!curl) {
        free(response.data);
        ctx->last_error = OTA_ERR_NETWORK;
        ctx->state = OTA_STATE_IDLE;
        return OTA_ERR_NETWORK;
    }

    /* Build request headers - REQUIRED by TESAIoT OTA API */
    char hdr_version[64], hdr_platform[64], hdr_auth[600];

    snprintf(hdr_version, sizeof(hdr_version), "%s: %s",
             HDR_DEVICE_VERSION, ctx->current_version);
    snprintf(hdr_platform, sizeof(hdr_platform), "%s: %s",
             HDR_HARDWARE_TYPE, ctx->platform);

    headers = curl_slist_append(headers, hdr_version);
    headers = curl_slist_append(headers, hdr_platform);
    headers = curl_slist_append(headers, "Accept: application/json");

    if (ctx->auth_token[0] != '\0') {
        snprintf(hdr_auth, sizeof(hdr_auth), "Authorization: Bearer %s", ctx->auth_token);
        headers = curl_slist_append(headers, hdr_auth);
    }

    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TOTAL_TIMEOUT_SEC);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTP_CONNECT_TIMEOUT_SEC);

    if (ctx->use_tls) {
        if (ctx->ca_cert_path[0] != '\0') {
            curl_easy_setopt(curl, CURLOPT_CAINFO, ctx->ca_cert_path);
        } else {
            /* For testing - disable cert verification */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        if (ctx->use_mtls && ctx->client_cert_path[0] != '\0') {
            curl_easy_setopt(curl, CURLOPT_SSLCERT, ctx->client_cert_path);
            curl_easy_setopt(curl, CURLOPT_SSLKEY, ctx->client_key_path);
        }
    }

    /* Perform request */
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        OTA_ERR("Request failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response.data);
        ctx->last_error = OTA_ERR_NETWORK;
        ctx->state = OTA_STATE_IDLE;
        return OTA_ERR_NETWORK;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    OTA_LOG("Response: HTTP %ld (%zu bytes)", http_code, response.size);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    /* Parse response */
    if (http_code == 200) {
        /* Update available - parse job document */
        OTA_LOG("Update available! Parsing job document...");

        parse_json_string(response.data, "firmware_id",
                         ctx->job.firmware_id, sizeof(ctx->job.firmware_id));
        parse_json_string(response.data, "version",
                         ctx->job.version, sizeof(ctx->job.version));
        parse_json_string(response.data, "device_type",
                         ctx->job.device_type, sizeof(ctx->job.device_type));
        parse_json_string(response.data, "download_url",
                         ctx->job.download_url, sizeof(ctx->job.download_url));
        parse_json_string(response.data, "file_hash",
                         ctx->job.file_hash, sizeof(ctx->job.file_hash));
        parse_json_string(response.data, "signature",
                         ctx->job.signature, sizeof(ctx->job.signature));
        ctx->job.file_size = parse_json_int(response.data, "file_size");

        ctx->has_pending_job = 1;

        OTA_LOG("  Firmware ID: %s", ctx->job.firmware_id);
        OTA_LOG("  Version: %s", ctx->job.version);
        OTA_LOG("  Size: %u bytes", ctx->job.file_size);

        free(response.data);
        ctx->state = OTA_STATE_IDLE;
        ctx->last_error = OTA_OK;
        return OTA_OK;

    } else if (http_code == 204) {
        /* No update available */
        OTA_LOG("No update available (HTTP 204)");
        free(response.data);
        ctx->state = OTA_STATE_IDLE;
        ctx->last_error = OTA_NO_UPDATE;
        return OTA_NO_UPDATE;

    } else if (http_code == 401 || http_code == 403) {
        OTA_ERR("Authentication failed (HTTP %ld)", http_code);
        free(response.data);
        ctx->state = OTA_STATE_IDLE;
        ctx->last_error = OTA_ERR_AUTH;
        return OTA_ERR_AUTH;

    } else if (http_code == 404) {
        OTA_ERR("Device or endpoint not found (HTTP 404)");
        free(response.data);
        ctx->state = OTA_STATE_IDLE;
        ctx->last_error = OTA_ERR_NOT_FOUND;
        return OTA_ERR_NOT_FOUND;

    } else {
        OTA_ERR("Unexpected response: HTTP %ld", http_code);
        OTA_ERR("Response: %s", response.data);
        free(response.data);
        ctx->state = OTA_STATE_IDLE;
        ctx->last_error = OTA_ERR_NETWORK;
        return OTA_ERR_NETWORK;
    }
}

ota_error_t ota_download_firmware(ota_client_t *ctx)
{
    /* Placeholder - implement firmware download with chunk handling */
    if (!ctx || !ctx->has_pending_job) {
        return OTA_ERR_PARSE;
    }

    ctx->state = OTA_STATE_DOWNLOADING;
    OTA_LOG("Downloading firmware from: %s", ctx->job.download_url);

    /* TODO: Implement chunked download with progress callback */
    /* For PSoC Edge: Use Infineon chunk format (32-byte header + 4KB data) */

    ctx->state = OTA_STATE_IDLE;
    return OTA_OK;
}

ota_error_t ota_verify_firmware(ota_client_t *ctx)
{
    if (!ctx) return OTA_ERR_PARSE;

    ctx->state = OTA_STATE_VERIFYING;
    OTA_LOG("Verifying firmware integrity...");

    /* TODO: Calculate SHA256 hash and compare with ctx->job.file_hash */

    ctx->state = OTA_STATE_IDLE;
    return OTA_OK;
}

ota_error_t ota_report_status(ota_client_t *ctx,
                              const char *status,
                              const char *error_msg)
{
    if (!ctx || !status) return OTA_ERR_PARSE;

    OTA_LOG("Reporting status: %s%s%s",
            status,
            error_msg ? " - " : "",
            error_msg ? error_msg : "");

    /* TODO: POST to status endpoint */

    return OTA_OK;
}

ota_error_t ota_run_update_cycle(ota_client_t *ctx)
{
    ota_error_t err;

    err = ota_check_for_update(ctx);
    if (err == OTA_NO_UPDATE) {
        return OTA_NO_UPDATE;
    }
    if (err != OTA_OK) {
        ota_report_status(ctx, "failed", ota_error_str(err));
        return err;
    }

    err = ota_download_firmware(ctx);
    if (err != OTA_OK) {
        ota_report_status(ctx, "failed", "Download failed");
        return err;
    }

    err = ota_verify_firmware(ctx);
    if (err != OTA_OK) {
        ota_report_status(ctx, "failed", "Verification failed");
        return err;
    }

    /* Apply firmware if callback provided */
    if (ctx->apply_firmware) {
        ctx->state = OTA_STATE_APPLYING;
        if (ctx->apply_firmware() != 0) {
            ota_report_status(ctx, "failed", "Apply failed");
            return OTA_ERR_FLASH;
        }
    }

    ota_report_status(ctx, "success", NULL);
    ctx->state = OTA_STATE_COMPLETE;

    return OTA_OK;
}

const char *ota_error_str(ota_error_t error)
{
    switch (error) {
        case OTA_OK:            return "OK";
        case OTA_NO_UPDATE:     return "No update available";
        case OTA_ERR_NETWORK:   return "Network error";
        case OTA_ERR_AUTH:      return "Authentication failed";
        case OTA_ERR_NOT_FOUND: return "Not found";
        case OTA_ERR_CHECKSUM:  return "Checksum mismatch";
        case OTA_ERR_FLASH:     return "Flash write error";
        case OTA_ERR_VERSION:   return "Version error";
        case OTA_ERR_TIMEOUT:   return "Timeout";
        case OTA_ERR_MEMORY:    return "Memory error";
        case OTA_ERR_PARSE:     return "Parse error";
        default:                return "Unknown error";
    }
}

const char *ota_state_str(ota_state_t state)
{
    switch (state) {
        case OTA_STATE_IDLE:        return "Idle";
        case OTA_STATE_CHECKING:    return "Checking for update";
        case OTA_STATE_DOWNLOADING: return "Downloading";
        case OTA_STATE_VERIFYING:   return "Verifying";
        case OTA_STATE_APPLYING:    return "Applying";
        case OTA_STATE_REBOOTING:   return "Rebooting";
        case OTA_STATE_COMPLETE:    return "Complete";
        case OTA_STATE_FAILED:      return "Failed";
        default:                    return "Unknown";
    }
}
