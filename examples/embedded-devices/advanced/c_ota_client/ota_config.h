/*
 * TESAIoT Platform - OTA HTTPS Client Configuration
 *
 * Copyright (c) 2026 TESAIoT Platform (TESA)
 * Licensed under the Apache License, Version 2.0
 *
 * This configuration file defines OTA client parameters for embedded devices.
 * Compatible with PSoC Edge (E84), ESP32, STM32, and other Cortex-M/A platforms.
 */

#ifndef TESAIOT_OTA_CONFIG_H
#define TESAIOT_OTA_CONFIG_H

#include <stdint.h>

/* =============================================================================
 * Device Identity Configuration
 * ============================================================================= */

/* Device ID - UUID format from Trust M secure element or generated */
#ifndef OTA_DEVICE_ID
#define OTA_DEVICE_ID           "95ad6ed........................."
#endif

/* Platform/Hardware type for compatibility check */
#ifndef OTA_PLATFORM
#define OTA_PLATFORM            "PSE84"
#endif

/* Current firmware version (will be compared against available updates) */
#ifndef OTA_CURRENT_VERSION
#define OTA_CURRENT_VERSION     "1.0.0"
#endif

/* =============================================================================
 * TESAIoT Platform Endpoints
 * ============================================================================= */

/* Production server */
#define OTA_SERVER_HOST         "admin.tesaiot.com"
#define OTA_SERVER_PORT         443
#define OTA_USE_TLS             1

/* API endpoints */
#define OTA_JOB_ENDPOINT        "/api/v1/ota/devices/%s/job"
#define OTA_FIRMWARE_ENDPOINT   "/api/v1/ota/firmware/%s/download"
#define OTA_STATUS_ENDPOINT     "/api/v1/ota/deployments/%s/status"

/* =============================================================================
 * TLS/mTLS Configuration
 * ============================================================================= */

/* Certificate paths (for mTLS authentication) */
#define PATH_CERTS_DIR          "certs/"
#define PATH_CA_CHAIN           PATH_CERTS_DIR "ca-chain.pem"
#define PATH_CLIENT_CERT        PATH_CERTS_DIR "client_cert.pem"
#define PATH_CLIENT_KEY         PATH_CERTS_DIR "client_key.pem"
#define PATH_AUTH_TOKEN         PATH_CERTS_DIR "auth_token.txt"

/* System CA bundle for server-TLS mode */
#define SYSTEM_CA_BUNDLE_PATH   "/etc/ssl/certs/ca-certificates.crt"

/* Enable mTLS (mutual TLS) or use server-TLS with JWT token */
#ifndef OTA_USE_MTLS
#define OTA_USE_MTLS            0
#endif

/* =============================================================================
 * HTTP Request Configuration
 * ============================================================================= */

#define HTTP_CONNECT_TIMEOUT_SEC    10
#define HTTP_TOTAL_TIMEOUT_SEC      60
#define HTTP_RETRY_COUNT            3
#define HTTP_RETRY_DELAY_SEC        5

/* Buffer sizes */
#define MAX_URL_SIZE                512
#define MAX_HEADER_SIZE             256
#define MAX_JSON_SIZE               4096
#define MAX_FIRMWARE_CHUNK_SIZE     4096

/* =============================================================================
 * OTA Protocol Headers (TESAIoT Standard)
 * ============================================================================= */

/* Request headers (Device -> Platform) */
#define HDR_DEVICE_ID           "X-Device-ID"
#define HDR_DEVICE_VERSION      "X-Device-Version"
#define HDR_DEVICE_PLATFORM     "X-Device-Platform"
#define HDR_HARDWARE_TYPE       "X-Hardware-Type"
#define HDR_AUTHORIZATION       "Authorization"
#define HDR_USER_AGENT          "User-Agent"

/* Response headers (Platform -> Device) */
#define HDR_UPDATE_CHECK        "X-Update-Check"
#define HDR_BUILD_FINGERPRINT   "X-Build-Fingerprint"
#define HDR_CONTENT_LENGTH      "Content-Length"
#define HDR_CONTENT_TYPE        "Content-Type"

/* =============================================================================
 * Infineon-Compatible Chunk Format (32-byte header + 4KB data)
 * ============================================================================= */

/* Magic bytes for Infineon OTA chunk validation */
#define INFINEON_HEADER_MAGIC   "OTAImage"
#define INFINEON_HEADER_SIZE    32
#define INFINEON_CHUNK_SIZE     4096

/* Chunk header structure (matches PSoC Secure OTA format) */
typedef struct __attribute__((packed)) {
    uint8_t  magic[8];          /* "OTAImage" */
    uint16_t offset_to_data;    /* Offset from header start to payload */
    uint16_t chunk_number;      /* Current chunk index (0-based) */
    uint16_t chunk_size;        /* Size of data in this chunk */
    uint16_t total_chunks;      /* Total number of chunks */
    uint32_t total_size;        /* Total firmware size in bytes */
    uint32_t crc32;             /* CRC32 of chunk data */
    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t reserved3;
} infineon_chunk_header_t;

/* =============================================================================
 * Debug Configuration
 * ============================================================================= */

#ifndef OTA_DEBUG
#define OTA_DEBUG               1
#endif

#if OTA_DEBUG
#include <stdio.h>
#define OTA_LOG(fmt, ...)       printf("[OTA] " fmt "\n", ##__VA_ARGS__)
#define OTA_ERR(fmt, ...)       printf("[OTA ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define OTA_LOG(fmt, ...)       do {} while(0)
#define OTA_ERR(fmt, ...)       do {} while(0)
#endif

/* =============================================================================
 * OTA State Machine
 * ============================================================================= */

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_APPLYING,
    OTA_STATE_REBOOTING,
    OTA_STATE_COMPLETE,
    OTA_STATE_FAILED
} ota_state_t;

/* =============================================================================
 * OTA Error Codes
 * ============================================================================= */

typedef enum {
    OTA_OK = 0,
    OTA_NO_UPDATE = 1,
    OTA_ERR_NETWORK = -1,
    OTA_ERR_AUTH = -2,
    OTA_ERR_NOT_FOUND = -3,
    OTA_ERR_CHECKSUM = -4,
    OTA_ERR_FLASH = -5,
    OTA_ERR_VERSION = -6,
    OTA_ERR_TIMEOUT = -7,
    OTA_ERR_MEMORY = -8,
    OTA_ERR_PARSE = -9
} ota_error_t;

#endif /* TESAIOT_OTA_CONFIG_H */
