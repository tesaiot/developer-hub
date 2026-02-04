/**
 * @file main.c
 * @brief Protected Update Workflow Thin Client
 * @version 2.0
 * @date 2026-01-23
 *
 * Thin client that uses TESAIoT Library (libtesaiot.a)
 * All Protected Update logic is in tesaiot_library/src/tesaiot_protected_update.c
 *
 * Commands (Reference: PSoC E84 Menu 1,2,4,5):
 *   ./tesaiot_pu_client            - Run Protected Update workflow (default)
 *   ./tesaiot_pu_client run        - Run Protected Update workflow
 *   ./tesaiot_pu_client identity   - Print factory UID and certificate
 *   ./tesaiot_pu_client mqtt       - Test MQTT connection
 *   ./tesaiot_pu_client diag       - OPTIGA metadata diagnostics
 *   ./tesaiot_pu_client license    - Verify license
 *   ./tesaiot_pu_client help       - Show help
 */

/* IMPORTANT: Include tesaiot_config.h FIRST to set device-specific overrides */
#include <tesaiot_config.h>

#include <tesaiot.h>
#include <tesaiot_protected_update.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*----------------------------------------------------------------------------
 * Global Configuration
 *---------------------------------------------------------------------------*/

/* Global target OID (parsed from command line) */
static uint16_t g_target_oid = 0;  /* 0 = use OID from manifest */

/*----------------------------------------------------------------------------
 * Print Usage
 *---------------------------------------------------------------------------*/

static void print_usage(const char *prog_name)
{
    printf("\n");
    printf("===========================================================\n");
    printf("  TESAIoT Protected Update Workflow - Raspberry Pi Edition\n");
    printf("  Version: 2.1\n");
    printf("===========================================================\n");
    printf("\n");
    printf("Usage: %s [command] [options]\n", prog_name);
    printf("\n");
    printf("Commands:\n");
    printf("  run       Run Protected Update workflow (MQTT-based) [DEFAULT]\n");
    printf("  identity  Print factory UID and factory certificate\n");
    printf("  mqtt      Test MQTT connection with current certificate\n");
    printf("  diag      Test OPTIGA Trust M metadata operations\n");
    printf("  license   Verify device license\n");
    printf("  help      Show this help message\n");
    printf("\n");
    printf("Options:\n");
    printf("  --target-oid 0xE0E2   Override target OID (default: from manifest)\n");
    printf("                        Valid OIDs: 0xE0E1, 0xE0E2, 0xE0E3\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                         # Run PU workflow (OID from manifest)\n", prog_name);
    printf("  %s run                     # Run PU workflow\n", prog_name);
    printf("  %s run --target-oid 0xE0E3 # Override target OID to 0xE0E3\n", prog_name);
    printf("  %s identity                # Display Trust M UID and factory cert\n", prog_name);
    printf("  %s license                 # Check license status\n", prog_name);
    printf("\n");
}

/*----------------------------------------------------------------------------
 * Command: Print Factory Identity (Menu 1)
 * Reference: PSoC E84 print_factory_identity()
 *---------------------------------------------------------------------------*/

static int cmd_print_identity(void)
{
    char factory_uid[128];
    int ret;

    printf("\n[IDENTITY] Reading Factory Identity from OPTIGA Trust M...\n");
    printf("[IDENTITY] -------------------------------------------\n");

    /* Read Factory UID from OID 0xE0C2 */
    ret = tesaiot_read_trustm_uid(factory_uid, sizeof(factory_uid));
    if (ret == 0) {
        printf("[IDENTITY] Factory UID: %s\n", factory_uid);
    } else {
        fprintf(stderr, "[IDENTITY] Failed to read factory UID (error %d)\n", ret);
        return ret;
    }

    /* Factory Certificate - use trustm_cert command line tool */
    printf("[IDENTITY] Factory Certificate: Use 'trustm_cert -r 0xe0e0' to read\n");

    printf("[IDENTITY] -------------------------------------------\n");
    return 0;
}

/*----------------------------------------------------------------------------
 * Command: Test MQTT Connection (Menu 2)
 * Reference: PSoC E84 test_mqtt_connection()
 *---------------------------------------------------------------------------*/

static int cmd_test_mqtt(void)
{
    printf("\n[MQTT-Test] Starting MQTT Connection Test...\n");
    printf("[MQTT-Test] -------------------------------------------\n");
    printf("[MQTT-Test] This will test MQTT connectivity using\n");
    printf("[MQTT-Test] the current device certificate.\n");
    printf("[MQTT-Test] -------------------------------------------\n");

    /* TODO: Implement MQTT connection test in library */
    printf("[MQTT-Test] Feature not implemented yet.\n");
    printf("[MQTT-Test] Run 'run' command to test full PU workflow with MQTT.\n");

    return 0;
}

/*----------------------------------------------------------------------------
 * Command: OPTIGA Metadata Diagnostics (Menu 5)
 * Reference: PSoC E84 tesaiot_test_metadata_operations()
 *---------------------------------------------------------------------------*/

static int cmd_run_diagnostics(void)
{
    printf("\n[DIAG] Starting OPTIGA Trust M Metadata Diagnostics...\n");
    printf("[DIAG] -------------------------------------------\n");
    printf("[DIAG] This will help diagnose OID permissions and\n");
    printf("[DIAG] metadata configuration issues.\n");
    printf("[DIAG] -------------------------------------------\n");

    /* TODO: Implement metadata diagnostics in library */
    printf("[DIAG] Feature not implemented yet.\n");
    printf("[DIAG] Use 'trustm_metadata' command line tool for diagnostics.\n");

    return 0;
}

/*----------------------------------------------------------------------------
 * Command: Verify License
 *---------------------------------------------------------------------------*/

static int cmd_verify_license(void)
{
    int ret;

    printf("[LICENSE] Verifying device license...\n");
    ret = tesaiot_verify_license();
    if (ret == 0) {
        printf("[LICENSE] ✓ License VALID - Device is authorized for TESAIoT services\n");

        tesaiot_device_info_t device_info;
        if (tesaiot_get_device_info(&device_info) == 0) {
            printf("[LICENSE]   Device UID: %s\n", device_info.uid);
            printf("[LICENSE]   License is active and verified\n");
        }
        return 0;
    } else {
        fprintf(stderr, "[LICENSE] ✗ License verification FAILED\n");
        fprintf(stderr, "[LICENSE]   Device is NOT authorized for TESAIoT services\n");
        fprintf(stderr, "[LICENSE]   Please contact your administrator to obtain a valid license\n");
        return ret;
    }
}

/*----------------------------------------------------------------------------
 * Command: Run Protected Update Workflow (Menu 4)
 * Reference: PSoC E84 tesaiot_run_protected_update_workflow()
 *---------------------------------------------------------------------------*/

static int cmd_run_pu_workflow(void)
{
    int ret;

    printf("\n[PU] Starting Protected Update Workflow...\n");
    printf("[PU] -------------------------------------------\n");
    printf("[PU] This will connect to TESAIoT Platform via MQTT\n");
    printf("[PU] and wait for Protected Update manifest + fragments\n");

    /* Set target OID override if specified */
    if (g_target_oid != 0) {
        tesaiot_pu_set_default_target_oid(g_target_oid);
    }

    /* Run Protected Update workflow (blocking call) */
    ret = tesaiot_run_protected_update_workflow();

    printf("[PU] -------------------------------------------\n");
    if (ret == 0) {
        printf("[PU] ✓ Protected Update completed successfully!\n");
        printf("[PU] Data/firmware updated securely\n");
    } else {
        fprintf(stderr, "[PU] ✗ Protected Update failed (error %d)\n", ret);
    }

    return ret;
}

/*----------------------------------------------------------------------------
 * Main Program
 *---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int ret;
    int exit_code = EXIT_SUCCESS;

    /* Help command (no init needed) */
    if (argc >= 2 && (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    /* Parse --target-oid argument */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--target-oid") == 0 && i + 1 < argc) {
            unsigned int oid_val = 0;
            if (sscanf(argv[i + 1], "0x%x", &oid_val) == 1 ||
                sscanf(argv[i + 1], "0X%x", &oid_val) == 1 ||
                sscanf(argv[i + 1], "%x", &oid_val) == 1) {
                /* Validate OID range (E0E1-E0E3 for device certificates) */
                if (oid_val >= 0xE0E1 && oid_val <= 0xE0E3) {
                    g_target_oid = (uint16_t)oid_val;
                    printf("[INIT] Target OID override: 0x%04X\n", g_target_oid);
                } else {
                    fprintf(stderr, "[ERROR] Invalid OID 0x%04X. Valid range: 0xE0E1-0xE0E3\n", oid_val);
                    return EXIT_FAILURE;
                }
            } else {
                fprintf(stderr, "[ERROR] Invalid OID format: %s\n", argv[i + 1]);
                return EXIT_FAILURE;
            }
            i++;  /* Skip the OID value */
        }
    }

    /* Determine command: default is "run" if no argument or if first arg is an option */
    const char *cmd = "run";
    if (argc >= 2 && argv[1][0] != '-') {
        cmd = argv[1];
    }

    /* Initialize TESAIoT Library (OPTIGA Trust M) */
    printf("[INIT] Initializing OPTIGA Trust M...\n");
    ret = tesaiot_init();
    if (ret != 0) {
        fprintf(stderr, "[ERROR] OPTIGA initialization failed (error %d)\n", ret);
        return EXIT_FAILURE;
    }
    printf("[INIT] OPTIGA Trust M initialized successfully\n");

    /* Process command */
    if (strcmp(cmd, "identity") == 0 || strcmp(cmd, "uid") == 0) {
        /* Menu 1: Print factory UID and certificate */
        exit_code = cmd_print_identity() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else if (strcmp(cmd, "mqtt") == 0 || strcmp(cmd, "test-mqtt") == 0) {
        /* Menu 2: Test MQTT connection */
        exit_code = cmd_test_mqtt() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else if (strcmp(cmd, "run") == 0 || strcmp(cmd, "pu") == 0) {
        /* Menu 4: Full Protected Update workflow */
        /* Verify license first */
        printf("[LICENSE] Verifying device license...\n");
        ret = tesaiot_verify_license();
        if (ret != 0) {
            fprintf(stderr, "[LICENSE] ✗ License verification FAILED\n");
            fprintf(stderr, "[LICENSE] Device is NOT authorized for TESAIoT services\n");
            exit_code = EXIT_FAILURE;
        } else {
            printf("[LICENSE] ✓ License VALID\n");
            exit_code = cmd_run_pu_workflow() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        }

    } else if (strcmp(cmd, "diag") == 0 || strcmp(cmd, "metadata") == 0) {
        /* Menu 5: OPTIGA metadata diagnostics */
        exit_code = cmd_run_diagnostics() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else if (strcmp(cmd, "license") == 0) {
        /* Verify license */
        exit_code = cmd_verify_license() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else {
        fprintf(stderr, "[ERROR] Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        exit_code = EXIT_FAILURE;
    }

    /* Cleanup */
    printf("[CLEANUP] Shutting down OPTIGA Trust M...\n");
    tesaiot_deinit();
    printf("[CLEANUP] Done\n");

    return exit_code;
}
