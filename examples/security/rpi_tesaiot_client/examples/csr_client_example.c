/**
 * @file main.c
 * @brief CSR Workflow Thin Client
 * @version 2.0
 * @date 2026-01-23
 *
 * Thin client that uses TESAIoT Library (libtesaiot.a)
 * All CSR logic is in tesaiot_library/src/tesaiot_csr.c
 *
 * Commands (Reference: PSoC E84 Menu 1,2,3,5):
 *   ./tesaiot_csr_client           - Run CSR workflow (default)
 *   ./tesaiot_csr_client run       - Run CSR workflow
 *   ./tesaiot_csr_client identity  - Print factory UID and certificate
 *   ./tesaiot_csr_client mqtt      - Test MQTT connection
 *   ./tesaiot_csr_client diag      - OPTIGA metadata diagnostics
 *   ./tesaiot_csr_client license   - Verify license
 *   ./tesaiot_csr_client help      - Show help
 */

#include <tesaiot.h>
#include <tesaiot_csr.h>
#include <tesaiot_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*----------------------------------------------------------------------------
 * CA Certificate Helper
 *---------------------------------------------------------------------------*/

/**
 * @brief Write embedded CA certificate to temp file
 * @return 0 on success, -1 on failure
 */
static int write_ca_cert_to_file(void)
{
    FILE *fp = fopen(TESAIOT_CA_CERT_PATH, "w");
    if (!fp) {
        fprintf(stderr, "Failed to create CA cert file: %s\n", TESAIOT_CA_CERT_PATH);
        return -1;
    }
    fprintf(fp, "%s", TESAIOT_CA_CERTIFICATE);
    fclose(fp);
    printf("[INIT] CA certificate written to: %s\n", TESAIOT_CA_CERT_PATH);
    return 0;
}

/*----------------------------------------------------------------------------
 * Print Usage
 *---------------------------------------------------------------------------*/

/* Global target OID (parsed from command line) */
static uint16_t g_target_oid = 0;  /* 0 = use default (0xE0E2) */

static void print_usage(const char *prog_name)
{
    printf("\n");
    printf("===========================================================\n");
    printf("  TESAIoT CSR Workflow - Raspberry Pi Edition\n");
    printf("  Version: 2.1\n");
    printf("===========================================================\n");
    printf("\n");
    printf("Usage: %s [command] [options]\n", prog_name);
    printf("\n");
    printf("Commands:\n");
    printf("  run       Run CSR workflow (certificate enrollment) [DEFAULT]\n");
    printf("  identity  Print factory UID and factory certificate\n");
    printf("  mqtt      Test MQTT connection with current certificate\n");
    printf("  diag      Test OPTIGA Trust M metadata operations\n");
    printf("  license   Verify device license\n");
    printf("  help      Show this help message\n");
    printf("\n");
    printf("Options:\n");
    printf("  --target-oid 0xE0E2   Target OID for device certificate (default: 0xE0E2)\n");
    printf("                        Valid OIDs: 0xE0E1, 0xE0E2, 0xE0E3\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                         # Run CSR workflow (default OID 0xE0E2)\n", prog_name);
    printf("  %s run                     # Run CSR workflow\n", prog_name);
    printf("  %s run --target-oid 0xE0E3 # Write certificate to OID 0xE0E3\n", prog_name);
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
    printf("[MQTT-Test] Run 'run' command to test full CSR workflow with MQTT.\n");

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
 * Command: Run CSR Workflow (Menu 3)
 * Reference: PSoC E84 tesaiot_run_csr_workflow()
 *---------------------------------------------------------------------------*/

static int cmd_run_csr_workflow(void)
{
    int ret;

    printf("\n[CSR] Starting CSR Workflow...\n");
    printf("[CSR] -------------------------------------------\n");

    /* Write CA certificate to temp file */
    if (write_ca_cert_to_file() != 0) {
        return -1;
    }

    /* Configure CSR workflow
     * PSoC E84 Reference Pattern:
     * - device_id (UUID): For MQTT topics and username
     * - device_uid (Trust M UID): For MQTT client identifier
     */
    tesaiot_csr_workflow_config_t csr_config;
    memset(&csr_config, 0, sizeof(csr_config));

    /* All config from tesaiot_config.h */
    snprintf(csr_config.device_id, sizeof(csr_config.device_id),
             "%s", TESAIOT_DEVICE_ID);
    snprintf(csr_config.device_uid, sizeof(csr_config.device_uid),
             "%s", TESAIOT_DEVICE_UID);
    snprintf(csr_config.mqtt_broker_url, sizeof(csr_config.mqtt_broker_url),
             "%s", TESAIOT_MQTT_BROKER_URL);
    snprintf(csr_config.ca_cert_path, sizeof(csr_config.ca_cert_path),
             "%s", TESAIOT_CA_CERT_PATH);
    snprintf(csr_config.factory_cert_path, sizeof(csr_config.factory_cert_path),
             "%s", TESAIOT_FACTORY_CERT_OID);
    snprintf(csr_config.factory_key_path, sizeof(csr_config.factory_key_path),
             "%s", TESAIOT_FACTORY_KEY_OID);
    /* Target OID from command line (0 = use default 0xE0E2) */
    csr_config.target_oid = g_target_oid;

    /* Run CSR workflow (blocking call) */
    ret = tesaiot_csr_workflow_run_blocking(&csr_config);

    printf("[CSR] -------------------------------------------\n");
    if (ret == 0) {
        uint16_t oid = g_target_oid ? g_target_oid : 0xE0E2;
        printf("[CSR] ✓ CSR Workflow completed successfully!\n");
        printf("[CSR] Device certificate enrolled to OID 0x%04X\n", oid);
    } else {
        fprintf(stderr, "[CSR] ✗ CSR Workflow failed (error %d)\n", ret);
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
                    printf("[INIT] Target OID: 0x%04X\n", g_target_oid);
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

    } else if (strcmp(cmd, "run") == 0 || strcmp(cmd, "csr") == 0) {
        /* Menu 3: Full CSR workflow */
        /* Verify license first */
        printf("[LICENSE] Verifying device license...\n");
        ret = tesaiot_verify_license();
        if (ret != 0) {
            fprintf(stderr, "[LICENSE] ✗ License verification FAILED\n");
            fprintf(stderr, "[LICENSE] Device is NOT authorized for TESAIoT services\n");
            exit_code = EXIT_FAILURE;
        } else {
            printf("[LICENSE] ✓ License VALID\n");
            exit_code = cmd_run_csr_workflow() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        }

    } else if (strcmp(cmd, "diag") == 0 || strcmp(cmd, "metadata") == 0) {
        /* Menu 5: OPTIGA metadata diagnostics */
        exit_code = cmd_run_diagnostics() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else if (strcmp(cmd, "license") == 0) {
        /* Verify license */
        exit_code = cmd_verify_license() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    } else if (strcmp(cmd, "test-sig") == 0) {
        /* TEST ONLY: CSR generation without license check */
        printf("[TEST] Testing CSR signature generation (no license check)...\n");
        printf("[TEST] This bypasses license for testing SEQUENCE wrapper fix\n\n");

        /* Write CA cert for MQTT (not really needed for this test) */
        write_ca_cert_to_file();

        /* Configure CSR workflow */
        tesaiot_csr_workflow_config_t csr_config;
        memset(&csr_config, 0, sizeof(csr_config));
        snprintf(csr_config.device_id, sizeof(csr_config.device_id), "%s", TESAIOT_DEVICE_ID);
        snprintf(csr_config.device_uid, sizeof(csr_config.device_uid), "%s", TESAIOT_DEVICE_UID);
        snprintf(csr_config.mqtt_broker_url, sizeof(csr_config.mqtt_broker_url), "%s", TESAIOT_MQTT_BROKER_URL);
        snprintf(csr_config.ca_cert_path, sizeof(csr_config.ca_cert_path), "%s", TESAIOT_CA_CERT_PATH);
        snprintf(csr_config.factory_cert_path, sizeof(csr_config.factory_cert_path), "%s", TESAIOT_FACTORY_CERT_OID);
        snprintf(csr_config.factory_key_path, sizeof(csr_config.factory_key_path), "%s", TESAIOT_FACTORY_KEY_OID);

        /* Run CSR workflow (will generate CSR, connect to MQTT, and publish) */
        ret = tesaiot_csr_workflow_run_blocking(&csr_config);

        /* Verify CSR if it was saved to debug file */
        printf("\n[TEST] Verifying generated CSR...\n");
        ret = system("openssl req -in /tmp/tesaiot_debug_csr.pem -noout -verify 2>&1");
        if (ret == 0) {
            printf("[TEST] ✓ CSR SIGNATURE VALID!\n");
            exit_code = EXIT_SUCCESS;
        } else {
            printf("[TEST] ✗ CSR signature verification failed\n");
            exit_code = EXIT_FAILURE;
        }

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
