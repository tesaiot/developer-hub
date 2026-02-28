/*
 * TESAIoT Platform - OTA HTTPS Client API
 *
 * Copyright (c) 2026 TESAIoT Platform (TESA)
 * Licensed under the Apache License, Version 2.0
 *
 * This header defines the OTA client API for embedded devices.
 * Implements HTTPS-based polling mode for firmware updates.
 */

#ifndef TESAIOT_OTA_CLIENT_H
#define TESAIOT_OTA_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include "ota_config.h"

/* =============================================================================
 * OTA Job Document Structure
 * ============================================================================= */

typedef struct {
    char firmware_id[64];
    char version[32];
    char device_type[32];
    char download_url[256];
    char file_hash[128];
    char signature[512];
    uint32_t file_size;
    int is_critical;
    char min_version[32];
    char release_notes[256];
    char created_at[32];
    char expires_at[32];
} ota_job_document_t;

/* =============================================================================
 * OTA Client Context
 * ============================================================================= */

typedef struct {
    /* Device identity */
    char device_id[64];
    char current_version[32];
    char platform[32];

    /* Server configuration */
    char server_host[128];
    int server_port;
    int use_tls;
    int use_mtls;

    /* Authentication */
    char auth_token[512];
    char ca_cert_path[256];
    char client_cert_path[256];
    char client_key_path[256];

    /* State tracking */
    ota_state_t state;
    ota_error_t last_error;

    /* Current job (if any) */
    ota_job_document_t job;
    int has_pending_job;

    /* Download progress */
    uint32_t bytes_downloaded;
    uint32_t total_bytes;

    /* Callbacks */
    void (*on_progress)(uint32_t downloaded, uint32_t total);
    void (*on_complete)(ota_error_t result);
    int (*write_chunk)(const uint8_t *data, size_t len, uint32_t offset);
    int (*verify_firmware)(void);
    int (*apply_firmware)(void);

} ota_client_t;

/* =============================================================================
 * OTA Client API Functions
 * ============================================================================= */

/**
 * Initialize OTA client with default configuration
 *
 * @param ctx       Pointer to OTA client context
 * @param device_id Device UUID (from Trust M or generated)
 * @param version   Current firmware version string
 * @param platform  Platform identifier (e.g., "PSE84", "ESP32")
 * @return          OTA_OK on success, error code on failure
 */
ota_error_t ota_init(ota_client_t *ctx,
                     const char *device_id,
                     const char *version,
                     const char *platform);

/**
 * Configure server connection
 *
 * @param ctx       Pointer to OTA client context
 * @param host      Server hostname (e.g., "admin.tesaiot.com")
 * @param port      Server port (443 for HTTPS)
 * @param use_tls   Enable TLS (1) or disable (0)
 * @return          OTA_OK on success
 */
ota_error_t ota_set_server(ota_client_t *ctx,
                           const char *host,
                           int port,
                           int use_tls);

/**
 * Configure authentication token (for server-TLS mode)
 *
 * @param ctx       Pointer to OTA client context
 * @param token     JWT authentication token
 * @return          OTA_OK on success
 */
ota_error_t ota_set_auth_token(ota_client_t *ctx, const char *token);

/**
 * Configure mTLS certificates (for mutual TLS mode)
 *
 * @param ctx           Pointer to OTA client context
 * @param ca_cert       Path to CA certificate chain
 * @param client_cert   Path to client certificate
 * @param client_key    Path to client private key
 * @return              OTA_OK on success
 */
ota_error_t ota_set_mtls(ota_client_t *ctx,
                         const char *ca_cert,
                         const char *client_cert,
                         const char *client_key);

/**
 * Check for available firmware updates (polling mode)
 *
 * Sends request to /api/v1/ota/devices/{device_id}/job endpoint.
 * If update available (HTTP 200), job document is stored in ctx->job.
 * If no update (HTTP 204), returns OTA_NO_UPDATE.
 *
 * @param ctx       Pointer to OTA client context
 * @return          OTA_OK if update available, OTA_NO_UPDATE if none, error otherwise
 */
ota_error_t ota_check_for_update(ota_client_t *ctx);

/**
 * Download firmware from job document URL
 *
 * Downloads firmware in chunks, calling write_chunk callback for each.
 * Progress callback called after each chunk.
 *
 * @param ctx       Pointer to OTA client context
 * @return          OTA_OK on success, error code on failure
 */
ota_error_t ota_download_firmware(ota_client_t *ctx);

/**
 * Verify downloaded firmware integrity
 *
 * Checks SHA256 hash against job document file_hash.
 * Optionally verifies signature if signing enabled.
 *
 * @param ctx       Pointer to OTA client context
 * @return          OTA_OK if valid, OTA_ERR_CHECKSUM if invalid
 */
ota_error_t ota_verify_firmware(ota_client_t *ctx);

/**
 * Report update status to platform
 *
 * @param ctx       Pointer to OTA client context
 * @param status    Status string ("success", "failed", "in_progress")
 * @param error_msg Optional error message (NULL if none)
 * @return          OTA_OK on success
 */
ota_error_t ota_report_status(ota_client_t *ctx,
                              const char *status,
                              const char *error_msg);

/**
 * Perform full OTA update cycle
 *
 * Combines check -> download -> verify -> apply in one call.
 *
 * @param ctx       Pointer to OTA client context
 * @return          OTA_OK on success, OTA_NO_UPDATE if none, error otherwise
 */
ota_error_t ota_run_update_cycle(ota_client_t *ctx);

/**
 * Get human-readable error string
 *
 * @param error     OTA error code
 * @return          Static string describing error
 */
const char *ota_error_str(ota_error_t error);

/**
 * Get human-readable state string
 *
 * @param state     OTA state
 * @return          Static string describing state
 */
const char *ota_state_str(ota_state_t state);

#endif /* TESAIOT_OTA_CLIENT_H */
