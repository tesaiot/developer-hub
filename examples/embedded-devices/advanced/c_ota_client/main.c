/*
 * TESAIoT Platform - OTA HTTPS Client Main Program
 *
 * Copyright (c) 2026 TESAIoT Platform (TESA)
 * Licensed under the Apache License, Version 2.0
 *
 * This is a reference implementation for testing the TESAIoT OTA Service
 * from Linux/MacOS development machines. For embedded deployment, adapt
 * the ota_client.c to use platform-specific HTTP and TLS libraries.
 *
 * Usage:
 *   ./ota_client --device-id "your-uuid" --server "admin.tesaiot.com"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "ota_client.h"

/* =============================================================================
 * Command Line Options
 * ============================================================================= */

static void print_usage(const char *prog)
{
    printf("TESAIoT OTA HTTPS Client Tester (C Version)\n");
    printf("\n");
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -d, --device-id ID     Device UUID (required)\n");
    printf("  -s, --server HOST      OTA server hostname (default: %s)\n", OTA_SERVER_HOST);
    printf("  -p, --port PORT        Server port (default: %d)\n", OTA_SERVER_PORT);
    printf("  -v, --version VER      Current firmware version (default: %s)\n", OTA_CURRENT_VERSION);
    printf("  -P, --platform TYPE    Platform type (default: %s)\n", OTA_PLATFORM);
    printf("  -t, --token TOKEN      JWT authentication token\n");
    printf("  -c, --ca-cert FILE     CA certificate file for TLS\n");
    printf("  -C, --client-cert FILE Client certificate for mTLS\n");
    printf("  -K, --client-key FILE  Client private key for mTLS\n");
    printf("  -n, --no-tls           Disable TLS (for local testing)\n");
    printf("  -j, --job-only         Only check for job, don't download\n");
    printf("  -l, --loop SEC         Polling loop interval (0 = check once)\n");
    printf("  -h, --help             Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  # Check for updates (single poll)\n");
    printf("  %s --device-id \"95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9\" \\\n", prog);
    printf("      --server \"admin.tesaiot.com\" --token \"your-jwt-token\"\n");
    printf("\n");
    printf("  # Continuous polling every 60 seconds\n");
    printf("  %s -d \"device-uuid\" -s \"admin.tesaiot.com\" -t \"token\" -l 60\n", prog);
    printf("\n");
    printf("  # Local testing without TLS\n");
    printf("  %s -d \"test-device\" -s \"localhost\" -p 8080 -n\n", prog);
    printf("\n");
}

/* =============================================================================
 * Main Program
 * ============================================================================= */

int main(int argc, char *argv[])
{
    ota_client_t ota;
    ota_error_t err;

    /* Configuration from command line */
    char device_id[64] = "";
    char server[128] = OTA_SERVER_HOST;
    int port = OTA_SERVER_PORT;
    char version[32] = OTA_CURRENT_VERSION;
    char platform[32] = OTA_PLATFORM;
    char token[512] = "";
    char ca_cert[256] = "";
    char client_cert[256] = "";
    char client_key[256] = "";
    int use_tls = 1;
    int job_only = 0;
    int loop_interval = 0;

    /* Parse command line options */
    static struct option long_options[] = {
        {"device-id",    required_argument, 0, 'd'},
        {"server",       required_argument, 0, 's'},
        {"port",         required_argument, 0, 'p'},
        {"version",      required_argument, 0, 'v'},
        {"platform",     required_argument, 0, 'P'},
        {"token",        required_argument, 0, 't'},
        {"ca-cert",      required_argument, 0, 'c'},
        {"client-cert",  required_argument, 0, 'C'},
        {"client-key",   required_argument, 0, 'K'},
        {"no-tls",       no_argument,       0, 'n'},
        {"job-only",     no_argument,       0, 'j'},
        {"loop",         required_argument, 0, 'l'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt, opt_index = 0;
    while ((opt = getopt_long(argc, argv, "d:s:p:v:P:t:c:C:K:njl:h",
                              long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'd':
                strncpy(device_id, optarg, sizeof(device_id) - 1);
                break;
            case 's':
                strncpy(server, optarg, sizeof(server) - 1);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'v':
                strncpy(version, optarg, sizeof(version) - 1);
                break;
            case 'P':
                strncpy(platform, optarg, sizeof(platform) - 1);
                break;
            case 't':
                strncpy(token, optarg, sizeof(token) - 1);
                break;
            case 'c':
                strncpy(ca_cert, optarg, sizeof(ca_cert) - 1);
                break;
            case 'C':
                strncpy(client_cert, optarg, sizeof(client_cert) - 1);
                break;
            case 'K':
                strncpy(client_key, optarg, sizeof(client_key) - 1);
                break;
            case 'n':
                use_tls = 0;
                break;
            case 'j':
                job_only = 1;
                break;
            case 'l':
                loop_interval = atoi(optarg);
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    /* Validate required arguments */
    if (device_id[0] == '\0') {
        fprintf(stderr, "Error: --device-id is required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Print banner */
    printf("============================================================\n");
    printf("TESAIoT OTA HTTPS Client Tester (C Version)\n");
    printf("============================================================\n");
    printf("Device ID:  %s\n", device_id);
    printf("Server:     %s://%s:%d\n", use_tls ? "https" : "http", server, port);
    printf("TLS:        %s\n", use_tls ? "Enabled" : "Disabled");
    printf("Version:    %s\n", version);
    printf("Platform:   %s\n", platform);
    printf("============================================================\n\n");

    /* Initialize OTA client */
    err = ota_init(&ota, device_id, version, platform);
    if (err != OTA_OK) {
        fprintf(stderr, "Failed to initialize OTA client: %s\n", ota_error_str(err));
        return 1;
    }

    /* Configure server */
    ota_set_server(&ota, server, port, use_tls);

    /* Configure authentication */
    if (token[0] != '\0') {
        ota_set_auth_token(&ota, token);
    }

    /* Configure mTLS if certificates provided */
    if (ca_cert[0] != '\0' || client_cert[0] != '\0') {
        ota_set_mtls(&ota,
                     ca_cert[0] != '\0' ? ca_cert : NULL,
                     client_cert[0] != '\0' ? client_cert : NULL,
                     client_key[0] != '\0' ? client_key : NULL);
    }

    /* Main polling loop */
    do {
        printf("\n[INFO] Checking for OTA updates...\n");

        err = ota_check_for_update(&ota);

        switch (err) {
            case OTA_OK:
                printf("\n[INFO] Update available!\n");
                printf("  Firmware: %s v%s\n", ota.job.firmware_id, ota.job.version);
                printf("  Size: %u bytes\n", ota.job.file_size);

                if (!job_only) {
                    printf("\n[INFO] Starting download...\n");
                    err = ota_download_firmware(&ota);
                    if (err == OTA_OK) {
                        printf("[INFO] Download complete!\n");

                        err = ota_verify_firmware(&ota);
                        if (err == OTA_OK) {
                            printf("[INFO] Firmware verified successfully!\n");
                        } else {
                            printf("[ERROR] Verification failed: %s\n", ota_error_str(err));
                        }
                    } else {
                        printf("[ERROR] Download failed: %s\n", ota_error_str(err));
                    }
                }
                break;

            case OTA_NO_UPDATE:
                printf("\n[INFO] No update available\n");
                break;

            default:
                printf("\n[ERROR] Check failed: %s\n", ota_error_str(err));
                break;
        }

        if (loop_interval > 0) {
            printf("\n[INFO] Waiting %d seconds before next check...\n", loop_interval);
            sleep(loop_interval);
        }

    } while (loop_interval > 0);

    printf("\n[INFO] Done.\n");

    return (err == OTA_OK || err == OTA_NO_UPDATE) ? 0 : 1;
}
