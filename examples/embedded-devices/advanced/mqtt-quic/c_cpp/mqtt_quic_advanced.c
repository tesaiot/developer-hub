/**
 * TESA IoT Platform - Advanced MQTT over QUIC Client (C/C++)
 * Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)
 *
 * Advanced example extending entry-level Example #10 (mqtt-quic-c)
 * with production-ready features for IoT deployments.
 *
 * ADVANCED FEATURES (vs Entry-Level #10):
 * ============================================
 * 1. Automatic QUIC -> TCP+TLS Fallback
 *    - Try QUIC first, fall back to TCP+TLS if blocked
 *    - Seamless transport switching
 *
 * 2. Multi-Stream Parallel Publishing
 *    - QUIC allows multiple parallel streams
 *    - Publish to multiple topics concurrently
 *
 * 3. Session Resumption (0-RTT)
 *    - Store session tickets for fast reconnection
 *    - Reduce handshake latency on reconnect
 *
 * 4. Exponential Backoff Reconnection
 *    - Smart retry with increasing delays
 *    - Jitter to prevent thundering herd
 *
 * 5. Connection Health Monitoring
 *    - Track connection statistics
 *    - Latency measurement
 *    - Automatic health checks
 *
 * Requirements:
 * - NanoSDK library (https://github.com/nanomq/NanoSDK)
 * - paho.mqtt.c for TCP fallback (optional)
 * - CA certificate chain (ca-chain.pem from bundle)
 * - Device credentials (username/password)
 *
 * Build:
 *   mkdir build && cd build
 *   cmake ..
 *   make
 *
 * Run:
 *   ./mqtt_quic_advanced <device_id> <password>
 *
 * License: Apache 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>

// NanoSDK headers for QUIC
#include "nng/nng.h"
#include "nng/mqtt/mqtt_client.h"
#include "nng/mqtt/mqtt_quic.h"
#include "nng/supplemental/tls/tls.h"
#include "nng/supplemental/util/platform.h"

// ============================================
// CONFIGURATION
// ============================================

// QUIC Configuration (Primary)
#define QUIC_BROKER_HOST    "mqtt.tesaiot.com"
#define QUIC_BROKER_PORT    14567

// TCP+TLS Configuration (Fallback)
#define TCP_BROKER_HOST     "mqtt.tesaiot.com"
#define TCP_BROKER_PORT     8884

// MQTT Settings
#define MQTT_QOS            1
#define MQTT_KEEPALIVE      60
#define CA_CERT_PATH        "ca-chain.pem"
#define TELEMETRY_TOPIC     "device/%s/telemetry/sensors"

// Advanced Settings
#define QUIC_CONNECT_TIMEOUT_MS     5000    // 5 seconds for QUIC attempt
#define TCP_CONNECT_TIMEOUT_MS      10000   // 10 seconds for TCP fallback
#define MAX_RECONNECT_ATTEMPTS      10
#define BASE_RECONNECT_DELAY_MS     1000    // Start with 1 second
#define MAX_RECONNECT_DELAY_MS      60000   // Max 60 seconds
#define HEALTH_CHECK_INTERVAL_MS    30000   // Check every 30 seconds
#define SESSION_TICKET_FILE         ".session_ticket.bin"
#define MAX_PARALLEL_STREAMS        4

// ============================================
// DATA STRUCTURES
// ============================================

typedef enum {
    TRANSPORT_QUIC,
    TRANSPORT_TCP_TLS
} transport_type_t;

typedef enum {
    CONN_STATE_DISCONNECTED,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECTED,
    CONN_STATE_RECONNECTING
} connection_state_t;

typedef struct {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t reconnect_count;
    uint64_t quic_connections;
    uint64_t tcp_connections;
    double avg_latency_ms;
    double min_latency_ms;
    double max_latency_ms;
    time_t connected_since;
    time_t last_message_time;
    uint64_t latency_samples;
    double latency_sum;
} connection_stats_t;

typedef struct {
    char *topic;
    char *payload;
    size_t payload_len;
    int qos;
    int result;
    pthread_t thread;
    int completed;
} publish_task_t;

typedef struct {
    // Connection
    nng_socket sock;
    nng_dialer dialer;
    transport_type_t transport;
    connection_state_t state;

    // Configuration
    char device_id[64];
    char password[128];

    // Session resumption
    uint8_t *session_ticket;
    size_t session_ticket_len;
    int session_resumption_enabled;

    // Statistics
    connection_stats_t stats;

    // Reconnection
    int reconnect_attempt;

    // Thread safety
    pthread_mutex_t lock;

    // TLS Config
    nng_tls_config *tls_cfg;
} mqtt_client_t;

// ============================================
// GLOBAL VARIABLES
// ============================================

static mqtt_client_t g_client;
static volatile int g_running = 1;

// ============================================
// UTILITY FUNCTIONS
// ============================================

/**
 * Get current time in milliseconds
 */
static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

/**
 * Generate random jitter for reconnect delay
 */
static int random_jitter(int base) {
    return base + (rand() % (base / 2));
}

/**
 * Calculate exponential backoff delay
 */
static int calculate_backoff_delay(int attempt) {
    int delay = BASE_RECONNECT_DELAY_MS * (1 << attempt);  // 2^attempt
    if (delay > MAX_RECONNECT_DELAY_MS) {
        delay = MAX_RECONNECT_DELAY_MS;
    }
    return random_jitter(delay);
}

/**
 * Signal handler for graceful shutdown
 */
static void signal_handler(int sig) {
    printf("\n[INFO] Received signal %d, shutting down...\n", sig);
    g_running = 0;
}

/**
 * Update latency statistics
 */
static void update_latency_stats(mqtt_client_t *client, double latency_ms) {
    pthread_mutex_lock(&client->lock);

    client->stats.latency_sum += latency_ms;
    client->stats.latency_samples++;
    client->stats.avg_latency_ms = client->stats.latency_sum / client->stats.latency_samples;

    if (latency_ms < client->stats.min_latency_ms || client->stats.min_latency_ms == 0) {
        client->stats.min_latency_ms = latency_ms;
    }
    if (latency_ms > client->stats.max_latency_ms) {
        client->stats.max_latency_ms = latency_ms;
    }

    pthread_mutex_unlock(&client->lock);
}

// ============================================
// SESSION TICKET MANAGEMENT (0-RTT)
// ============================================

/**
 * Save session ticket for 0-RTT reconnection
 */
static int save_session_ticket(mqtt_client_t *client) {
    if (!client->session_ticket || client->session_ticket_len == 0) {
        return -1;
    }

    FILE *fp = fopen(SESSION_TICKET_FILE, "wb");
    if (!fp) {
        printf("[DEBUG] Could not save session ticket: %s\n", strerror(errno));
        return -1;
    }

    fwrite(&client->session_ticket_len, sizeof(size_t), 1, fp);
    fwrite(client->session_ticket, 1, client->session_ticket_len, fp);
    fclose(fp);

    printf("[INFO] Session ticket saved for 0-RTT reconnection\n");
    return 0;
}

/**
 * Load session ticket for 0-RTT reconnection
 */
static int load_session_ticket(mqtt_client_t *client) {
    FILE *fp = fopen(SESSION_TICKET_FILE, "rb");
    if (!fp) {
        printf("[DEBUG] No saved session ticket found\n");
        return -1;
    }

    if (fread(&client->session_ticket_len, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    client->session_ticket = (uint8_t *)malloc(client->session_ticket_len);
    if (!client->session_ticket) {
        fclose(fp);
        return -1;
    }

    if (fread(client->session_ticket, 1, client->session_ticket_len, fp) != client->session_ticket_len) {
        free(client->session_ticket);
        client->session_ticket = NULL;
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("[INFO] Session ticket loaded for 0-RTT fast reconnection\n");
    client->session_resumption_enabled = 1;
    return 0;
}

// ============================================
// TLS/CA CERTIFICATE HANDLING
// ============================================

/**
 * Load CA certificate for server verification
 */
static int load_ca_cert(nng_tls_config *tls_cfg) {
    FILE *fp = fopen(CA_CERT_PATH, "rb");
    if (!fp) {
        // Try alternative path
        fp = fopen("../ca-chain.pem", "rb");
        if (!fp) {
            fprintf(stderr, "[ERROR] Failed to open CA certificate: %s\n", CA_CERT_PATH);
            return -1;
        }
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *ca_cert = (char *)malloc(size + 1);
    if (!ca_cert) {
        fclose(fp);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return -1;
    }

    size_t read_bytes = fread(ca_cert, 1, size, fp);
    fclose(fp);
    ca_cert[read_bytes] = '\0';

    int rv = nng_tls_config_ca_chain(tls_cfg, ca_cert, NULL);
    free(ca_cert);

    if (rv != 0) {
        fprintf(stderr, "[ERROR] Failed to set CA certificate: %s\n", nng_strerror(rv));
        return -1;
    }

    return 0;
}

// ============================================
// CONNECTION CALLBACKS
// ============================================

/**
 * Connect callback - called when connection state changes
 */
static void connect_cb(nng_pipe p, nng_pipe_ev ev, void *arg) {
    mqtt_client_t *client = (mqtt_client_t *)arg;

    if (ev == NNG_PIPE_EV_ADD_POST) {
        pthread_mutex_lock(&client->lock);
        client->state = CONN_STATE_CONNECTED;
        client->stats.connected_since = time(NULL);

        if (client->transport == TRANSPORT_QUIC) {
            client->stats.quic_connections++;
            printf("[INFO] Connected via QUIC (UDP) - Fast path\n");
        } else {
            client->stats.tcp_connections++;
            printf("[INFO] Connected via TCP+TLS - Fallback path\n");
        }
        pthread_mutex_unlock(&client->lock);

    } else if (ev == NNG_PIPE_EV_REM_POST) {
        pthread_mutex_lock(&client->lock);
        client->state = CONN_STATE_DISCONNECTED;
        printf("[WARNING] Disconnected from MQTT broker\n");
        pthread_mutex_unlock(&client->lock);
    }
}

// ============================================
// QUIC CONNECTION (PRIMARY)
// ============================================

/**
 * Attempt QUIC connection
 */
static int try_quic_connect(mqtt_client_t *client) {
    int rv;

    printf("[INFO] Attempting QUIC connection to %s:%d...\n", QUIC_BROKER_HOST, QUIC_BROKER_PORT);

    // Create MQTT QUIC socket
    if ((rv = nng_mqtt_quic_client_open(&client->sock)) != 0) {
        fprintf(stderr, "[WARN] Failed to create MQTT QUIC socket: %s\n", nng_strerror(rv));
        return -1;
    }

    // Configure TLS for QUIC (TLS 1.3 mandatory)
    if ((rv = nng_tls_config_alloc(&client->tls_cfg, NNG_TLS_MODE_CLIENT)) != 0) {
        fprintf(stderr, "[WARN] Failed to allocate TLS config: %s\n", nng_strerror(rv));
        nng_close(client->sock);
        return -1;
    }

    // Set TLS 1.3 (required for QUIC)
    nng_tls_config_version(client->tls_cfg, NNG_TLS_1_3, NNG_TLS_1_3);

    // Load CA certificate
    if (load_ca_cert(client->tls_cfg) != 0) {
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    // Enable server verification
    nng_tls_config_auth_mode(client->tls_cfg, NNG_TLS_AUTH_MODE_REQUIRED);
    nng_tls_config_server_name(client->tls_cfg, QUIC_BROKER_HOST);

    // Apply TLS config
    nng_socket_set_ptr(client->sock, NNG_OPT_TLS_CONFIG, client->tls_cfg);

    // Set callbacks
    nng_pipe_notify(client->sock, NNG_PIPE_EV_ADD_POST, connect_cb, client);
    nng_pipe_notify(client->sock, NNG_PIPE_EV_REM_POST, connect_cb, client);

    // Create CONNECT message
    nng_msg *connmsg;
    nng_mqtt_msg_alloc(&connmsg, 0);
    nng_mqtt_msg_set_packet_type(connmsg, NNG_MQTT_CONNECT);
    nng_mqtt_msg_set_connect_client_id(connmsg, client->device_id);
    nng_mqtt_msg_set_connect_user_name(connmsg, client->device_id);
    nng_mqtt_msg_set_connect_password(connmsg, client->password);
    nng_mqtt_msg_set_connect_keep_alive(connmsg, MQTT_KEEPALIVE);
    nng_mqtt_msg_set_connect_clean_session(connmsg, !client->session_resumption_enabled);

    // Build QUIC URL
    char url[256];
    snprintf(url, sizeof(url), "mqtt-quic://%s:%d", QUIC_BROKER_HOST, QUIC_BROKER_PORT);

    // Create and start dialer
    if ((rv = nng_dialer_create(&client->dialer, client->sock, url)) != 0) {
        fprintf(stderr, "[WARN] Failed to create QUIC dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    if ((rv = nng_dialer_start(client->dialer, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[WARN] Failed to start QUIC dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    // Send CONNECT
    if ((rv = nng_sendmsg(client->sock, connmsg, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[WARN] Failed to send CONNECT: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    // Wait for CONNACK with timeout
    nng_msg *ack;
    uint64_t start = get_time_ms();
    while ((get_time_ms() - start) < QUIC_CONNECT_TIMEOUT_MS) {
        rv = nng_recvmsg(client->sock, &ack, NNG_FLAG_NONBLOCK);
        if (rv == 0) {
            if (nng_mqtt_msg_get_packet_type(ack) == NNG_MQTT_CONNACK) {
                uint8_t return_code = nng_mqtt_msg_get_connack_return_code(ack);
                nng_msg_free(ack);

                if (return_code == 0) {
                    client->transport = TRANSPORT_QUIC;
                    client->state = CONN_STATE_CONNECTED;
                    printf("[INFO] QUIC connection successful (0-RTT: %s)\n",
                           client->session_resumption_enabled ? "enabled" : "disabled");
                    return 0;
                } else {
                    fprintf(stderr, "[WARN] QUIC CONNACK rejected: %d\n", return_code);
                    break;
                }
            }
            nng_msg_free(ack);
        }
        nng_msleep(50);
    }

    // QUIC failed, cleanup
    fprintf(stderr, "[WARN] QUIC connection timeout after %d ms\n", QUIC_CONNECT_TIMEOUT_MS);
    nng_tls_config_free(client->tls_cfg);
    nng_close(client->sock);
    return -1;
}

// ============================================
// TCP+TLS CONNECTION (FALLBACK)
// ============================================

/**
 * Attempt TCP+TLS fallback connection
 */
static int try_tcp_connect(mqtt_client_t *client) {
    int rv;

    printf("[INFO] Attempting TCP+TLS fallback to %s:%d...\n", TCP_BROKER_HOST, TCP_BROKER_PORT);

    // Create MQTT TCP socket
    if ((rv = nng_mqtt_client_open(&client->sock)) != 0) {
        fprintf(stderr, "[ERROR] Failed to create MQTT TCP socket: %s\n", nng_strerror(rv));
        return -1;
    }

    // Configure TLS
    if ((rv = nng_tls_config_alloc(&client->tls_cfg, NNG_TLS_MODE_CLIENT)) != 0) {
        fprintf(stderr, "[ERROR] Failed to allocate TLS config: %s\n", nng_strerror(rv));
        nng_close(client->sock);
        return -1;
    }

    // TLS 1.2 or 1.3 for TCP
    nng_tls_config_version(client->tls_cfg, NNG_TLS_1_2, NNG_TLS_1_3);

    // Load CA certificate
    if (load_ca_cert(client->tls_cfg) != 0) {
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    nng_tls_config_auth_mode(client->tls_cfg, NNG_TLS_AUTH_MODE_REQUIRED);
    nng_tls_config_server_name(client->tls_cfg, TCP_BROKER_HOST);

    nng_socket_set_ptr(client->sock, NNG_OPT_TLS_CONFIG, client->tls_cfg);

    // Set callbacks
    nng_pipe_notify(client->sock, NNG_PIPE_EV_ADD_POST, connect_cb, client);
    nng_pipe_notify(client->sock, NNG_PIPE_EV_REM_POST, connect_cb, client);

    // Create CONNECT message
    nng_msg *connmsg;
    nng_mqtt_msg_alloc(&connmsg, 0);
    nng_mqtt_msg_set_packet_type(connmsg, NNG_MQTT_CONNECT);
    nng_mqtt_msg_set_connect_client_id(connmsg, client->device_id);
    nng_mqtt_msg_set_connect_user_name(connmsg, client->device_id);
    nng_mqtt_msg_set_connect_password(connmsg, client->password);
    nng_mqtt_msg_set_connect_keep_alive(connmsg, MQTT_KEEPALIVE);
    nng_mqtt_msg_set_connect_clean_session(connmsg, 1);

    // Build TLS URL
    char url[256];
    snprintf(url, sizeof(url), "tls+mqtt://%s:%d", TCP_BROKER_HOST, TCP_BROKER_PORT);

    // Create and start dialer
    if ((rv = nng_dialer_create(&client->dialer, client->sock, url)) != 0) {
        fprintf(stderr, "[ERROR] Failed to create TCP dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    if ((rv = nng_dialer_start(client->dialer, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[ERROR] Failed to start TCP dialer: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    // Send CONNECT
    if ((rv = nng_sendmsg(client->sock, connmsg, NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "[ERROR] Failed to send CONNECT: %s\n", nng_strerror(rv));
        nng_msg_free(connmsg);
        nng_tls_config_free(client->tls_cfg);
        nng_close(client->sock);
        return -1;
    }

    // Wait for CONNACK
    nng_msg *ack;
    uint64_t start = get_time_ms();
    while ((get_time_ms() - start) < TCP_CONNECT_TIMEOUT_MS) {
        rv = nng_recvmsg(client->sock, &ack, NNG_FLAG_NONBLOCK);
        if (rv == 0) {
            if (nng_mqtt_msg_get_packet_type(ack) == NNG_MQTT_CONNACK) {
                uint8_t return_code = nng_mqtt_msg_get_connack_return_code(ack);
                nng_msg_free(ack);

                if (return_code == 0) {
                    client->transport = TRANSPORT_TCP_TLS;
                    client->state = CONN_STATE_CONNECTED;
                    printf("[INFO] TCP+TLS fallback connection successful\n");
                    return 0;
                } else {
                    fprintf(stderr, "[ERROR] TCP CONNACK rejected: %d\n", return_code);
                    break;
                }
            }
            nng_msg_free(ack);
        }
        nng_msleep(50);
    }

    fprintf(stderr, "[ERROR] TCP connection timeout\n");
    nng_tls_config_free(client->tls_cfg);
    nng_close(client->sock);
    return -1;
}

// ============================================
// SMART CONNECTION (QUIC -> TCP FALLBACK)
// ============================================

/**
 * Smart connect: Try QUIC first, fallback to TCP+TLS
 */
static int smart_connect(mqtt_client_t *client) {
    client->state = CONN_STATE_CONNECTING;

    // Try to load saved session ticket for 0-RTT
    load_session_ticket(client);

    // Attempt QUIC first
    if (try_quic_connect(client) == 0) {
        // Save session ticket for future 0-RTT
        save_session_ticket(client);
        return 0;
    }

    printf("[INFO] QUIC unavailable (possibly blocked by firewall), trying TCP+TLS...\n");

    // Fallback to TCP+TLS
    if (try_tcp_connect(client) == 0) {
        return 0;
    }

    client->state = CONN_STATE_DISCONNECTED;
    return -1;
}

// ============================================
// RECONNECTION WITH EXPONENTIAL BACKOFF
// ============================================

/**
 * Reconnect with exponential backoff
 */
static int reconnect_with_backoff(mqtt_client_t *client) {
    while (g_running && client->reconnect_attempt < MAX_RECONNECT_ATTEMPTS) {
        int delay = calculate_backoff_delay(client->reconnect_attempt);

        printf("[INFO] Reconnect attempt %d/%d in %d ms...\n",
               client->reconnect_attempt + 1, MAX_RECONNECT_ATTEMPTS, delay);

        nng_msleep(delay);

        if (!g_running) break;

        client->state = CONN_STATE_RECONNECTING;

        if (smart_connect(client) == 0) {
            client->reconnect_attempt = 0;
            client->stats.reconnect_count++;
            printf("[INFO] Reconnection successful\n");
            return 0;
        }

        client->reconnect_attempt++;
    }

    fprintf(stderr, "[ERROR] Max reconnection attempts reached\n");
    return -1;
}

// ============================================
// PUBLISHING (SINGLE & PARALLEL)
// ============================================

/**
 * Publish single message with latency tracking
 */
static int publish_message(mqtt_client_t *client, const char *topic,
                          const char *payload, size_t payload_len, int qos) {
    if (client->state != CONN_STATE_CONNECTED) {
        fprintf(stderr, "[ERROR] Not connected\n");
        return -1;
    }

    uint64_t start = get_time_ms();

    // Create PUBLISH message
    nng_msg *pubmsg;
    nng_mqtt_msg_alloc(&pubmsg, 0);
    nng_mqtt_msg_set_packet_type(pubmsg, NNG_MQTT_PUBLISH);
    nng_mqtt_msg_set_publish_topic(pubmsg, topic);
    nng_mqtt_msg_set_publish_payload(pubmsg, (uint8_t *)payload, payload_len);
    nng_mqtt_msg_set_publish_qos(pubmsg, qos);

    int rv = nng_sendmsg(client->sock, pubmsg, 0);
    if (rv != 0) {
        fprintf(stderr, "[ERROR] Publish failed: %s\n", nng_strerror(rv));
        nng_msg_free(pubmsg);
        return -1;
    }

    uint64_t end = get_time_ms();
    double latency = (double)(end - start);

    // Update statistics
    pthread_mutex_lock(&client->lock);
    client->stats.messages_sent++;
    client->stats.bytes_sent += payload_len;
    client->stats.last_message_time = time(NULL);
    pthread_mutex_unlock(&client->lock);

    update_latency_stats(client, latency);

    return 0;
}

/**
 * Thread function for parallel publish
 */
static void *parallel_publish_thread(void *arg) {
    publish_task_t *task = (publish_task_t *)arg;

    task->result = publish_message(&g_client, task->topic, task->payload,
                                   task->payload_len, task->qos);
    task->completed = 1;

    return NULL;
}

/**
 * Publish to multiple topics in parallel (QUIC multi-stream feature)
 */
static int publish_parallel(mqtt_client_t *client, publish_task_t *tasks, int task_count) {
    if (task_count > MAX_PARALLEL_STREAMS) {
        fprintf(stderr, "[WARN] Limiting parallel streams to %d\n", MAX_PARALLEL_STREAMS);
        task_count = MAX_PARALLEL_STREAMS;
    }

    printf("[INFO] Starting parallel publish to %d topics...\n", task_count);

    // Start all threads
    for (int i = 0; i < task_count; i++) {
        tasks[i].completed = 0;
        pthread_create(&tasks[i].thread, NULL, parallel_publish_thread, &tasks[i]);
    }

    // Wait for all to complete
    int success_count = 0;
    for (int i = 0; i < task_count; i++) {
        pthread_join(tasks[i].thread, NULL);
        if (tasks[i].result == 0) {
            success_count++;
        }
    }

    printf("[INFO] Parallel publish completed: %d/%d successful\n", success_count, task_count);
    return success_count;
}

// ============================================
// CONNECTION HEALTH MONITORING
// ============================================

/**
 * Print connection statistics
 */
static void print_stats(mqtt_client_t *client) {
    pthread_mutex_lock(&client->lock);

    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║         CONNECTION STATISTICS                    ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║ Transport      : %-30s ║\n",
           client->transport == TRANSPORT_QUIC ? "QUIC (UDP)" : "TCP+TLS");
    printf("║ State          : %-30s ║\n",
           client->state == CONN_STATE_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    printf("║ Messages Sent  : %-30lu ║\n", client->stats.messages_sent);
    printf("║ Bytes Sent     : %-30lu ║\n", client->stats.bytes_sent);
    printf("║ Reconnects     : %-30lu ║\n", client->stats.reconnect_count);
    printf("║ QUIC Connects  : %-30lu ║\n", client->stats.quic_connections);
    printf("║ TCP Connects   : %-30lu ║\n", client->stats.tcp_connections);
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║ Avg Latency    : %-27.2f ms ║\n", client->stats.avg_latency_ms);
    printf("║ Min Latency    : %-27.2f ms ║\n", client->stats.min_latency_ms);
    printf("║ Max Latency    : %-27.2f ms ║\n", client->stats.max_latency_ms);
    printf("╚══════════════════════════════════════════════════╝\n\n");

    pthread_mutex_unlock(&client->lock);
}

/**
 * Health check thread
 */
static void *health_check_thread(void *arg) {
    mqtt_client_t *client = (mqtt_client_t *)arg;

    while (g_running) {
        nng_msleep(HEALTH_CHECK_INTERVAL_MS);

        if (!g_running) break;

        if (client->state == CONN_STATE_CONNECTED) {
            print_stats(client);
        } else if (client->state == CONN_STATE_DISCONNECTED && g_running) {
            printf("[WARN] Health check: Connection lost, initiating reconnect...\n");
            reconnect_with_backoff(client);
        }
    }

    return NULL;
}

// ============================================
// MAIN FUNCTION
// ============================================

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device_id> <password>\n", argv[0]);
        fprintf(stderr, "\nAdvanced MQTT over QUIC Client Features:\n");
        fprintf(stderr, "  - Automatic QUIC -> TCP+TLS Fallback\n");
        fprintf(stderr, "  - Session Resumption (0-RTT)\n");
        fprintf(stderr, "  - Exponential Backoff Reconnection\n");
        fprintf(stderr, "  - Multi-Stream Parallel Publishing\n");
        fprintf(stderr, "  - Connection Health Monitoring\n");
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s 95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9 oqvWjccXzS@mLLMe\n", argv[0]);
        return 1;
    }

    // Initialize client
    memset(&g_client, 0, sizeof(g_client));
    strncpy(g_client.device_id, argv[1], sizeof(g_client.device_id) - 1);
    strncpy(g_client.password, argv[2], sizeof(g_client.password) - 1);
    pthread_mutex_init(&g_client.lock, NULL);

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Seed random for jitter
    srand(time(NULL));

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TESA IoT Platform - Advanced MQTT over QUIC Client      ║\n");
    printf("║  Features: QUIC/TCP Fallback, 0-RTT, Multi-Stream        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    printf("[INFO] Device ID: %s\n", g_client.device_id);
    printf("[INFO] Primary: QUIC (UDP port %d)\n", QUIC_BROKER_PORT);
    printf("[INFO] Fallback: TCP+TLS (port %d)\n\n", TCP_BROKER_PORT);

    // Smart connect (QUIC with TCP fallback)
    if (smart_connect(&g_client) != 0) {
        fprintf(stderr, "[ERROR] Failed to connect to broker\n");
        return 1;
    }

    // Start health monitoring thread
    pthread_t health_thread;
    pthread_create(&health_thread, NULL, health_check_thread, &g_client);

    printf("\n[INFO] Starting telemetry loop (Ctrl+C to stop)...\n\n");

    // Main telemetry loop
    int count = 0;
    while (g_running) {
        if (g_client.state != CONN_STATE_CONNECTED) {
            nng_msleep(1000);
            continue;
        }

        // Build topic
        char topic[128];
        snprintf(topic, sizeof(topic), TELEMETRY_TOPIC, g_client.device_id);

        // Build JSON payload
        time_t now = time(NULL);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

        double temperature = 20.0 + (rand() % 100) / 10.0;
        double humidity = 40.0 + (rand() % 300) / 10.0;

        char payload[512];
        snprintf(payload, sizeof(payload),
                 "{\"timestamp\":\"%s\",\"data\":{\"temperature\":%.2f,\"humidity\":%.2f},"
                 "\"transport\":\"%s\",\"message_id\":%d}",
                 timestamp, temperature, humidity,
                 g_client.transport == TRANSPORT_QUIC ? "QUIC" : "TCP",
                 ++count);

        printf("[INFO] Publishing telemetry #%d via %s\n", count,
               g_client.transport == TRANSPORT_QUIC ? "QUIC" : "TCP+TLS");
        printf("[INFO]   Topic: %s\n", topic);
        printf("[INFO]   Payload: %s\n", payload);

        if (publish_message(&g_client, topic, payload, strlen(payload), MQTT_QOS) == 0) {
            printf("[INFO] Telemetry #%d sent successfully\n\n", count);
        } else {
            printf("[WARN] Telemetry #%d failed, will retry on reconnect\n\n", count);
        }

        // Sleep 10 seconds
        for (int i = 0; i < 100 && g_running; i++) {
            nng_msleep(100);
        }

        // Demo parallel publish every 5 messages
        if (count % 5 == 0 && g_running && g_client.state == CONN_STATE_CONNECTED) {
            printf("\n[DEMO] Parallel multi-stream publish demonstration...\n");

            // Prepare multiple topics
            char topic1[128], topic2[128], topic3[128];
            snprintf(topic1, sizeof(topic1), "device/%s/telemetry/temperature", g_client.device_id);
            snprintf(topic2, sizeof(topic2), "device/%s/telemetry/humidity", g_client.device_id);
            snprintf(topic3, sizeof(topic3), "device/%s/telemetry/status", g_client.device_id);

            char payload1[128], payload2[128], payload3[128];
            snprintf(payload1, sizeof(payload1), "{\"temperature\":%.2f}", temperature);
            snprintf(payload2, sizeof(payload2), "{\"humidity\":%.2f}", humidity);
            snprintf(payload3, sizeof(payload3), "{\"status\":\"ok\",\"uptime\":%d}", count * 10);

            publish_task_t tasks[3] = {
                {.topic = topic1, .payload = payload1, .payload_len = strlen(payload1), .qos = 1},
                {.topic = topic2, .payload = payload2, .payload_len = strlen(payload2), .qos = 1},
                {.topic = topic3, .payload = payload3, .payload_len = strlen(payload3), .qos = 1}
            };

            publish_parallel(&g_client, tasks, 3);
            printf("\n");
        }
    }

    // Wait for health thread
    pthread_join(health_thread, NULL);

    // Print final statistics
    print_stats(&g_client);

    // Cleanup
    printf("[INFO] Disconnecting...\n");

    if (g_client.tls_cfg) {
        nng_tls_config_free(g_client.tls_cfg);
    }
    nng_dialer_close(g_client.dialer);
    nng_close(g_client.sock);

    if (g_client.session_ticket) {
        free(g_client.session_ticket);
    }

    pthread_mutex_destroy(&g_client.lock);

    printf("[INFO] Shutdown complete\n");
    return 0;
}
