/**
 * TESAIoT WSS Live Streaming - C Client
 *
 * Subscribe to real-time device telemetry via WebSocket Secure MQTT.
 *
 * Prerequisites:
 * 1. Install libmosquitto: sudo apt install libmosquitto-dev
 * 2. Generate an MQTT API Token from TESAIoT Admin UI
 *    (Organization Settings > MQTT API Tokens > Generate New Token)
 * 3. Set MQTT_API_TOKEN environment variable
 *
 * Build:
 *   make
 *
 * Usage:
 *   cp .env.example .env
 *   source .env  # or export MQTT_API_TOKEN=...
 *   ./wss_mqtt_client
 *
 * See: https://github.com/tesaiot/developer-hub
 */

#include <mosquitto.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * Configuration
 * ========================================================================== */

#define DEFAULT_HOST "mqtt.tesaiot.com"
#define DEFAULT_PORT 8085
#define DEFAULT_TOPIC "device/+/telemetry/#"
#define KEEPALIVE 60
#define TOKEN_PREFIX "tesa_mqtt_"

typedef struct
{
    const char *token;
    const char *host;
    int port;
    const char *topic;
    char client_id[64];
} Config;

/* Global state for signal handling */
static volatile int g_running = 1;
static struct mosquitto *g_mosq = NULL;

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * Get current ISO8601 timestamp
 */
static void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

/**
 * Parse device ID from topic
 * Topic format: device/<device_id>/telemetry/<sensor_type>
 */
static const char *parse_device_id(const char *topic, char *buffer, size_t size)
{
    const char *start = topic;
    const char *end;

    /* Skip "device/" prefix */
    if (strncmp(topic, "device/", 7) == 0)
    {
        start = topic + 7;
    }

    /* Find end of device ID */
    end = strchr(start, '/');
    if (end)
    {
        size_t len = end - start;
        if (len >= size)
            len = size - 1;
        strncpy(buffer, start, len);
        buffer[len] = '\0';
    }
    else
    {
        strncpy(buffer, start, size - 1);
        buffer[size - 1] = '\0';
    }

    return buffer;
}

/**
 * Parse sensor type from topic
 */
static const char *parse_sensor_type(const char *topic, char *buffer, size_t size)
{
    const char *telemetry = strstr(topic, "/telemetry/");

    if (telemetry)
    {
        const char *sensor = telemetry + 11; /* Skip "/telemetry/" */
        strncpy(buffer, sensor, size - 1);
        buffer[size - 1] = '\0';
    }
    else
    {
        strncpy(buffer, "default", size - 1);
        buffer[size - 1] = '\0';
    }

    return buffer;
}

/* ============================================================================
 * MQTT Callbacks
 * ========================================================================== */

/**
 * Called when connected to broker
 */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    Config *config = (Config *)userdata;

    if (rc == 0)
    {
        printf("✅ Connected to TESAIoT MQTT Broker!\n");

        /* Subscribe to telemetry topic */
        int ret = mosquitto_subscribe(mosq, NULL, config->topic, 1);
        if (ret == MOSQ_ERR_SUCCESS)
        {
            printf("📡 Subscribed to: %s\n", config->topic);
            printf("\n");
            printf("Waiting for telemetry messages...\n");
            printf("──────────────────────────────────────────────────\n");
            printf("\n");
        }
        else
        {
            fprintf(stderr, "❌ Subscribe failed: %s\n",
                    mosquitto_strerror(ret));
        }
    }
    else
    {
        const char *reason;
        switch (rc)
        {
        case 1:
            reason = "protocol version";
            break;
        case 2:
            reason = "identifier rejected";
            break;
        case 3:
            reason = "broker unavailable";
            break;
        case 4:
            reason = "bad credentials";
            break;
        case 5:
            reason = "not authorized";
            break;
        default:
            reason = "unknown";
            break;
        }
        fprintf(stderr, "❌ Connection failed: %s (rc=%d)\n", reason, rc);
    }
}

/**
 * Called when message received
 */
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg)
{
    (void)mosq;
    (void)userdata;

    char timestamp[32];
    char device_id[64];
    char sensor_type[64];

    get_timestamp(timestamp, sizeof(timestamp));
    parse_device_id(msg->topic, device_id, sizeof(device_id));
    parse_sensor_type(msg->topic, sensor_type, sizeof(sensor_type));

    /* Print received message */
    printf("[%s] %s\n", timestamp, msg->topic);
    printf("  Device: %s\n", device_id);
    printf("  Sensor: %s\n", sensor_type);
    printf("  Data: %.*s\n", msg->payloadlen, (char *)msg->payload);
    printf("\n");

    /* TODO: Add custom message processing here */
    /* - Store in database */
    /* - Forward to webhook */
    /* - Trigger alerts */
}

/**
 * Called when disconnected from broker
 */
static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    (void)mosq;
    (void)userdata;

    if (rc != 0)
    {
        fprintf(stderr, "🔌 Disconnected unexpectedly (rc=%d)\n", rc);
        if (g_running)
        {
            fprintf(stderr, "   Reconnecting in 5 seconds...\n");
        }
    }
}

/**
 * Called on log messages (debug)
 */
#ifdef DEBUG
static void on_log(struct mosquitto *mosq, void *userdata,
                   int level, const char *str)
{
    (void)mosq;
    (void)userdata;
    printf("[MOSQUITTO-%d] %s\n", level, str);
}
#endif

/* ============================================================================
 * Signal Handling
 * ========================================================================== */

static void signal_handler(int signum)
{
    (void)signum;
    printf("\n");
    printf("Received shutdown signal. Disconnecting...\n");
    g_running = 0;
    if (g_mosq)
    {
        mosquitto_disconnect(g_mosq);
    }
}

/* ============================================================================
 * Configuration Loading
 * ========================================================================== */

static Config load_config(void)
{
    Config config;

    config.token = getenv("MQTT_API_TOKEN");
    config.host = getenv("MQTT_HOST");
    config.topic = getenv("MQTT_TOPIC");

    const char *port_str = getenv("MQTT_PORT");

    /* Apply defaults */
    if (!config.host || config.host[0] == '\0')
    {
        config.host = DEFAULT_HOST;
    }
    if (!config.topic || config.topic[0] == '\0')
    {
        config.topic = DEFAULT_TOPIC;
    }
    if (port_str && port_str[0] != '\0')
    {
        config.port = atoi(port_str);
    }
    else
    {
        config.port = DEFAULT_PORT;
    }

    /* Generate client ID */
    snprintf(config.client_id, sizeof(config.client_id),
             "tesaiot-c-%ld", (long)time(NULL));

    return config;
}

static int validate_config(const Config *config)
{
    if (!config->token || config->token[0] == '\0')
    {
        fprintf(stderr, "❌ ERROR: MQTT_API_TOKEN is required\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Please set MQTT_API_TOKEN environment variable\n");
        fprintf(stderr, "Generate a token from TESAIoT Admin UI:\n");
        fprintf(stderr, "  1. Login to https://admin.tesaiot.com\n");
        fprintf(stderr, "  2. Go to Organization Settings > MQTT API Tokens\n");
        fprintf(stderr, "  3. Click \"Generate New Token\"\n");
        return 0;
    }

    if (strncmp(config->token, TOKEN_PREFIX, strlen(TOKEN_PREFIX)) != 0)
    {
        fprintf(stderr, "❌ ERROR: Invalid token format\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Token should start with \"%s\"\n", TOKEN_PREFIX);
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Display Functions
 * ========================================================================== */

static void display_banner(void)
{
    printf("\n");
    printf("┌─────────────────────────────────────────────────┐\n");
    printf("│  TESAIoT WSS Live Streaming - C Client          │\n");
    printf("└─────────────────────────────────────────────────┘\n");
    printf("\n");
}

static void display_config(const Config *config)
{
    size_t token_len = strlen(config->token);

    printf("Connecting to TESAIoT MQTT Broker via WSS...\n");
    printf("  Host: %s\n", config->host);
    printf("  Port: %d\n", config->port);
    printf("  Client ID: %s\n", config->client_id);
    printf("  Token: %.20s...%s\n",
           config->token,
           token_len > 4 ? config->token + token_len - 4 : "");
    printf("\n");
}

/* ============================================================================
 * Main
 * ========================================================================== */

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int ret = EXIT_SUCCESS;

    display_banner();

    /* Load and validate configuration */
    Config config = load_config();
    if (!validate_config(&config))
    {
        return EXIT_FAILURE;
    }

    display_config(&config);

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize mosquitto library */
    mosquitto_lib_init();

    /* Create mosquitto client */
    struct mosquitto *mosq = mosquitto_new(config.client_id, true, &config);
    if (!mosq)
    {
        fprintf(stderr, "❌ Failed to create mosquitto client\n");
        ret = EXIT_FAILURE;
        goto cleanup_lib;
    }

    g_mosq = mosq;

    /* Set protocol version */
    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V311);

    /* Enable TLS for WSS */
    int tls_ret = mosquitto_tls_set(mosq, NULL, NULL, NULL, NULL, NULL);
    if (tls_ret != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "❌ TLS setup failed: %s\n", mosquitto_strerror(tls_ret));
        ret = EXIT_FAILURE;
        goto cleanup_mosq;
    }

    /* Skip certificate verification for testing (remove in production) */
    mosquitto_tls_insecure_set(mosq, true);

    /* Set credentials */
    mosquitto_username_pw_set(mosq, config.token, config.token);

    /* Set callbacks */
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

#ifdef DEBUG
    mosquitto_log_callback_set(mosq, on_log);
#endif

    /* Connect to broker */
    /* Note: For WSS, we need to connect with port 8085 and the path is handled
     * by the broker. Some mosquitto versions support ws:// prefix in hostname
     * for WebSocket connections. */
    int conn_ret = mosquitto_connect(mosq, config.host, config.port, KEEPALIVE);
    if (conn_ret != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "❌ Connection failed: %s\n", mosquitto_strerror(conn_ret));
        ret = EXIT_FAILURE;
        goto cleanup_mosq;
    }

    /* Run event loop */
    while (g_running)
    {
        int loop_ret = mosquitto_loop(mosq, 1000, 1);
        if (loop_ret != MOSQ_ERR_SUCCESS && g_running)
        {
            fprintf(stderr, "Loop error: %s\n", mosquitto_strerror(loop_ret));
            /* Attempt reconnection */
            sleep(5);
            mosquitto_reconnect(mosq);
        }
    }

    printf("✅ Disconnected. Goodbye!\n");

cleanup_mosq:
    mosquitto_destroy(mosq);
    g_mosq = NULL;

cleanup_lib:
    mosquitto_lib_cleanup();

    return ret;
}
