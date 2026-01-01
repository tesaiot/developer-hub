# Advanced MQTT over QUIC - C/C++ Client Architecture

## Overview

Production-ready C/C++ client for TESAIoT Platform with advanced MQTT over QUIC features. Extends entry-level Example #10 with automatic transport fallback, session resumption (0-RTT), multi-stream publishing, and connection health monitoring.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────────────┐
│                      Advanced C/C++ MQTT Client                         │
├─────────────────────────────────────────────────────────────────────────┤
│  mqtt_quic_advanced.c (950+ lines)                                      │
│  ├── Connection Layer                                                   │
│  │   ├── smart_connect()          → QUIC → TCP fallback logic           │
│  │   ├── try_quic_connect()       → Primary QUIC (UDP 14567)            │
│  │   ├── try_tcp_connect()        → Fallback TCP+TLS (8884)             │
│  │   └── reconnect_with_backoff() → Exponential retry with jitter       │
│  ├── Session Management                                                 │
│  │   ├── load_session_ticket()    → Restore 0-RTT ticket from file      │
│  │   └── save_session_ticket()    → Persist ticket for fast reconnect   │
│  ├── Publishing                                                         │
│  │   ├── publish_message()        → Single publish with latency track   │
│  │   └── publish_parallel()       → Multi-stream concurrent publish     │
│  └── Monitoring                                                         │
│      ├── health_check_thread()    → Background connection monitor       │
│      ├── print_stats()            → Statistics display                  │
│      └── update_latency_stats()   → Latency tracking (avg/min/max)      │
└─────────────────────────────────────────────────────────────────────────┘
                              │
           ┌──────────────────┼──────────────────┐
           ▼                                     ▼
┌─────────────────────────┐         ┌─────────────────────────┐
│   Primary: QUIC (UDP)   │         │  Fallback: TCP+TLS      │
│   Port: 14567           │         │  Port: 8884             │
│   TLS: 1.3 (mandatory)  │         │  TLS: 1.2/1.3           │
│   0-RTT: Supported      │         │  Full handshake only    │
└─────────────────────────┘         └─────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform (EMQX)                              │
│  Topic: device/{device_id}/telemetry/sensors                            │
└─────────────────────────────────────────────────────────────────────────┘
```

## Advanced Features vs Entry-Level

```ini
┌──────────────────────────────────────────────────────────────────────┐
│                    Feature Comparison                                │
├──────────────────────────────────────────────────────────────────────┤
│  Feature               Entry-Level #10      Advanced #21             │
│  ─────────────────────────────────────────────────────────────────── │
│  Transport             QUIC only            QUIC + TCP fallback      │
│  Reconnection          Manual               Auto exponential backoff │
│  Multi-Stream          Single stream        Up to 4 parallel streams │
│  Session Resume        None                 0-RTT ticket storage     │
│  Health Monitoring     None                 Built-in stats + checks  │
│  Latency Tracking      None                 Avg/Min/Max metrics      │
│  Thread Safety         None                 Mutex-protected ops      │
│  Memory Usage          ~2 MB                ~3 MB                    │
└──────────────────────────────────────────────────────────────────────┘
```

## Smart Connection Flow

```ini
┌────────────────────────────────────────────────────────────────────────┐
│                         smart_connect()                                │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  ┌────────────────────┐                                                │
│  │ Load Session Ticket│  ← Load from .session_ticket.bin if exists     │
│  └─────────┬──────────┘                                                │
│            ▼                                                           │
│  ┌────────────────────┐     ┌────────────────────┐                     │
│  │ Try QUIC Connect   │────►│ Success?           │                     │
│  │ (UDP 14567)        │     │ 0-RTT if ticket    │                     │
│  │ Timeout: 5000ms    │     └────────┬───────────┘                     │
│  └────────────────────┘              │                                 │
│            │                    Yes  │  No                             │
│            │    ┌────────────────────┼────────────────┐                │
│            │    ▼                                     ▼                │
│            │ ┌─────────────────┐    ┌────────────────────┐             │
│            │ │ Save Session    │    │ Try TCP+TLS        │             │
│            │ │ Ticket for 0-RTT│    │ (Port 8884)        │             │
│            │ │ Return Success  │    │ Timeout: 10000ms   │             │
│            │ └─────────────────┘    └────────┬───────────┘             │
│            │                                  │                        │
│            │                            Yes   │  No                    │
│            │              ┌───────────────────┼─────────┐              │
│            │              ▼                             ▼              │
│            │  ┌─────────────────┐          ┌─────────────────┐         │
│            │  │ Return Success  │          │ Connection      │         │
│            │  │ (TCP mode)      │          │ Failed          │         │
│            │  └─────────────────┘          └─────────────────┘         │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

## Session Resumption (0-RTT)

```ini
┌────────────────────────────────────────────────────────────────────────┐
│                    0-RTT Session Ticket Flow                           │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  FIRST CONNECTION (Full Handshake):                                    │
│  ┌────────────┐     ┌────────────┐                                     │
│  │   Client   │     │   Server   │                                     │
│  └─────┬──────┘     └──────┬─────┘                                     │
│        │  1. ClientHello    │        ← TLS 1.3 handshake (~100-200ms)  │
│        │───────────────────►│                                          │
│        │  2. ServerHello    │                                          │
│        │◄───────────────────│                                          │
│        │  3. Finished       │                                          │
│        │◄──────────────────►│                                          │
│        │  4. Session Ticket │        ← Server sends ticket             │
│        │◄───────────────────│                                          │
│        │                    │                                          │
│        │  [Save to .session_ticket.bin]                                │
│                                                                        │
│  SUBSEQUENT CONNECTION (0-RTT):                                        │
│  ┌────────────┐     ┌────────────┐                                     │
│  │   Client   │     │   Server   │                                     │
│  └─────┬──────┘     └──────┬─────┘                                     │
│        │  ClientHello +    │        ← Resume with ticket (~20-50ms)    │
│        │  Session Ticket + │                                           │
│        │  Early Data       │                                           │
│        │──────────────────►│                                           │
│        │  ServerHello +    │        ← Immediate data allowed           │
│        │  Accept Early Data│                                           │
│        │◄──────────────────│                                           │
│                                                                        │
│  Result: 90% faster reconnection!                                      │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

## Multi-Stream Parallel Publishing

```ini
┌─────────────────────────────────────────────────────────────────────────┐
│                    QUIC Multi-Stream Publishing                         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Traditional TCP MQTT (Sequential):                                     │
│  ┌────────────┐                                                         │
│  │ Stream 0   │──► Topic A ──► Topic B ──► Topic C ──► ...              │
│  └────────────┘    (blocking)   (wait)     (wait)                       │
│                                                                         │
│  QUIC Multi-Stream (Parallel):                                          │
│  ┌────────────┐                                                         │
│  │ Stream 1   │──────────────────► Topic A                              │
│  ├────────────┤                                                         │
│  │ Stream 2   │──────────────────► Topic B     (all concurrent!)        │
│  ├────────────┤                                                         │
│  │ Stream 3   │──────────────────► Topic C                              │
│  └────────────┘                                                         │
│                                                                         │
│  Code Example:                                                          │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │ publish_task_t tasks[3] = {                                  │       │
│  │   {.topic = "device/.../temperature", .payload = temp_json}, │       │
│  │   {.topic = "device/.../humidity", .payload = humid_json},   │       │
│  │   {.topic = "device/.../status", .payload = status_json}     │       │
│  │ };                                                           │       │
│  │ publish_parallel(&client, tasks, 3);                         │       │
│  └──────────────────────────────────────────────────────────────┘       │
│                                                                         │
│  Result: 3-4x faster for multi-topic publish!                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Exponential Backoff Reconnection

```ini
┌─────────────────────────────────────────────────────────────────────────┐
│                    Reconnection with Exponential Backoff                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Formula: delay = BASE × 2^attempt + random_jitter                      │
│  BASE = 1000ms, MAX = 60000ms                                           │
│                                                                         │
│  Attempt 1: delay = 1000 × 2^0 + jitter  =  ~1-1.5 seconds              │
│  Attempt 2: delay = 1000 × 2^1 + jitter  =  ~2-3 seconds                │
│  Attempt 3: delay = 1000 × 2^2 + jitter  =  ~4-6 seconds                │
│  Attempt 4: delay = 1000 × 2^3 + jitter  =  ~8-12 seconds               │
│  Attempt 5: delay = 1000 × 2^4 + jitter  = ~16-24 seconds               │
│  Attempt 6: delay = 1000 × 2^5 + jitter  = ~32-48 seconds               │
│  Attempt 7+: delay = MAX (60 seconds)                                   │
│                                                                         │
│  Why Jitter?                                                            │
│  - Prevents "thundering herd" when many devices reconnect               │
│  - Spreads load on server after network recovery                        │
│  - Random 0-50% of base delay added                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Connection Health Monitoring

```ini
╔══════════════════════════════════════════════════╗
║         CONNECTION STATISTICS                    ║
╠══════════════════════════════════════════════════╣
║ Transport      : QUIC (UDP)                      ║
║ State          : CONNECTED                       ║
║ Messages Sent  : 150                             ║
║ Bytes Sent     : 18500                           ║
║ Reconnects     : 2                               ║
║ QUIC Connects  : 3                               ║
║ TCP Connects   : 0                               ║
╠══════════════════════════════════════════════════╣
║ Avg Latency    : 15.32 ms                        ║
║ Min Latency    : 8.20 ms                         ║
║ Max Latency    : 45.50 ms                        ║
╚══════════════════════════════════════════════════╝
```

## Library Stack

```ini
┌────────────────────────────────────────────────────────────────────────┐
│                    Application Code                                    │
│                 mqtt_quic_advanced.c                                   │
├────────────────────────────────────────────────────────────────────────┤
│                       NanoSDK                                          │
│         MQTT Protocol + QUIC Transport Layer                           │
│         nng_mqtt_quic_client_open(), nng_mqtt_msg_*()                  │
├────────────────────────────────────────────────────────────────────────┤
│                       MsQuic                                           │
│            Microsoft QUIC Implementation                               │
│            TLS 1.3 built-in via OpenSSL                                │
├────────────────────────────────────────────────────────────────────────┤
│                   UDP Sockets + Threads                                │
│                   Operating System                                     │
└────────────────────────────────────────────────────────────────────────┘
```

## Data Structures

```c
// Connection state tracking
typedef struct {
    nng_socket sock;
    nng_dialer dialer;
    transport_type_t transport;    // QUIC or TCP_TLS
    connection_state_t state;      // DISCONNECTED, CONNECTING, CONNECTED
    char device_id[64];
    char password[128];
    uint8_t *session_ticket;       // 0-RTT session ticket
    size_t session_ticket_len;
    connection_stats_t stats;
    pthread_mutex_t lock;          // Thread safety
    nng_tls_config *tls_cfg;
} mqtt_client_t;

// Connection statistics
typedef struct {
    uint64_t messages_sent;
    uint64_t bytes_sent;
    uint64_t reconnect_count;
    uint64_t quic_connections;
    uint64_t tcp_connections;
    double avg_latency_ms;
    double min_latency_ms;
    double max_latency_ms;
    time_t connected_since;
} connection_stats_t;
```

## Configuration Constants

```c
// QUIC (Primary)
#define QUIC_BROKER_HOST        "mqtt.tesaiot.com"
#define QUIC_BROKER_PORT        14567

// TCP+TLS (Fallback)
#define TCP_BROKER_HOST         "mqtt.tesaiot.com"
#define TCP_BROKER_PORT         8884

// Timing
#define QUIC_CONNECT_TIMEOUT_MS     5000    // 5 seconds
#define TCP_CONNECT_TIMEOUT_MS      10000   // 10 seconds
#define MAX_RECONNECT_ATTEMPTS      10
#define BASE_RECONNECT_DELAY_MS     1000    // 1 second
#define MAX_RECONNECT_DELAY_MS      60000   // 60 seconds
#define HEALTH_CHECK_INTERVAL_MS    30000   // 30 seconds

// Performance
#define MAX_PARALLEL_STREAMS        4
#define SESSION_TICKET_FILE         ".session_ticket.bin"
```

## Files

```ini
advanced/mqtt-quic/c_cpp/
├── mqtt_quic_advanced.c          # Main advanced client (950+ lines)
├── mqtt_quic_client.c            # Entry-level client (reference)
├── CMakeLists.txt                # CMake build configuration
├── mqtt-quic-config.example.json # Connection config template
├── ca-chain.pem                  # CA certificate (from bundle)
├── README.md                     # Detailed documentation
└── ARCHITECTURE.md               # This file
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Ubuntu 22.04+ | ✅ Supported | Primary target |
| Debian 12+ | ✅ Supported | |
| Raspberry Pi OS | ✅ Supported | Build MsQuic from source |
| macOS | ❌ Not supported | MsQuic uses Linux-only APIs |
| Windows | ❌ Not supported | Use WSL2 |

## Build Requirements

- GCC 11+ or Clang 14+
- CMake 3.10+
- NanoSDK with QUIC enabled
- MsQuic library
- OpenSSL 1.1.1+

## Performance Comparison

| Metric | Entry-Level #10 | Advanced #21 |
|--------|-----------------|--------------|
| Cold Connect | ~150ms | ~150ms |
| Reconnect (0-RTT) | N/A | ~30ms |
| Network Switch | Manual restart | Auto fallback |
| Parallel Publish | Sequential | 3-4x faster |
| Memory | ~2 MB | ~3 MB |
| CPU (idle) | Minimal | +1% monitoring |
