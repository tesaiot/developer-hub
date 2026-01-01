# MQTT over QUIC - C/C++ Client Architecture

## Overview

High-performance C/C++ client for connecting to TESAIoT Platform using MQTT over QUIC protocol. Uses NanoSDK library with MsQuic for UDP-based, low-latency IoT communication with built-in TLS 1.3 security.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                      C/C++ Application                          │
├─────────────────────────────────────────────────────────────────┤
│  mqtt_quic_client.c                                             │
│  ├── main()                    → Entry point, signal handling   │
│  ├── mqtt_connect()            → QUIC connection setup          │
│  │   ├── nng_mqtt_quic_client_open()                            │
│  │   ├── nng_tls_config_alloc() → TLS 1.3 configuration         │
│  │   ├── load_ca_cert()         → Server cert verification      │
│  │   └── nng_dialer_start()     → Initiate QUIC handshake       │
│  ├── publish_telemetry()       → JSON payload publishing        │
│  └── Callbacks                                                  │
│      ├── connect_cb()          → Connection events              │
│      ├── message_cb()          → Incoming messages              │
│      └── signal_handler()      → Graceful shutdown              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ QUIC (UDP) Port 14567
                              │ TLS 1.3 (built-in)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform (EMQX)                      │
├─────────────────────────────────────────────────────────────────┤
│  MQTT over QUIC Endpoint                                        │
│  ├── mqtt-quic://tesaiot.com:14567                              │
│  ├── TLS 1.3 Server Certificate                                 │
│  ├── Username/Password Authentication                           │
│  └── Topic: devices/{device_id}/telemetry                       │
└─────────────────────────────────────────────────────────────────┘
```

## QUIC vs TCP Connection Flow

```ini
┌────────────────────────────────────────────────────────────────────┐
│                    Traditional TCP + TLS                           │
├────────────────────────────────────────────────────────────────────┤
│  Client              Server                                        │
│    │                   │                                           │
│    │──── SYN ─────────►│  RTT 1 (TCP Handshake)                    │
│    │◄─── SYN+ACK ──────│                                           │
│    │──── ACK ─────────►│                                           │
│    │                   │                                           │
│    │──── ClientHello ─►│  RTT 2 (TLS Handshake)                    │
│    │◄─── ServerHello ──│                                           │
│    │──── Finished ────►│                                           │
│    │◄─── Finished ─────│                                           │
│    │                   │                                           │
│    │  Total: 2-RTT (~200ms on typical networks)                    │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│                    QUIC (TLS 1.3 Built-in)                         │
├────────────────────────────────────────────────────────────────────┤
│  Client              Server                                        │
│    │                   │                                           │
│    │──── Initial ─────►│  1-RTT (Combined QUIC+TLS)                │
│    │◄─── Handshake ────│                                           │
│    │──── Data ────────►│  Immediate data after handshake           │
│    │                   │                                           │
│    │  Total: 1-RTT (~100ms) or 0-RTT on reconnect (~10ms)          │
└────────────────────────────────────────────────────────────────────┘
```

## Connection Setup Sequence

```ini
┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│  CLI Args   │────►│ Load CA Cert │────►│ Create QUIC     │
│ device_id   │     │ ca-chain.pem │     │ Socket (NNG)    │
│ password    │     └──────────────┘     └───────┬─────────┘
└─────────────┘                                  │
                                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TLS Configuration                            │
│  nng_tls_config_alloc(NNG_TLS_MODE_CLIENT)                      │
│  ├── nng_tls_config_version(TLS_1_3, TLS_1_3)  // QUIC requires │
│  ├── nng_tls_config_ca_chain(ca_cert)          // Server verify │
│  ├── nng_tls_config_auth_mode(REQUIRED)        // Must verify   │
│  └── nng_tls_config_server_name(HOST)          // SNI           │
└─────────────────────────────────────────────────────────────────┘
                                                 │
                                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT CONNECT Message                         │
│  nng_mqtt_msg_set_connect_client_id(device_id)                  │
│  nng_mqtt_msg_set_connect_user_name(device_id)                  │
│  nng_mqtt_msg_set_connect_password(password)                    │
│  nng_mqtt_msg_set_connect_keep_alive(60)                        │
│  nng_mqtt_msg_set_connect_clean_session(true)                   │
└─────────────────────────────────────────────────────────────────┘
                                                 │
                                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Wait for CONNACK                             │
│  Poll nng_recvmsg() for NNG_MQTT_CONNACK                        │
│  ├── return_code == 0 → Connected successfully                  │
│  └── return_code != 0 → Auth failed or error                    │
└─────────────────────────────────────────────────────────────────┘
```

## Telemetry Payload Format

```json
{
  "timestamp": "2025-12-09T10:30:45Z",
  "data": {
    "temperature": 25.50,
    "humidity": 55.30
  }
}
```

## Library Stack

```ini
┌─────────────────────────────────────────┐
│           Application Code              │
│         mqtt_quic_client.c              │
├─────────────────────────────────────────┤
│              NanoSDK                    │
│  MQTT Protocol + QUIC Transport Layer   │
├─────────────────────────────────────────┤
│              MsQuic                     │
│     Microsoft QUIC Implementation       │
│     (TLS 1.3 built-in via OpenSSL)      │
├─────────────────────────────────────────┤
│              UDP Sockets                │
│         Operating System                │
└─────────────────────────────────────────┘
```

## Authentication Flow

```ini
┌────────────────────────────────────────────────────────────────┐
│                    Server-TLS Authentication                   │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  1. Server Certificate Verification                            │
│     ├── Load ca-chain.pem (from bundle)                        │
│     ├── Verify server presents valid certificate               │
│     └── Hostname verification via SNI                          │
│                                                                │
│  2. Client Authentication (No Client Cert)                     │
│     ├── Username: device_id (UUID)                             │
│     └── Password: from bundle or reset                         │
│                                                                │
│  3. Topic Authorization                                        │
│     └── devices/{device_id}/telemetry (scoped to device)       │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Build System

```ini
┌─────────────────────────────────────────────────────────────────┐
│                    CMake Build Process                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  CMakeLists.txt                                                 │
│  ├── find_package(nng REQUIRED)     // NanoSDK                  │
│  ├── find_package(OpenSSL REQUIRED) // TLS support              │
│  └── target_link_libraries                                      │
│      └── nng::nng, OpenSSL::SSL, OpenSSL::Crypto                │
│                                                                 │
│  Build Commands:                                                │
│  $ mkdir build && cd build                                      │
│  $ cmake ..                                                     │
│  $ make                                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Platform Support

| Platform | Architecture | Status | Notes |
|----------|--------------|--------|-------|
| Ubuntu 22.04+ | x86_64 | ✅ Supported | Primary target |
| Debian 11+ | x86_64 | ✅ Supported | |
| Raspberry Pi OS | ARM64 | ✅ Supported | Build MsQuic from source |
| macOS | ARM64/x86_64 | ❌ Not supported | MsQuic Linux-only APIs |
| Windows | x86_64 | ❌ Not supported | Use WSL2 |

## Files

```ini
mqtt-quic-c/
├── mqtt_quic_client.c    # Main client implementation
├── CMakeLists.txt        # CMake build configuration
├── install_deps.sh       # Dependency installation script
├── certs/
│   └── ca-chain.pem      # CA certificate (from bundle)
├── README.md             # Usage documentation
├── README-RPI.md         # Raspberry Pi specific instructions
└── ARCHITECTURE.md       # This file
```

## Network Requirements

| Requirement | Details |
|-------------|---------|
| Protocol | UDP (not TCP) |
| Port | 14567 outbound |
| Firewall | Must allow UDP egress |
| NAT | QUIC handles NAT traversal |
| MTU | 1280 bytes minimum |

## Error Handling

| Error Code | Meaning | Solution |
|------------|---------|----------|
| `Connection timeout` | QUIC handshake failed | Check UDP port 14567 allowed |
| `CA certificate not found` | Missing ca-chain.pem | Download bundle from platform |
| `return_code != 0` | MQTT auth failed | Verify device_id/password |
| `NNG_ECLOSED` | Connection dropped | Check network stability |
