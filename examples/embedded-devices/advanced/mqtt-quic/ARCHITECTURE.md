# MQTT over QUIC Architecture

## Overview

High-performance MQTT client using QUIC transport protocol for faster connections and better network resilience.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT-QUIC Application                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐                     │
│  │  mqtt_client    │───▶│  quic_transport │                     │
│  │  (MQTT Layer)   │    │  (QUIC Layer)   │                     │
│  └─────────────────┘    └─────────┬───────┘                     │
│                                   │                             │
│  ┌─────────────────┐              │                             │
│  │  config_loader  │              │                             │
│  │  (.env/yaml)    │──────────────┤                             │
│  └─────────────────┘              │                             │
│                                   │                             │
└───────────────────────────────────┼─────────────────────────────┘
                                    │
                                    │  MQTT over QUIC
                                    │  (UDP :14567)
                                    ▼
                         ┌──────────────────────┐
                         │   TESAIoT Platform   │
                         │  ┌────────────────┐  │
                         │  │  EMQX Broker   │  │
                         │  │  (QUIC + TLS)  │  │
                         │  └────────────────┘  │
                         └──────────────────────┘
```

## QUIC vs TCP Comparison

```
Traditional MQTT over TCP:
┌─────────┐    ┌─────────┐    ┌────────┐    ┌────────┐
│  TCP    │───▶│  TLS    │───▶│  MQTT  │───▶│  Data  │
│Handshake│    │Handshake│    │CONNECT │    │Transfer│
│ 1 RTT   │    │ 2 RTT   │    │ 1 RTT  │    │        │
└─────────┘    └─────────┘    └────────┘    └────────┘
                Total: ~4 RTT

MQTT over QUIC:
┌─────────────────────────────┐    ┌────────┐
│   QUIC (UDP + Crypto)       │───▶│  Data  │
│   0-RTT or 1-RTT            │    │Transfer│
└─────────────────────────────┘    └────────┘
        Total: ~1 RTT (50% faster)
```

## Connection Flow

```
Device                                           EMQX Broker
  │                                                   │
  │──────── QUIC Initial (Client Hello) ────────────▶ │
  │                                                   │
  │◀─────── QUIC Initial (Server Hello) ───────────── │
  │         + Encrypted Extensions                    │
  │         + Certificate                             │
  │         + Finished                                │
  │                                                   │
  │──────── QUIC Handshake (Finished) ──────────────▶ │
  │                                                   │
  │═════════ QUIC Connection Established ════════════ │
  │                                                   │
  │──────── MQTT CONNECT ───────────────────────────▶ │
  │                                                   │
  │◀─────── MQTT CONNACK ──────────────────────────── │
  │                                                   │
```

## QUIC Benefits

| Feature | TCP/TLS | QUIC |
|---------|---------|------|
| Handshake RTT | 3-4 RTT | 1 RTT (0-RTT resume) |
| Head-of-line blocking | Yes | No (multiplexed) |
| Connection migration | No | Yes (IP change OK) |
| Packet loss handling | Per-connection | Per-stream |

## Key Components

| Component | Description | Location |
|-----------|-------------|----------|
| C Client | Native QUIC implementation | `c_cpp/` |
| Python Client | aioquic-based client | `python/` |
| Credentials | CA cert and config | `credential_certificates/` |

## Python Implementation

```python
from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration

config = QuicConfiguration(
    alpn_protocols=["mqtt"],
    is_client=True
)
config.load_verify_locations("ca.crt")

async with connect(
    "mqtt.tesaiot.com", 14567,
    configuration=config
) as protocol:
    # MQTT over QUIC connection established
    await mqtt_connect(protocol)
```

## Dependencies

### Python
- aioquic - QUIC protocol implementation
- paho-mqtt - MQTT protocol
- python-dotenv - Configuration

### C/C++
- ngtcp2 - QUIC library
- nghttp3 - HTTP/3 library
- OpenSSL 3.0+ - QUIC support

## NCSA Compliance

| Level | Requirement | Implementation |
|-------|-------------|----------------|
| Level 1 | Encrypted transport | QUIC built-in TLS 1.3 |
| Level 1 | Server verification | CA certificate |
| Level 2 | Modern crypto | TLS 1.3 mandatory |
