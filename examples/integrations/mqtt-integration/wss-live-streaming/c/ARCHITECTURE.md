# C Implementation Architecture

## Overview

WebSocket-based MQTT client using the `libmosquitto` library for real-time telemetry streaming.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    C Application                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              wss_mqtt_client.c (Entry Point)          │  │
│  │                                                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐   │  │
│  │  │ getenv()    │  │ Config      │  │ Validation   │   │  │
│  │  │ loader      │  │ struct      │  │ functions    │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘   │  │
│  │         │                │                │           │  │
│  │         └────────────────┴────────────────┘           │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │           libmosquitto Client                 │    │  │
│  │  │                                               │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ mosquitto_   │  │ Callbacks           │    │    │  │
│  │  │  │ new()        │  │ - on_connect        │    │    │  │
│  │  │  │              │  │ - on_message        │    │    │  │
│  │  │  │              │  │ - on_disconnect     │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │           Message Processing                  │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ Topic Parser │  │ process_message()   │    │    │  │
│  │  │  │              │  │ (Custom Handler)    │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Code Structure

```c
// Configuration structure
typedef struct {
    const char *token;
    const char *host;
    int port;
    const char *topic;
    char client_id[64];
} Config;

// Callback functions
void on_connect(struct mosquitto *mosq, void *userdata, int rc);
void on_message(struct mosquitto *mosq, void *userdata,
                const struct mosquitto_message *msg);
void on_disconnect(struct mosquitto *mosq, void *userdata, int rc);

// Main function
int main(int argc, char *argv[]) {
    // 1. Load configuration
    Config config = load_config();

    // 2. Initialize mosquitto library
    mosquitto_lib_init();

    // 3. Create client
    struct mosquitto *mosq = mosquitto_new(
        config.client_id, true, &config
    );

    // 4. Configure WSS
    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION,
                         MQTT_PROTOCOL_V311);
    mosquitto_int_option(mosq, MOSQ_OPT_TRANSPORT,
                         MOSQ_T_WEBSOCKETS);
    mosquitto_tls_set(mosq, NULL, NULL, NULL, NULL, NULL);

    // 5. Set credentials
    mosquitto_username_pw_set(mosq, config.token, config.token);

    // 6. Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    // 7. Connect and loop
    mosquitto_connect(mosq, config.host, config.port, 60);
    mosquitto_loop_forever(mosq, -1, 1);

    // 8. Cleanup
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
```

## Event Lifecycle

```
┌─────────────────┐
│  main()         │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Load Config    │
│  (getenv)       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  mosquitto_     │
│  lib_init()     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  mosquitto_     │
│  new()          │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Configure      │
│  WSS + TLS      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Set Callbacks  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│  mosquitto_     │────▶│  on_disconnect  │
│  connect()      │     │  Auto-reconnect │
└────────┬────────┘     └────────┬────────┘
         │                       │
         ▼                       │
┌─────────────────┐              │
│  on_connect     │◀─────────────┘
│  Subscribe      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  loop_forever() │
│  on_message     │◀───┐
└────────┬────────┘    │
         │             │
         ▼             │
┌─────────────────┐    │
│ process_message │────┘
└─────────────────┘
```

## WebSocket Configuration

```c
// Enable WebSocket transport
mosquitto_int_option(mosq, MOSQ_OPT_TRANSPORT, MOSQ_T_WEBSOCKETS);

// Set WebSocket path (if supported by version)
// Note: libmosquitto 2.0+ supports path via host string
// Format: "mqtt.tesaiot.com/mqtt"

// Enable TLS for WSS
mosquitto_tls_set(mosq,
    NULL,    // CA file (NULL for system certs)
    NULL,    // CA path
    NULL,    // client cert (not needed for server TLS)
    NULL,    // client key
    NULL     // password callback
);

// Optional: Skip certificate verification (development only)
mosquitto_tls_insecure_set(mosq, true);
```

## Key Design Decisions

| Decision            | Rationale                              |
| ------------------- | -------------------------------------- |
| libmosquitto        | Mature, well-tested, WebSocket support |
| Single file         | Simple example, easy to understand     |
| getenv() for config | Standard C, no external dependencies   |
| Signal handlers     | Graceful shutdown on Ctrl+C            |

## Error Handling

```c
void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    switch (rc) {
        case 0:
            printf("✅ Connected!\n");
            mosquitto_subscribe(mosq, NULL, topic, 1);
            break;
        case 1:
            fprintf(stderr, "Connection refused: protocol version\n");
            break;
        case 2:
            fprintf(stderr, "Connection refused: identifier rejected\n");
            break;
        case 3:
            fprintf(stderr, "Connection refused: broker unavailable\n");
            break;
        case 4:
            fprintf(stderr, "Connection refused: bad credentials\n");
            break;
        case 5:
            fprintf(stderr, "Connection refused: not authorized\n");
            break;
        default:
            fprintf(stderr, "Connection failed: %d\n", rc);
    }
}
```

## Signal Handling

```c
#include <signal.h>

static volatile int running = 1;
static struct mosquitto *g_mosq = NULL;

void signal_handler(int signum) {
    running = 0;
    if (g_mosq) {
        mosquitto_disconnect(g_mosq);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // ... setup code ...

    while (running) {
        mosquitto_loop(mosq, 1000, 1);
    }

    // Cleanup
}
```

## Memory Management

| Resource           | Allocation             | Cleanup                   |
| ------------------ | ---------------------- | ------------------------- |
| mosquitto instance | `mosquitto_new()`      | `mosquitto_destroy()`     |
| mosquitto library  | `mosquitto_lib_init()` | `mosquitto_lib_cleanup()` |
| Config strings     | Stack/static           | None needed               |
| Message payloads   | Library-managed        | Auto-freed                |
