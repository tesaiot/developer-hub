/**
 * TESA IoT Platform - MQTT over QUIC Client Example (C/C++)
 * Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)
 *
 * This example demonstrates connecting to the TESA IoT Platform
 * using MQTT over QUIC with Server-TLS authentication.
 *
 * Requirements:
 * - NanoSDK library (https://github.com/nanomq/NanoSDK)
 * - CA certificate chain (ca-chain.pem from bundle)
 * - Device credentials (username/password)
 *
 * Build:
 *   mkdir build && cd build
 *   cmake ..
 *   make
 *
 * Run:
 *   ./mqtt_quic_client <device_id> <password>
 *
 * License: Apache 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

// NanoSDK headers (adjust path based on installation)
#include "nng/nng.h"
#include "nng/mqtt/mqtt_client.h"
#include "nng/mqtt/mqtt_quic.h"
#include "nng/supplemental/tls/tls.h"
#include "nng/supplemental/util/platform.h"

// Connection configuration
#define MQTT_BROKER_HOST    "tesaiot.com"
#define MQTT_BROKER_PORT    14567
#define MQTT_QOS            1
#define MQTT_KEEPALIVE      60
#define CA_CERT_PATH        "ca-chain.pem"
#define TELEMETRY_TOPIC     "devices/%s/telemetry"

// Global variables
static nng_socket   sock;
static nng_dialer   dialer;
static volatile int running = 1;

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int sig) {
    printf("\n[INFO] Received signal %d, shutting down...\n", sig);
    running = 0;
}

/**
 * Connect callback - called when connection established
 */
void connect_cb(nng_pipe p, nng_pipe_ev ev, void *arg) {
    if (ev == NNG_PIPE_EV_ADD_POST) {
        printf("[INFO] ✅ Connected to MQTT broker via QUIC\n");
    } else if (ev == NNG_PIPE_EV_REM_POST) {
        printf("[WARNING] ⚠️  Disconnected from MQTT broker\n");
    }
}

/**
 * Message callback - called when message received
 */
void message_cb(nng_msg *msg, void *arg) {
    (void)arg;  // Unused parameter
    uint32_t topic_len;
    const char *topic = nng_mqtt_msg_get_publish_topic(msg, &topic_len);
    uint32_t payload_len;
    uint8_t *payload = nng_mqtt_msg_get_publish_payload(msg, &payload_len);

    printf("[INFO] 📩 Received message on topic: %.*s\n", (int)topic_len, topic);
    printf("[INFO]    Payload: %.*s\n", (int)payload_len, payload);

    nng_msg_free(msg);
}

/**
 * Load CA certificate for server verification
 */
int load_ca_cert(nng_tls_config *tls_cfg) {
    FILE *fp = fopen(CA_CERT_PATH, "rb");
    if (!fp) {
        fprintf(stderr, "[ERROR] ❌ Failed to open CA certificate: %s\n", CA_CERT_PATH);
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read certificate
    char *ca_cert = (char *)malloc(size + 1);
    if (!ca_cert) {
        fclose(fp);
        fprintf(stderr, "[ERROR] ❌ Memory allocation failed\n");
        return -1;
    }

    size_t read = fread(ca_cert, 1, size, fp);
    fclose(fp);
    ca_cert[read] = '\0';

    // Set CA certificate for server verification
    int rv = nng_tls_config_ca_chain(tls_cfg, ca_cert, NULL);
    free(ca_cert);

    if (rv != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to set CA certificate: %s\n", nng_strerror(rv));
        return -1;
    }

    printf("[INFO] ✅ CA certificate loaded successfully\n");
    return 0;
}

/**
 * Connect to MQTT broker using QUIC
 */
int mqtt_connect(const char *device_id, const char *password) {
    int rv;

    // Create MQTT client socket
    if ((rv = nng_mqtt_quic_client_open(&sock)) != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to create MQTT QUIC socket: %s\n", nng_strerror(rv));
        return -1;
    }

    // Configure TLS for QUIC (TLS 1.3 is mandatory)
    nng_tls_config *tls_cfg;
    if ((rv = nng_tls_config_alloc(&tls_cfg, NNG_TLS_MODE_CLIENT)) != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to allocate TLS config: %s\n", nng_strerror(rv));
        nng_close(sock);
        return -1;
    }

    // Set TLS 1.3 (QUIC requires TLS 1.3)
    nng_tls_config_version(tls_cfg, NNG_TLS_1_3, NNG_TLS_1_3);

    // Load and set CA certificate for server verification
    if (load_ca_cert(tls_cfg) != 0) {
        nng_tls_config_free(tls_cfg);
        nng_close(sock);
        return -1;
    }

    // Enable server certificate verification
    nng_tls_config_auth_mode(tls_cfg, NNG_TLS_AUTH_MODE_REQUIRED);

    // Set server name for SNI (Server Name Indication)
    nng_tls_config_server_name(tls_cfg, MQTT_BROKER_HOST);

    // Apply TLS config to socket
    nng_socket_set_ptr(sock, NNG_OPT_TLS_CONFIG, tls_cfg);

    // Set pipe callbacks for connection events
    nng_pipe_notify(sock, NNG_PIPE_EV_ADD_POST, connect_cb, NULL);
    nng_pipe_notify(sock, NNG_PIPE_EV_REM_POST, connect_cb, NULL);

    // Create CONNECT message
    nng_msg *connmsg;
    nng_mqtt_msg_alloc(&connmsg, 0);
    nng_mqtt_msg_set_packet_type(connmsg, NNG_MQTT_CONNECT);

    // Set connection properties
    nng_mqtt_msg_set_connect_client_id(connmsg, device_id);
    nng_mqtt_msg_set_connect_user_name(connmsg, device_id);
    nng_mqtt_msg_set_connect_password(connmsg, password);
    nng_mqtt_msg_set_connect_keep_alive(connmsg, MQTT_KEEPALIVE);
    nng_mqtt_msg_set_connect_clean_session(connmsg, true);

    printf("[INFO] 🔌 Connecting to mqtts://%s:%d via QUIC...\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    printf("[INFO]    Username: %s\n", device_id);
    printf("[INFO]    Transport: QUIC (UDP)\n");
    printf("[INFO]    TLS Version: 1.3\n");

    // Build connection URL
    char url[256];
    snprintf(url, sizeof(url), "mqtt-quic://%s:%d", MQTT_BROKER_HOST, MQTT_BROKER_PORT);

    // Create dialer
    if ((rv = nng_dialer_create(&dialer, sock, url)) != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to create dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(tls_cfg);
        nng_close(sock);
        return -1;
    }

    // Start connection
    if ((rv = nng_dialer_start(dialer, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to start dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(tls_cfg);
        nng_close(sock);
        return -1;
    }

    // Send CONNECT message
    if ((rv = nng_sendmsg(sock, connmsg, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to send CONNECT: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(tls_cfg);
        nng_close(sock);
        return -1;
    }

    // Wait for CONNACK (connection acknowledgment)
    nng_msg *ack;
    int retry = 0;
    while (retry < 50) {  // 5 seconds timeout (50 x 100ms)
        rv = nng_recvmsg(sock, &ack, NNG_FLAG_NONBLOCK);
        if (rv == 0) {
            // Check if it's CONNACK
            if (nng_mqtt_msg_get_packet_type(ack) == NNG_MQTT_CONNACK) {
                uint8_t return_code = nng_mqtt_msg_get_connack_return_code(ack);
                nng_msg_free(ack);

                if (return_code == 0) {
                    printf("[INFO] ✅ Successfully connected and authenticated\n");
                    return 0;
                } else {
                    fprintf(stderr, "[ERROR] ❌ Connection rejected with code: %d\n", return_code);
                    nng_tls_config_free(tls_cfg);
                    nng_close(sock);
                    return -1;
                }
            }
            nng_msg_free(ack);
        }
        nng_msleep(100);
        retry++;
    }

    fprintf(stderr, "[ERROR] ❌ Connection timeout - no CONNACK received\n");
    nng_tls_config_free(tls_cfg);
    nng_close(sock);
    return -1;
}

/**
 * Publish telemetry data
 */
int publish_telemetry(const char *device_id, double temperature, double humidity) {
    char topic[128];
    snprintf(topic, sizeof(topic), TELEMETRY_TOPIC, device_id);

    // Build JSON payload
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"timestamp\":\"%s\",\"data\":{\"temperature\":%.2f,\"humidity\":%.2f}}",
             timestamp, temperature, humidity);

    // Create PUBLISH message
    nng_msg *pubmsg;
    nng_mqtt_msg_alloc(&pubmsg, 0);
    nng_mqtt_msg_set_packet_type(pubmsg, NNG_MQTT_PUBLISH);
    nng_mqtt_msg_set_publish_topic(pubmsg, topic);
    nng_mqtt_msg_set_publish_payload(pubmsg, (uint8_t *)payload, strlen(payload));
    nng_mqtt_msg_set_publish_qos(pubmsg, MQTT_QOS);

    printf("[INFO] 📤 Publishing telemetry to %s\n", topic);
    printf("[INFO]    Payload: %s\n", payload);

    int rv = nng_sendmsg(sock, pubmsg, 0);
    if (rv != 0) {
        fprintf(stderr, "[ERROR] ❌ Failed to publish: %s\n", nng_strerror(rv));
        nng_msg_free(pubmsg);
        return -1;
    }

    return 0;
}

/**
 * Main function
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device_id> <password>\n", argv[0]);
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123\n", argv[0]);
        return 1;
    }

    const char *device_id = argv[1];
    const char *password = argv[2];

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  TESA IoT Platform - MQTT over QUIC Client\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    // Connect to broker
    if (mqtt_connect(device_id, password) != 0) {
        return 1;
    }

    printf("\n[INFO] 🚀 Starting telemetry loop (Ctrl+C to stop)...\n\n");

    // Main loop - publish telemetry every 10 seconds
    int count = 0;
    while (running) {
        // Simulate sensor readings
        double temperature = 20.0 + (rand() % 100) / 10.0;  // 20.0 - 30.0°C
        double humidity = 40.0 + (rand() % 300) / 10.0;     // 40.0 - 70.0%

        if (publish_telemetry(device_id, temperature, humidity) == 0) {
            count++;
            printf("[INFO] ✅ Telemetry #%d sent successfully\n\n", count);
        }

        // Sleep for 10 seconds
        for (int i = 0; i < 100 && running; i++) {
            nng_msleep(100);
        }
    }

    // Cleanup
    printf("\n[INFO] 🛑 Disconnecting...\n");
    nng_dialer_close(dialer);
    nng_close(sock);

    printf("[INFO] ✅ Shutdown complete\n");
    return 0;
}
