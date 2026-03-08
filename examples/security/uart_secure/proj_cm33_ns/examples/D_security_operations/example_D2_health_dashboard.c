/**
 * @file example_D2_health_dashboard.c
 * @brief Example: Comprehensive device health dashboard
 * @copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform
 *
 * Demonstrates OID Use Case Category D: Security Operations
 *
 * Comprehensive health check covering:
 * - OPTIGA Trust M hardware status
 * - Factory certificate validity (0xE0E0)
 * - Device certificate validity (0xE0E1)
 * - License verification status
 * - MQTT connection status
 * - NTP time sync status
 * - Lifecycle State Object (LcsO)
 * - Boot counter (monotonic counter)
 *
 * OIDs checked: 0xE0E0, 0xE0E1, 0xE0C2, 0xE120 (boot counter)
 *
 * Required: Phase 3 functions (tesaiot_health_check, tesaiot_counter_*)
 */

#include "tesaiot.h"
#include <stdio.h>
#include <string.h>

void example_D2_health_dashboard(void)
{
    printf("\n");
    printf("================================================================\n");
    printf("  Example D2: Device Health Dashboard\n");
    printf("  OID Category: D - Security Operations\n");
    printf("  Pattern: Comprehensive Diagnostics\n");
    printf("================================================================\n\n");

    int rc;

    /* --- Step 1: Run health check --- */
    printf("[Step 1] Running comprehensive health check...\n\n");
    tesaiot_health_report_t report;
    memset(&report, 0, sizeof(report));

    rc = tesaiot_health_check(&report);
    if (rc != TESAIOT_OK) {
        printf("  Health check FAIL: %s\n", tesaiot_error_str((tesaiot_error_t)rc));
        printf("  (Individual field checks may still be useful)\n\n");
    }

    /* --- Step 2: Display health dashboard --- */
    printf("[Step 2] Health Dashboard:\n\n");
    printf("  +====================================================+\n");
    printf("  |          TESAIoT DEVICE HEALTH DASHBOARD           |\n");
    printf("  +====================================================+\n");
    printf("  |                                                    |\n");
    printf("  |  OPTIGA Trust M:    %s                         |\n",
           report.optiga_ok ? "[OK]" : "[!!]");
    printf("  |  Factory Cert:      %s  (OID 0xE0E0)          |\n",
           report.factory_cert_ok ? "[OK]" : "[!!]");
    printf("  |  Device Cert:       %s  (OID 0xE0E1)          |\n",
           report.device_cert_ok ? "[OK]" : "[--]");
    printf("  |  License:           %s                         |\n",
           report.license_ok ? "[OK]" : "[!!]");
    printf("  |  MQTT Connection:   %s                         |\n",
           report.mqtt_ok ? "[OK]" : "[--]");
    printf("  |  NTP Time Sync:     %s                         |\n",
           report.time_synced ? "[OK]" : "[--]");
    printf("  |                                                    |\n");
    printf("  +----------------------------------------------------+\n");
    printf("  |  Cert Expiry:       ");
    if (report.cert_days_left > 0) {
        printf("%lu days remaining        |\n", (unsigned long)report.cert_days_left);
    } else {
        printf("Unknown / Expired           |\n");
    }
    printf("  |  LcsO Value:        0x%02X", report.lcso_value);
    switch (report.lcso_value) {
        case 0x01: printf(" (Creation)              |\n"); break;
        case 0x03: printf(" (Initialization)        |\n"); break;
        case 0x07: printf(" (Operational)           |\n"); break;
        case 0x0F: printf(" (Termination)           |\n"); break;
        default:   printf(" (Unknown)               |\n"); break;
    }
    printf("  |                                                    |\n");
    printf("  +====================================================+\n\n");

    /* --- Step 3: Track boot counter --- */
    printf("[Step 3] Boot Counter (Counter 1 / OID 0xE121)...\n");
    uint32_t boot_count = 0;
    rc = tesaiot_counter_read(1, &boot_count);
    if (rc == TESAIOT_OK) {
        printf("  Boot count: %lu\n", (unsigned long)boot_count);

        /* Increment boot counter */
        rc = tesaiot_counter_increment(1, 1);
        if (rc == TESAIOT_OK) {
            printf("  Incremented to: %lu\n\n", (unsigned long)(boot_count + 1));
        }
    } else {
        printf("  Counter not available (may need configuration)\n\n");
    }

    /* --- Step 4: Overall status assessment --- */
    printf("[Step 4] Status Assessment:\n\n");

    int critical_ok = report.optiga_ok && report.license_ok;
    int operational_ok = critical_ok && report.factory_cert_ok;
    int full_ok = operational_ok && report.device_cert_ok &&
                  report.mqtt_ok && report.time_synced;

    if (full_ok) {
        printf("  STATUS: FULLY OPERATIONAL\n");
        printf("  All systems nominal. Device is ready for production.\n\n");
    } else if (operational_ok) {
        printf("  STATUS: OPERATIONAL (with warnings)\n");
        if (!report.device_cert_ok)
            printf("  - Device cert missing (run CSR workflow)\n");
        if (!report.mqtt_ok)
            printf("  - MQTT not connected\n");
        if (!report.time_synced)
            printf("  - Time not synchronized (run NTP sync)\n");
        printf("\n");
    } else if (critical_ok) {
        printf("  STATUS: DEGRADED\n");
        printf("  Critical systems OK but certificates may need attention.\n\n");
    } else {
        printf("  STATUS: CRITICAL\n");
        if (!report.optiga_ok)
            printf("  - OPTIGA hardware NOT responding!\n");
        if (!report.license_ok)
            printf("  - License INVALID!\n");
        printf("\n");
    }

    /* --- Step 5: Recommended actions --- */
    printf("[Step 5] Recommended Actions:\n\n");
    if (!report.device_cert_ok) {
        printf("  >> Run CSR Workflow (Menu 3) to provision device certificate\n");
    }
    if (!report.mqtt_ok) {
        printf("  >> Test MQTT Connection (Menu 2) to verify connectivity\n");
    }
    if (!report.time_synced) {
        printf("  >> Ensure NTP server is reachable for time synchronization\n");
    }
    if (report.cert_days_left > 0 && report.cert_days_left < 30) {
        printf("  >> Certificate expires in %lu days - renewal recommended!\n",
               (unsigned long)report.cert_days_left);
    }
    if (report.optiga_ok && report.license_ok &&
        report.factory_cert_ok && report.device_cert_ok) {
        printf("  >> No action required - device is healthy\n");
    }
    printf("\n");

    /* --- Step 6: MQTT health report format --- */
    printf("[Step 6] MQTT Health Report Format:\n");
    printf("  Topic: device/{uid}/health\n");
    printf("  {\n");
    printf("    \"optiga\": %s,\n", report.optiga_ok ? "true" : "false");
    printf("    \"factory_cert\": %s,\n", report.factory_cert_ok ? "true" : "false");
    printf("    \"device_cert\": %s,\n", report.device_cert_ok ? "true" : "false");
    printf("    \"license\": %s,\n", report.license_ok ? "true" : "false");
    printf("    \"mqtt\": %s,\n", report.mqtt_ok ? "true" : "false");
    printf("    \"ntp\": %s,\n", report.time_synced ? "true" : "false");
    printf("    \"cert_days_left\": %lu,\n", (unsigned long)report.cert_days_left);
    printf("    \"lcso\": \"0x%02X\",\n", report.lcso_value);
    printf("    \"boot_count\": %lu\n", (unsigned long)boot_count);
    printf("  }\n\n");

    printf("================================================================\n");
    printf("  Example D2 Complete\n");
    printf("  Tip: Run this periodically and publish to MQTT\n");
    printf("  for fleet health monitoring.\n");
    printf("================================================================\n\n");
}
