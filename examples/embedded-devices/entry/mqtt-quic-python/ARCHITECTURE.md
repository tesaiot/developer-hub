# MQTT over QUIC - Python Client Architecture

## Overview

Python client for connecting to TESAIoT Platform using MQTT over QUIC protocol. Provides faster connection times, seamless reconnection, and connection migration for mobile/IoT devices. Uses aioquic library for native QUIC support with TLS 1.3.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                      Python Application                         │
├─────────────────────────────────────────────────────────────────┤
│  mqtt_quic_client.py / mqtt_quic_aioquic.py                     │
│  ├── MQTTQUICClient class                                       │
│  │   ├── __init__()            → Load config, validate creds    │
│  │   ├── connect()             → Establish QUIC connection      │
│  │   ├── _create_ssl_context() → TLS 1.3 with CA verification   │
│  │   ├── publish_telemetry()   → Send sensor data               │
│  │   └── disconnect()          → Graceful shutdown              │
│  └── Callbacks                                                  │
│      ├── on_connect()          → Connection established         │
│      ├── on_disconnect()       → Handle disconnection           │
│      ├── on_message()          → Incoming message handler       │
│      └── on_publish()          → Publish confirmation           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ QUIC (UDP) Port 14567
                              │ TLS 1.3 (built-in)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform (EMQX)                      │
├─────────────────────────────────────────────────────────────────┤
│  MQTT over QUIC Endpoint                                        │
│  ├── mqtts://tesaiot.com:14567                                  │
│  ├── TLS 1.3 Server Certificate Verification                    │
│  ├── Username/Password Authentication                           │
│  └── Topic: devices/{device_id}/telemetry                       │
└─────────────────────────────────────────────────────────────────┘
```

## Performance Benefits

```ini
┌────────────────────────────────────────────────────────────────┐
│                    QUIC vs TCP+TLS Comparison                  │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  Metric              TCP+TLS        QUIC         Improvement   │
│  ────────────────────────────────────────────────────────────  │
│  Initial Connect     2-RTT          1-RTT        50% faster    │
│                      (~200ms)       (~100ms)                   │
│                                                                │
│  Reconnection        2-RTT          0-RTT        90% faster    │
│                      (~200ms)       (~10ms)                    │
│                                                                │
│  IP Address Change   Reconnect      Migration    Seamless      │
│                      Required       Built-in                   │
│                                                                │
│  Packet Loss         Head-of-line   Independent  Better        │
│                      Blocking       Streams      Resilience    │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Connection Flow

```ini
┌──────────┐     ┌───────────────┐     ┌─────────────────┐
│  Start   │────►│ Load .env     │────►│ Load Config     │
│          │     │ credentials   │     │ mqtt-quic-      │
└──────────┘     └───────────────┘     │ config.json     │
                                       └───────┬─────────┘
                                               │
                 ┌─────────────────────────────┘
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    SSL Context Creation                         │
│  ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)                        │
│  ├── minimum_version = TLSv1_3  // QUIC requires TLS 1.3        │
│  ├── maximum_version = TLSv1_3                                  │
│  ├── load_verify_locations(ca-chain.pem)                        │
│  ├── check_hostname = True                                      │
│  └── verify_mode = CERT_REQUIRED                                │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT Client Setup                            │
│  mqtt.Client(client_id, clean_session=True, protocol=MQTTv311)  │
│  ├── username_pw_set(device_id, password)                       │
│  ├── tls_set_context(ssl_context)                               │
│  └── Set callbacks (connect, disconnect, message, publish)      │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Connect & Loop                               │
│  client.connect(host, port, keepalive=60)                       │
│  client.loop_start()  // Background thread                      │
│  Wait for on_connect callback (timeout: 10s)                    │
└─────────────────────────────────────────────────────────────────┘
```

## Configuration Loading

```python
# Priority order for configuration:
# 1. mqtt-quic-config.json (from downloaded bundle)
# 2. .env file (environment variables)
# 3. Default values

config = {
    "protocol": "mqtts",
    "host": "tesaiot.com",
    "port": 14567,
    "transport": "quic",
    "tls": {
        "enabled": True,
        "version": "1.3",
        "ca_file": "ca-chain.pem",
        "verify_server": True,
        "verify_mode": "server-only"
    },
    "auth": {
        "method": "username_password",
        "username": "<device_id>",
        "password": "<from_bundle>"
    },
    "connection": {
        "keepalive": 60,
        "clean_session": True,
        "connect_timeout": 10
    }
}
```

## Telemetry Payload Format

```json
{
  "timestamp": "2025-12-09T10:30:45.123456Z",
  "data": {
    "temperature": 25.50,
    "humidity": 55.30
  }
}
```

## Authentication Model

```ini
┌────────────────────────────────────────────────────────────────┐
│                    Server-TLS Authentication                   │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────────┐                      ┌─────────────────────┐  │
│  │   Client    │                      │   TESAIoT Server    │  │
│  └──────┬──────┘                      └──────────┬──────────┘  │
│         │                                        │             │
│         │  1. QUIC ClientHello + TLS 1.3         │             │
│         │───────────────────────────────────────►│             │
│         │                                        │             │
│         │  2. Server Certificate                 │             │
│         │◄───────────────────────────────────────│             │
│         │                                        │             │
│         │  3. Verify against ca-chain.pem        │             │
│         │  (Client validates server identity)    │             │
│         │                                        │             │
│         │  4. MQTT CONNECT                       │             │
│         │     Username: device_id                │             │
│         │     Password: from_bundle              │             │
│         │───────────────────────────────────────►│             │
│         │                                        │             │
│         │  5. MQTT CONNACK (success/fail)        │             │
│         │◄───────────────────────────────────────│             │
│                                                                │
│  Note: No client certificate required (Server-TLS mode)        │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Library Options

```ini
┌─────────────────────────────────────────────────────────────────┐
│                    Python MQTT over QUIC Options                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Option 1: aioquic (Recommended)                                │
│  ├── Native Python QUIC implementation                          │
│  ├── Full 0-RTT support                                         │
│  ├── Connection migration                                       │
│  └── Requires: pip install aioquic                              │
│                                                                 │
│  Option 2: NanoSDK Python Bindings                              │
│  ├── Uses NanoMQ's NNG library                                  │
│  ├── C-based performance                                        │
│  └── Requires: pip install nng                                  │
│                                                                 │
│  Option 3: paho-mqtt (Fallback)                                 │
│  ├── Uses TCP+TLS (not true QUIC)                               │
│  ├── Widely available                                           │
│  └── Requires: pip install paho-mqtt                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Dependencies

```sh
# requirements.txt
aioquic>=0.9.0          # QUIC protocol implementation
paho-mqtt>=2.0.0        # MQTT client (fallback)
python-dotenv>=1.0.0    # Environment variable management
```

## Files

```ini
mqtt-quic-python/
├── mqtt_quic_client.py      # paho-mqtt based client
├── mqtt_quic_aioquic.py     # aioquic based client (recommended)
├── requirements.txt         # Python dependencies
├── install_deps.sh          # Dependency installation script
├── .env.example             # Environment template
├── ca-chain.pem             # CA certificate (from bundle)
├── mqtt-quic-config.json    # Connection config (from bundle)
├── QUICKSTART.md            # Quick start guide
├── README.md                # Full documentation
└── ARCHITECTURE.md          # This file
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | ✅ Supported | Python 3.9+ |
| Linux (Ubuntu/Debian) | ✅ Supported | Python 3.9+ |
| Raspberry Pi | ✅ Supported | Python 3.9+ |
| Windows | ✅ Supported | Python 3.9+ |

## Use Cases

```ini
┌────────────────────────────────────────────────────────────────┐
│                    Ideal Use Cases for QUIC                    │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ✅ Internet of Vehicles (IoV)                                  │
│     └── Seamless connectivity while moving between cell towers  │
│                                                                 │
│  ✅ Mobile IoT Devices                                          │
│     └── Weak/intermittent cellular networks                     │
│                                                                 │
│  ✅ Low-Latency Applications                                    │
│     └── Real-time control systems requiring fast reconnect      │
│                                                                 │
│  ✅ High-Density Deployments                                    │
│     └── Resource-efficient multiplexed connections              │
│                                                                 │
│  ❌ Corporate Networks (may block UDP)                          │
│  ❌ mTLS Required (use port 8883 instead)                       │
│  ❌ TCP Mandated by Policy                                      │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Network Requirements

| Requirement | Details |
|-------------|---------|
| Protocol | UDP (not TCP) |
| Port | 14567 outbound |
| Firewall | Must allow UDP egress |
| NAT | QUIC handles traversal automatically |
| MTU | 1280 bytes minimum |

## Error Handling

| Error | Meaning | Solution |
|-------|---------|----------|
| `Connection timeout` | QUIC handshake failed | Check UDP 14567 allowed |
| `ssl.SSLCertVerificationError` | CA cert issue | Verify ca-chain.pem exists |
| `rc == 4` | Bad username/password | Check credentials from bundle |
| `FileNotFoundError` | Missing config/cert | Download bundle from platform |
