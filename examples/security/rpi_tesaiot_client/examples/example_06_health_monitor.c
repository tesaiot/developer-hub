/**
 * @file example_06_health_monitor.c
 * @brief Example: System Health Monitor Dashboard
 * @version 3.0.0
 * @date 2026-02-08
 *
 * (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.
 *
 * Demonstrates:
 *   - tesaiot_health_check()         — Comprehensive diagnostic check
 *   - tesaiot_counter_read()         — Read monotonic counter
 *   - tesaiot_counter_increment()    — Increment counter (irreversible)
 *
 * OIDs used:
 *   0xE0C0 — Lifecycle State Object (LcsO)
 *   0xE0E0 — Factory Certificate (health check)
 *   0xE0E1 — Device Certificate (health check)
 *   0xE120 — Anti-Replay Counter
 *   0xE121 — Firmware Version Counter
 *   0xE122 — Usage Meter Counter
 *   0xE123 — Boot Counter
 *
 * Use Cases:
 *   D1: Anti-Replay Protection (message sequence counter)
 *   D2: Firmware Anti-Rollback (version counter)
 *   D3: Usage Metering / Pay-Per-Use (usage counter)
 *
 * Application Patterns:
 *   - Smart Home: Device health dashboard
 *   - Industrial: Predictive maintenance via counter monitoring
 *   - Medical IoT: FDA compliance audit trail with boot counter
 */

#include <stdio.h>
#include <string.h>
#include "tesaiot.h"
#include "tesaiot_crypto.h"
#include "tesaiot_advanced.h"

/*---------------------------------------------------------------------------
 * Health Check Dashboard
 *
 * Run a comprehensive check of all device subsystems.
 * Useful for: monitoring dashboards, self-diagnostics, commissioning.
 *---------------------------------------------------------------------------*/
static int demo_health_check(void)
{
    printf("\n=== Device Health Check Dashboard ===\n");

    tesaiot_health_report_t report;
    memset(&report, 0, sizeof(report));

    int rc = tesaiot_health_check(&report);

    printf("  +-------------------------------+--------+\n");
    printf("  | Component                     | Status |\n");
    printf("  +-------------------------------+--------+\n");
    printf("  | OPTIGA Trust M                | %s  |\n", report.optiga_ok ? " OK " : "FAIL");
    printf("  | Factory Certificate (0xE0E0)  | %s  |\n", report.factory_cert_ok ? " OK " : "FAIL");
    printf("  | Device Certificate (0xE0E1)   | %s  |\n", report.device_cert_ok ? " OK " : "FAIL");
    printf("  | License Verification          | %s  |\n", report.license_ok ? " OK " : "FAIL");
    printf("  | MQTT Connection               | %s  |\n", report.mqtt_ok ? " OK " : " N/A");
    printf("  | External Tools Integrity      | %s  |\n", report.tools_ok ? " OK " : "FAIL");
    printf("  | Lifecycle State (LcsO)        | 0x%02X   |\n", report.lcso_value);
    printf("  +-------------------------------+--------+\n");
    printf("  Overall: %s\n", rc == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");

    /* Interpret LcsO value */
    const char *state;
    switch (report.lcso_value) {
        case 0x01: state = "CREATION"; break;
        case 0x03: state = "INITIALIZATION"; break;
        case 0x07: state = "OPERATIONAL"; break;
        case 0x0F: state = "TERMINATION"; break;
        default:   state = "UNKNOWN"; break;
    }
    printf("  Lifecycle State: %s (0x%02X)\n", state, report.lcso_value);

    return rc;
}

/*---------------------------------------------------------------------------
 * Use Case D1: Anti-Replay Protection
 *
 * Each message includes a monotonic counter value.
 * Receiver rejects messages with counter <= last seen.
 *---------------------------------------------------------------------------*/
static int demo_anti_replay_counter(void)
{
    printf("\n=== D1: Anti-Replay Counter (Counter 0 = OID 0xE120) ===\n");

    uint32_t counter;
    int rc = tesaiot_counter_read(0, &counter);
    if (rc != 0) {
        printf("  ERROR: Failed to read counter (rc=%d)\n", rc);
        return rc;
    }
    printf("  Current counter value: %u\n", counter);

    /* Simulate sending a message with sequence number */
    printf("  Sending message with seq=%u\n", counter);

    /* Increment counter (irreversible!) */
    rc = tesaiot_counter_increment(0, 1);
    if (rc != 0) {
        printf("  ERROR: Counter increment failed (rc=%d)\n", rc);
        return rc;
    }

    /* Read back to verify */
    uint32_t new_counter;
    tesaiot_counter_read(0, &new_counter);
    printf("  Counter after increment: %u\n", new_counter);
    printf("  WARNING: Counter increments are irreversible (~600K lifetime)\n");

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case D2: Firmware Anti-Rollback
 *
 * Counter tracks minimum allowed firmware version.
 * Reject any firmware with version <= counter.
 *---------------------------------------------------------------------------*/
static int demo_firmware_anti_rollback(void)
{
    printf("\n=== D2: Firmware Anti-Rollback (Counter 1 = OID 0xE121) ===\n");

    uint32_t fw_counter;
    int rc = tesaiot_counter_read(1, &fw_counter);
    if (rc != 0) {
        printf("  ERROR: Failed to read FW counter (rc=%d)\n", rc);
        return rc;
    }
    printf("  Current minimum FW version: %u\n", fw_counter);

    /* Simulate firmware update check */
    uint32_t new_fw_version = fw_counter + 1;
    uint32_t old_fw_version = (fw_counter > 0) ? fw_counter - 1 : 0;

    printf("  Update to v%u: %s (v%u > v%u)\n",
           new_fw_version,
           new_fw_version > fw_counter ? "ALLOWED" : "BLOCKED",
           new_fw_version, fw_counter);

    printf("  Rollback to v%u: %s (v%u <= v%u)\n",
           old_fw_version,
           old_fw_version > fw_counter ? "ALLOWED" : "BLOCKED",
           old_fw_version, fw_counter);

    /*
     * In production after successful FW update:
     *   tesaiot_counter_increment(1, 1);
     * This permanently prevents rollback to current version.
     */

    return 0;
}

/*---------------------------------------------------------------------------
 * Use Case D3: Usage Metering / Pay-Per-Use
 *
 * Counter tracks usage. When threshold is reached, device locks.
 * Platform can reset via Protected Update.
 *---------------------------------------------------------------------------*/
static int demo_usage_metering(void)
{
    printf("\n=== D3: Usage Metering (Counter 2 = OID 0xE122) ===\n");

    uint32_t usage;
    int rc = tesaiot_counter_read(2, &usage);
    if (rc != 0) {
        printf("  ERROR: Failed to read usage counter (rc=%d)\n", rc);
        return rc;
    }

    uint32_t limit = 1000; /* Example: 1000 uses allowed */
    uint32_t remaining = (usage < limit) ? (limit - usage) : 0;

    printf("  Usage: %u / %u\n", usage, limit);
    printf("  Remaining: %u operations\n", remaining);

    if (remaining == 0) {
        printf("  STATUS: LOCKED - Contact TESAIoT Platform for renewal\n");
        printf("  (Platform resets counter via Protected Update)\n");
    } else {
        printf("  STATUS: ACTIVE\n");
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * Main Example Entry Point
 *---------------------------------------------------------------------------*/
int example_06_health_monitor(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("  Example 06: System Health Monitor Dashboard\n");
    printf("  TESAIoT Secure Library v3.0.0\n");
    printf("========================================================\n");

    int rc = tesaiot_init();
    if (rc != 0) return rc;

    rc = tesaiot_verify_license();
    if (rc != 0) { tesaiot_deinit(); return rc; }

    demo_health_check();
    demo_anti_replay_counter();
    demo_firmware_anti_rollback();
    demo_usage_metering();

    tesaiot_deinit();
    printf("\n=== Example 06 Complete ===\n\n");
    return 0;
}

int main(void) { return example_06_health_monitor(); }
