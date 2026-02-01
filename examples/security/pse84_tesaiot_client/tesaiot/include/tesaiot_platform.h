/**
 * SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
 *
 * \author TESAIoT Platform Developer Team
 *
 * \file tesaiot_platform.h
 *
 * \brief TESAIoT Standard Libraries - Umbrella Header (Group 5)
 *
 * This umbrella header consolidates standard platform service headers:
 * - MQTT operations (connect, publish, subscribe)
 * - SNTP time synchronization
 *
 * Domain Group: Core TESAIoT's Standard Libraries (Communication services)
 *
 * Usage:
 * @code
 * #include "tesaiot_platform.h"
 *
 * // MQTT operations
 * tesaiot_mqtt_connect();
 * tesaiot_mqtt_is_connected();
 *
 * // Time sync
 * tesaiot_sntp_sync_time(NULL, NULL);
 * @endcode
 *
 * @note For CSR workflow, use tesaiot_csr.h
 * @note For Protected Update, use tesaiot_protected_update.h
 *
 * This file is part of the TESAIoT AIoT Foundation Platform.
 *
 * \ingroup TESAIoT
 *
 * ============================================================================
 * VERSION HISTORY:
 * v1.0 (2026-01-18): Initial creation as umbrella header for platform services.
 * v2.1 (2026-01-18): Reorganized - CSR and Protected Update moved to own umbrellas.
 * v2.2 (2026-01-18): Merged MQTT and SNTP APIs directly into this header.
 * ============================================================================
 */

#ifndef TESAIOT_PLATFORM_H_
#define TESAIOT_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/*******************************************************************************
 * MQTT Client API
 ******************************************************************************/

/**
 * Connect to MQTT broker
 *
 * This function checks if MQTT is already connected. If not, it will
 * request the MQTT task to start and connect using the currently selected
 * certificate (Factory or Device).
 *
 * @return true on success (connected or already connected), false on error
 */
bool tesaiot_mqtt_connect(void);

/**
 * Check if MQTT is currently connected
 *
 * @return true if connected, false otherwise
 */
bool tesaiot_mqtt_is_connected(void);

/*******************************************************************************
 * SNTP Client API
 ******************************************************************************/

/******************************************************************************
 * Configuration
 ******************************************************************************/

/**
 * Default NTP servers (pool.ntp.org - global server pool)
 * Format: "pool.ntp.org" or specific servers like:
 * - "time.google.com" (Google NTP)
 * - "time.cloudflare.com" (Cloudflare NTP)
 * - "time.nist.gov" (NIST NTP)
 */
#define TESAIOT_SNTP_DEFAULT_SERVER         "pool.ntp.org"
#define TESAIOT_SNTP_BACKUP_SERVER          "time.google.com"

/**
 * NTP port (standard UDP port 123)
 */
#define TESAIOT_SNTP_PORT                   123

/**
 * Timeout for NTP request/response (milliseconds)
 */
#define TESAIOT_SNTP_TIMEOUT_MS             5000

/**
 * Maximum retry attempts
 */
#define TESAIOT_SNTP_MAX_RETRIES            3

/**
 * NTP packet size (48 bytes as per RFC 4330)
 */
#define TESAIOT_SNTP_PACKET_SIZE            48

/**
 * Enable debug logging (set to 1 to enable)
 */
#ifndef TESAIOT_SNTP_DEBUG_ENABLED
#define TESAIOT_SNTP_DEBUG_ENABLED          1
#endif

/******************************************************************************
 * NTP Timestamp Utilities
 ******************************************************************************/

/**
 * NTP epoch starts at Jan 1, 1900
 * Unix epoch starts at Jan 1, 1970
 * Offset = 70 years = 2208988800 seconds
 */
#define TESAIOT_NTP_EPOCH_OFFSET            2208988800UL

/**
 * Minimum valid Unix timestamp (Jan 1, 2020 00:00:00 UTC)
 * Used to detect if system time is synced
 */
#define TESAIOT_MIN_VALID_TIMESTAMP         1577836800UL

/**
 * @brief NTP timestamp format (64-bit: 32-bit seconds + 32-bit fraction)
 */
typedef struct {
    uint32_t seconds;      /**< Seconds since NTP epoch (Jan 1, 1900) */
    uint32_t fraction;     /**< Fractional seconds (1/2^32 of a second) */
} tesaiot_ntp_timestamp_t;

/**
 * @brief SNTP packet structure (48 bytes as per RFC 4330)
 *
 * Packet format:
 * - LI (2 bits): Leap Indicator
 * - VN (3 bits): Version Number (4 for SNTPv4)
 * - Mode (3 bits): Mode (3 = client, 4 = server)
 * - Stratum (8 bits): Server stratum level
 * - Poll (8 bits): Maximum interval between successive messages
 * - Precision (8 bits): Precision of system clock
 * - Root Delay (32 bits): Total round-trip delay to reference clock
 * - Root Dispersion (32 bits): Maximum error relative to reference clock
 * - Reference Identifier (32 bits): Reference clock identifier
 * - Reference Timestamp (64 bits): Time reference clock was last set
 * - Originate Timestamp (64 bits): Time client sent request
 * - Receive Timestamp (64 bits): Time server received request
 * - Transmit Timestamp (64 bits): Time server sent reply
 */
typedef struct __attribute__((packed)) {
    uint8_t  li_vn_mode;                    /**< LI (2b) + VN (3b) + Mode (3b) */
    uint8_t  stratum;                       /**< Stratum level of server */
    uint8_t  poll;                          /**< Poll interval */
    int8_t   precision;                     /**< Precision of clock */
    uint32_t root_delay;                    /**< Root delay */
    uint32_t root_dispersion;               /**< Root dispersion */
    uint32_t reference_id;                  /**< Reference clock identifier */
    tesaiot_ntp_timestamp_t ref_timestamp;  /**< Reference timestamp */
    tesaiot_ntp_timestamp_t orig_timestamp; /**< Originate timestamp (T1) */
    tesaiot_ntp_timestamp_t recv_timestamp; /**< Receive timestamp (T2) */
    tesaiot_ntp_timestamp_t xmit_timestamp; /**< Transmit timestamp (T3) */
} tesaiot_sntp_packet_t;

/**
 * @brief SNTP result structure
 */
typedef struct {
    bool     success;              /**< True if time sync succeeded */
    time_t   unix_time;            /**< Unix timestamp (seconds since Jan 1, 1970) */
    uint32_t round_trip_delay_ms; /**< Round-trip delay in milliseconds */
    uint8_t  stratum;              /**< NTP server stratum (0=invalid, 1=primary, 2-15=secondary) */
    char     server[64];           /**< NTP server hostname/IP used */
} tesaiot_sntp_result_t;

/******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * @brief Synchronize system time with NTP server
 *
 * Sends SNTP request to NTP server and updates system time if successful.
 * Automatically retries with backup server if primary server fails.
 *
 * Workflow:
 * 1. Resolve NTP server hostname to IP address (DNS query)
 * 2. Create UDP socket
 * 3. Send SNTP request packet (48 bytes)
 * 4. Wait for SNTP response (timeout: TESAIOT_SNTP_TIMEOUT_MS)
 * 5. Parse response and extract transmit timestamp
 * 6. Convert NTP timestamp to Unix timestamp
 * 7. Set system time using settimeofday() or FreeRTOS equivalent
 * 8. Close socket
 *
 * @param server NTP server hostname (NULL = use default "pool.ntp.org")
 * @param result Output parameter for sync result (can be NULL)
 *
 * @return true if time synced successfully, false on error
 *
 * @note Requires active Wi-Fi/network connection
 * @note Requires DNS resolution capability
 * @note Sets system time globally - affects all time-dependent operations
 *
 * @example
 * tesaiot_sntp_result_t result;
 * if (tesaiot_sntp_sync_time("time.google.com", &result)) {
 *     printf("Time synced: %s", ctime(&result.unix_time));
 *     printf("Stratum: %u, RTT: %u ms\n", result.stratum, result.round_trip_delay_ms);
 * } else {
 *     printf("NTP sync failed\n");
 * }
 */
bool tesaiot_sntp_sync_time(const char *server, tesaiot_sntp_result_t *result);

/**
 * @brief Get current Unix timestamp after NTP sync
 *
 * Helper function to get current time_t after NTP synchronization.
 * Equivalent to time(NULL) but ensures NTP sync happened first.
 *
 * @param synced_time Output parameter for Unix timestamp (can be NULL)
 *
 * @return true if system time is valid (NTP synced), false otherwise
 *
 * @note Call tesaiot_sntp_sync_time() first to ensure valid system time
 *
 * @example
 * time_t now;
 * if (tesaiot_sntp_get_time(&now)) {
 *     printf("Current time: %s", ctime(&now));
 * }
 */
bool tesaiot_sntp_get_time(time_t *synced_time);

/**
 * @brief Check if system time is synchronized
 *
 * Checks if system time is valid (i.e., NTP sync succeeded at least once).
 * Useful for certificate validation that requires accurate time.
 *
 * @return true if system time is synchronized, false if time invalid
 *
 * @note System time is considered invalid if < year 2020 (Unix timestamp < 1577836800)
 */
bool tesaiot_sntp_is_time_synced(void);

/******************************************************************************
 * Timezone Support (Added 2025-10-28)
 ******************************************************************************/

/**
 * Thailand timezone offset (UTC+7)
 * For other timezones, adjust this value:
 * - UTC+0:  0
 * - UTC+7:  7 (Thailand, Vietnam, Indonesia)
 * - UTC+8:  8 (Singapore, Malaysia, China)
 * - UTC+9:  9 (Japan, Korea)
 * - UTC-5: -5 (US Eastern Standard Time)
 */
#define TESAIOT_TIMEZONE_OFFSET_HOURS   7

/**
 * @brief Get current time in local timezone (Thailand = UTC+7)
 *
 * This function converts system time (UTC) to local timezone.
 * System time remains UTC for certificate validation and platform communication.
 * Use this function ONLY for displaying time to users.
 *
 * @param local_time Output parameter for local time (Unix timestamp adjusted for timezone)
 *
 * @return true if successful, false if time not synced
 *
 * @note System time (UTC) is preserved - this function only adds timezone offset
 * @note Certificate validation still uses UTC (no impact)
 *
 * @example
 * time_t local;
 * if (tesaiot_sntp_get_local_time(&local)) {
 *     printf("Local time (UTC+7): %s", ctime(&local));
 * }
 */
bool tesaiot_sntp_get_local_time(time_t *local_time);

/**
 * @brief Format timestamp as string in local timezone
 *
 * Formats Unix timestamp (UTC) to human-readable string in local timezone.
 * Format: "YYYY-MM-DD HH:MM:SS UTC+7"
 *
 * @param timestamp Unix timestamp (UTC) to format
 * @param buffer Output buffer for formatted string
 * @param buffer_size Size of buffer (recommended: at least 64 bytes)
 *
 * @return Number of characters written (excluding null terminator)
 *
 * @example
 * char time_str[64];
 * time_t now;
 * tesaiot_sntp_get_time(&now);
 * tesaiot_sntp_format_local_time(now, time_str, sizeof(time_str));
 * printf("[TIME] %s\n", time_str);  // Output: "[TIME] 2025-10-28 16:45:30 UTC+7"
 */
int tesaiot_sntp_format_local_time(time_t timestamp, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* TESAIOT_PLATFORM_H_ */
