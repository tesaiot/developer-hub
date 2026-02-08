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

#define _POSIX_C_SOURCE 200809L

/* IMPORTANT: Include tesaiot_config.h FIRST to set device-specific overrides */
#include <tesaiot_config.h>

#include <tesaiot.h>
#include <tesaiot_protected_update.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

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
    printf("  --target-oid 0xNNNN   Override target OID (default: 0x%04X)\n", TESAIOT_PU_OID_DEFAULT_TARGET);
    printf("                        Valid OIDs: 0xE0E1, 0xE0E2, 0xE0E3\n");
    printf("\n");
    printf("  Trust Anchor OID: 0x%04X (configured at build time)\n", TESAIOT_PU_OID_TRUST_ANCHOR);
    printf("\n");
    printf("Examples:\n");
    printf("  %s                         # Run PU workflow (target OID: 0x%04X)\n", prog_name, TESAIOT_PU_OID_DEFAULT_TARGET);
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
 * Diagnostics Helpers — extract fields from openssl x509 -text output
 *---------------------------------------------------------------------------*/

static int diag_extract_next_line(const char *text, const char *marker,
                                   char *buf, size_t buf_sz)
{
    const char *p = strstr(text, marker);
    if (!p) return 0;
    const char *nl = strchr(p, '\n');
    if (!nl) return 0;
    nl++;
    while (*nl == ' ' || *nl == '\t') nl++;
    const char *end = strchr(nl, '\n');
    if (!end) end = nl + strlen(nl);
    size_t len = (size_t)(end - nl);
    if (len >= buf_sz) len = buf_sz - 1;
    memcpy(buf, nl, len);
    buf[len] = '\0';
    return 1;
}

static int diag_extract_same_line(const char *text, const char *marker,
                                   char *buf, size_t buf_sz)
{
    const char *p = strstr(text, marker);
    if (!p) return 0;
    p += strlen(marker);
    while (*p == ' ') p++;
    const char *end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    size_t len = (size_t)(end - p);
    if (len >= buf_sz) len = buf_sz - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return 1;
}

/*----------------------------------------------------------------------------
 * Command: OPTIGA Metadata Diagnostics (Menu 5)
 * Reference: PSoC E84 tesaiot_test_metadata_operations()
 *---------------------------------------------------------------------------*/

static int cmd_run_diagnostics(void)
{
    char cmd[512];
    char output[4096];
    size_t out_len;
    FILE *pipe;

    const char *tools_path = getenv("TRUSTM_TOOLS_PATH");
    if (!tools_path) {
        tools_path = "/home/wiroon/references/linux-optiga-trust-m/bin";
    }

    printf("\n[DIAG] OPTIGA Trust M Diagnostics\n");
    printf("[DIAG] ===========================================\n\n");

    /* --- Section 1: Device Identity (from config, no library needed) --- */
    printf("[DIAG] Device UID:  %s\n", TESAIOT_DEVICE_UID);
    printf("[DIAG] Device ID:   %s\n", TESAIOT_DEVICE_ID);
    snprintf(cmd, sizeof(cmd), "date '+%%Y-%%m-%%d %%H:%%M:%%S %%Z (UTC%%z)'");
    pipe = popen(cmd, "r");
    if (pipe) {
        out_len = fread(output, 1, sizeof(output) - 1, pipe);
        output[out_len] = '\0';
        pclose(pipe);
        char *nl = strchr(output, '\n');
        if (nl) *nl = '\0';
        printf("[DIAG] System Time: %s\n", output);
    }
    printf("[DIAG] Note: certificate dates are in UTC/GMT\n");
    printf("\n");

    /* --- Section 2: Certificate OID Metadata --- */
    printf("[DIAG] --- Certificate OID Metadata ---\n");
    fflush(stdout);
    static const uint16_t cert_oids[] = {0xE0E0, 0xE0E1, 0xE0E2, 0xE0E3};
    static const char *cert_labels[] = {
        "Factory Cert ", "Device Cert 1", "Device Cert 2", "Device Cert 3"
    };

    for (int i = 0; i < 4; i++) {
        snprintf(cmd, sizeof(cmd),
                 "%s/trustm_metadata -r 0x%04X 2>&1",
                 tools_path, cert_oids[i]);
        pipe = popen(cmd, "r");
        if (pipe) {
            out_len = fread(output, 1, sizeof(output) - 1, pipe);
            output[out_len] = '\0';
            pclose(pipe);
            char *summary = strstr(output, "LcsO:");
            if (summary) {
                char *end = summary + strlen(summary) - 1;
                while (end > summary && (*end == '\n' || *end == ' ' || *end == '\t' || *end == ',')) {
                    *end-- = '\0';
                }
                printf("[DIAG] 0x%04X (%s): %s\n", cert_oids[i], cert_labels[i], summary);
            } else {
                printf("[DIAG] 0x%04X (%s): (no metadata)\n", cert_oids[i], cert_labels[i]);
            }
        }
    }
    printf("\n");

    /* --- Section 3: Certificate Details from Target OID --- */
    uint16_t target = g_target_oid ? g_target_oid : TESAIOT_PU_OID_DEFAULT_TARGET;
    printf("[DIAG] --- Certificate at OID 0x%04X ---\n", target);
    printf("[DIAG]   %-24s %s\n", "Field", "Value");
    printf("[DIAG]   %-24s %s\n", "------------------------", "--------------------------------------------");
    fflush(stdout);

    char temp_cert[64];
    snprintf(temp_cert, sizeof(temp_cert), "/tmp/tesaiot_diag_%d.pem", getpid());

    snprintf(cmd, sizeof(cmd), "%s/trustm_data -r 0x%04X -o %s >/dev/null 2>&1",
             tools_path, target, temp_cert);
    int sys_ret = system(cmd);
    if (WEXITSTATUS(sys_ret) == 0) {
        /* Clean PEM: OPTIGA may pad trailing dashes beyond standard 5 */
        {
            FILE *cf = fopen(temp_cert, "rb");
            if (cf) {
                char pem_buf[4096];
                size_t n = fread(pem_buf, 1, sizeof(pem_buf) - 1, cf);
                fclose(cf);
                pem_buf[n] = '\0';
                char *end_mark = strstr(pem_buf, "-----END CERTIFICATE");
                if (end_mark) {
                    strcpy(end_mark, "-----END CERTIFICATE-----\n");
                    cf = fopen(temp_cert, "w");
                    if (cf) {
                        size_t fixed = (size_t)(end_mark - pem_buf) + 27;
                        fwrite(pem_buf, 1, fixed, cf);
                        fclose(cf);
                    }
                }
            }
        }

        char f_serial[128] = "", f_notbefore[128] = "", f_notafter[128] = "";
        char f_subject[256] = "", f_issuer[256] = "";
        char f_algo[128] = "", f_ku[256] = "", f_eku[256] = "", f_san[256] = "";

        /* Basic fields */
        snprintf(cmd, sizeof(cmd),
                 "openssl x509 -in %s -noout -startdate -enddate -subject -issuer 2>&1",
                 temp_cert);
        pipe = popen(cmd, "r");
        if (pipe) {
            out_len = fread(output, 1, sizeof(output) - 1, pipe);
            output[out_len] = '\0';
            pclose(pipe);
            char *saveptr = NULL;
            char *line = strtok_r(output, "\n", &saveptr);
            while (line) {
                if (strncmp(line, "notBefore=", 10) == 0)
                    snprintf(f_notbefore, sizeof(f_notbefore), "%s", line + 10);
                else if (strncmp(line, "notAfter=", 9) == 0)
                    snprintf(f_notafter, sizeof(f_notafter), "%s", line + 9);
                else if (strncmp(line, "subject=", 8) == 0)
                    snprintf(f_subject, sizeof(f_subject), "%s", line + 8);
                else if (strncmp(line, "issuer=", 7) == 0)
                    snprintf(f_issuer, sizeof(f_issuer), "%s", line + 7);
                line = strtok_r(NULL, "\n", &saveptr);
            }
        }

        /* Extended fields from -text */
        snprintf(cmd, sizeof(cmd),
                 "openssl x509 -in %s -noout -text 2>&1", temp_cert);
        pipe = popen(cmd, "r");
        if (pipe) {
            out_len = fread(output, 1, sizeof(output) - 1, pipe);
            output[out_len] = '\0';
            pclose(pipe);
            diag_extract_next_line(output, "Serial Number:", f_serial, sizeof(f_serial));
            diag_extract_same_line(output, "Signature Algorithm:", f_algo, sizeof(f_algo));
            diag_extract_next_line(output, "X509v3 Key Usage:", f_ku, sizeof(f_ku));
            diag_extract_next_line(output, "X509v3 Extended Key Usage:", f_eku, sizeof(f_eku));
            diag_extract_next_line(output, "X509v3 Subject Alternative Name:", f_san, sizeof(f_san));
        }

        /* Print in order */
        if (f_serial[0])    printf("[DIAG]   %-24s %s\n", "Serial Number", f_serial);
        if (f_notbefore[0]) printf("[DIAG]   %-24s %s\n", "Not Before", f_notbefore);
        if (f_notafter[0])  printf("[DIAG]   %-24s %s\n", "Not After", f_notafter);
        if (f_subject[0])   printf("[DIAG]   %-24s %s\n", "Subject", f_subject);
        if (f_issuer[0])    printf("[DIAG]   %-24s %s\n", "Issuer", f_issuer);
        if (f_algo[0])      printf("[DIAG]   %-24s %s\n", "Algorithm", f_algo);
        if (f_ku[0])        printf("[DIAG]   %-24s %s\n", "Key Usage", f_ku);
        if (f_eku[0])       printf("[DIAG]   %-24s %s\n", "Extended Key Usage", f_eku);
        if (f_san[0])       printf("[DIAG]   %-24s %s\n", "SAN", f_san);

        if (!f_serial[0] && !f_notbefore[0])
            printf("[DIAG]   (no valid certificate)\n");
    } else {
        printf("[DIAG]   (no certificate found)\n");
    }
    unlink(temp_cert);
    printf("\n");

    /* --- Section 4: Monotonic Counters --- */
    printf("[DIAG] --- Monotonic Counters ---\n");
    static const uint16_t counter_oids[] = {0xE120, 0xE121, 0xE122, 0xE123};
    static const char *counter_labels[] = {
        "Anti-Replay ", "FW Version  ", "Usage Meter ", "Boot Counter"
    };

    for (int i = 0; i < 4; i++) {
        snprintf(cmd, sizeof(cmd),
                 "%s/trustm_data -r 0x%04X 2>&1", tools_path, counter_oids[i]);
        pipe = popen(cmd, "r");
        if (pipe) {
            out_len = fread(output, 1, sizeof(output) - 1, pipe);
            output[out_len] = '\0';
            pclose(pipe);
            char *size_line = strstr(output, "[Size ");
            if (size_line) {
                unsigned int sz = 0;
                sscanf(size_line, "[Size %04X]", &sz);
                if (sz >= 8) {
                    char *hex_start = strchr(size_line, '\n');
                    if (hex_start) {
                        hex_start++;
                        unsigned int b[8] = {0};
                        if (sscanf(hex_start, " %02X %02X %02X %02X %02X %02X %02X %02X",
                                   &b[0], &b[1], &b[2], &b[3],
                                   &b[4], &b[5], &b[6], &b[7]) == 8) {
                            uint32_t counter_val = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
                            uint32_t threshold = (b[4] << 24) | (b[5] << 16) | (b[6] << 8) | b[7];
                            printf("[DIAG] 0x%04X (%s): value=%u, threshold=%u\n",
                                   counter_oids[i], counter_labels[i], counter_val, threshold);
                        } else {
                            printf("[DIAG] 0x%04X (%s): (parse error)\n",
                                   counter_oids[i], counter_labels[i]);
                        }
                    }
                } else {
                    printf("[DIAG] 0x%04X (%s): (empty or < 8 bytes)\n",
                           counter_oids[i], counter_labels[i]);
                }
            } else {
                printf("[DIAG] 0x%04X (%s): (read error)\n",
                       counter_oids[i], counter_labels[i]);
            }
        }
    }

    printf("\n[DIAG] ===========================================\n");
    printf("[DIAG] Diagnostics complete\n");
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

    /* Diag uses external tools — runs WITHOUT library init to avoid I2C conflict */
    if (strcmp(cmd, "diag") == 0 || strcmp(cmd, "metadata") == 0) {
        exit_code = cmd_run_diagnostics() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        return exit_code;
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
