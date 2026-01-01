# TESAIoT Platform - Overall Architecture for Open Source Examples

This document provides a comprehensive overview of how all 22 Open Source Examples integrate with TESAIoT Platform services.

## Platform Architecture Overview

```ini
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              TESAIoT Platform (14 Core Services)                        │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐   │
│  │                           EXTERNAL ACCESS LAYER                                  │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────────────────────┐  │   │
│  │  │ TESA Proxy     │  │ TESA API        │  │     TESA MQTT Broker             │  │   │
│  │  │   Server       │  │   Gateway       │  │   (MQTT Broker + QUIC)           │  │   │
│  │  │   (Reverse     │  │   (Rate Limit)  │  │   (Secure Protocols)             │  │   │
│  │  │    Proxy)      │  │   (Auth Plugin) │  │   (MQTTS/WSS/QUIC)               │  │   │
│  │  │                 │  │                 │  │                                  │  │   │
│  │  └────────┬────────┘  └────────┬────────┘  └────────────────┬─────────────────┘  │   │
│  └───────────┼────────────────────┼────────────────────────────┼────────────────────┘   │
│              │                    │                            │                        │
│  ┌───────────┼────────────────────┼────────────────────────────┼────────────────────┐   │
│  │           ▼                    ▼                            ▼                    │   │
│  │  ┌──────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                         APPLICATION LAYER                                    ││   │
│  │  │                                                                              ││   │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐   ││   │
│  │  │  │  TESA Core API  │  │   TESA Admin    │  │      TESA Bridge            │   ││   │
│  │  │  │  (REST API)     │  │  (Admin Console)│  │  (MQTT ↔ API Integration)   │   ││   │
│  │  │  │  (FastAPI)      │  │  (React + Vite) │  │  (Internal Service)         │   ││   │
│  │  │  └────────┬────────┘  └─────────────────┘  └──────────────┬──────────────┘   ││   │
│  │  │           │                                               │                  ││   │
│  │  │  ┌────────┴───────────────────────────────────────────────┴────────────────┐ ││   │
│  │  │  │                  (WebSocket Streaming Service)                          │ ││   │
│  │  │  │                  wss://admin.tesaiot.com/ws/v1/...                      │ ││   │
│  │  │  └─────────────────────────────────────────────────────────────────────────┘ ││   │
│  │  └──────────────────────────────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              DATA LAYER                                          │   │
│  │                                                                                  │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────────────────────┐  │   │
│  │  │ TESA DataStore  │  │  TESA TimeDB    │  │        TESA Cache                │  │   │
│  │  │  (Document DB)  │  │ (Time-Series)   │  │        (Cache/Session)           │  │   │
│  │  │  (Metadata)     │  │  (Telemetry)    │  │        (Rate Limiting)           │  │   │
│  │  │                 │  │                 │  │                                  │  │   │
│  │  │  - Devices      │  │  - Telemetry    │  │  - API Key validation            │  │   │
│  │  │  - Users        │  │  - Analytics    │  │  - Session tokens                │  │   │
│  │  │  - Organizations│  │  - Metrics      │  │  - Rate limiting                 │  │   │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐   │
│  │                           SECURITY & CONFIG LAYER                                │   │
│  │                                                                                  │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────────────────────┐  │   │
│  │  │ TESA Vault PKI  │  │  TESA Config    │  │      TESA Metrics                │  │   │
│  │  │  (PKI/Secrets)  │  │   (Config KV)   │  │      (Metrics)                   │  │   │
│  │  │  (Certificates) │  │   (API Gateway) │  │      (Monitoring)                │  │   │
│  │  │                 │  │                 │  │                                  │  │   │
│  │  │  - CA certs     │  │  - APISIX conf  │  │  - Service metrics               │  │   │
│  │  │  - Device certs │  │  - Routes       │  │  - Health checks                 │  │   │
│  │  │  - Signing keys │  │  - Plugins      │  │                                  │  │   │
│  │  └─────────────────┘  └─────────────────┘  └────────────────┬─────────────────┘  │   │
│  │                                                             │                    │   │
│  │                                                             ▼                    │   │
│  │                                            ┌──────────────────────────────────┐  │   │
│  │                                            │      TESA Dashboards             │  │   │
│  │                                            │       (Dashboards)               │  │   │
│  │                                            │       (Visualization)            │  │   │
│  │                                            └──────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## Service Summary Table

| Service | Role | Protocol | Description |
|---------|------|----------|-------------|
| **TESA Proxy Server** | Reverse Proxy | HTTPS/WSS | SSL termination, secure routing |
| **TESA API Gateway** | API Gateway | HTTPS | Rate limiting, auth plugins |
| **TESA MQTT Broker** | MQTT Broker | MQTTS/WSS/QUIC | Secure real-time messaging |
| **TESA Core API** | REST API | HTTP (internal) | Device/User/Telemetry API |
| **TESA Admin** | Web UI | HTTP (internal) | Admin dashboard |
| **TESA Bridge** | Bridge | Internal | MQTT ↔ API integration |
| **TESA WebSocket** | WebSocket | WSS | Real-time streaming |
| **TESA DataStore** | Document DB | Internal | Devices, users, organizations |
| **TESA TimeDB** | Time-Series DB | Internal | Telemetry, analytics |
| **TESA Cache** | Cache | Internal | Sessions, rate limiting |
| **TESA Vault PKI** | Secrets | HTTPS | PKI, certificates |
| **TESA Config** | Config Store | Internal | API Gateway configuration |
| **TESA Metrics** | Metrics | HTTP (internal) | Service monitoring |
| **TESA Dashboards** | Dashboards | HTTP (internal) | Visualization |

> **Note:** All external-facing services use TLS encryption. Internal services are not exposed publicly.

---

## Data Flow Diagrams

### 1. Device Telemetry Flow (MQTT Path)

```ini
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         MQTT TELEMETRY DATA FLOW                                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────┐     MQTTS/QUIC      ┌─────────────┐     Internal    ┌───────────┐  │
│  │   Device    │────────────────────►│  TESA MQTT Broker  │────────────────►│  Bridge   │  │
│  │ (IoT/MCU)   │   Secure Protocol   │             │   MQTT Sub      │  (Python) │  │
│  └─────────────┘                     └─────────────┘                 └─────┬─────┘  │
│                                                                            │        │
│  Topic: device/{device_id}/telemetry                                       │        │
│  Payload: {"device_id":"...","timestamp":"...","data":{...}}               │        │
│                                                                            ▼        │
│                                                                     ┌───────────┐   │
│                                                                     │ TESA Core API  │   │
│                                                                     │ (FastAPI) │   │
│                                                                     └─────┬─────┘   │
│                                                                           │         │
│                    ┌──────────────────────────────────────────────────────┤         │
│                    ▼                                                      ▼         │
│             ┌─────────────┐                                        ┌───────────┐    │
│             │ TimescaleDB │  Time-series storage                   │  MongoDB  │    │
│             │ (telemetry) │  for analytics                         │ (devices) │    │
│             └─────────────┘                                        └───────────┘    │
│                                                                                     │
│  Examples using this flow:                                                          │
│  • #4 rpi-servertls       - MQTT + Server TLS                                       │
│  • #5 mqtt-quic-python    - MQTT over QUIC                                          │
│  • #6 device-mtls         - MQTT + Mutual TLS                                       │
│  • #7 device-mtls/CLI     - MQTT + mTLS (mosquitto_pub)                             │
│  • #9 mqtt-quic-c         - MQTT over QUIC (C)                                      │
│  • #21 mqtt-quic/c_cpp    - Advanced QUIC                                           │
│  • #22 mqtt-quic/python   - Advanced QUIC                                           │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 2. REST API Flow (HTTPS Path)

```ini
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           REST API DATA FLOW                                        │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────┐      HTTPS        ┌─────────────┐    Route     ┌───────────────┐   │
│  │   Client    │──────────────────►│ TESA Proxy Server  │─────────────►│  TESA API Gateway  │   │
│  │ (Python/JS) │   Secure HTTPS    │ (SSL Term)  │  /api/v1/*   │  (Gateway)    │   │
│  └─────────────┘                   └─────────────┘              └───────┬───────┘   │
│                                                                         │           │
│  Headers:                                                               │           │
│  • X-API-Key: <device_api_key>                                          │           │
│  • Authorization: Bearer <jwt_token>                                    ▼           │
│                                                                  ┌───────────┐      │
│                                                                  │ TESA Core API  │      │
│                                                                  │ (FastAPI) │      │
│                                                                  └─────┬─────┘      │
│                                                                        │            │
│                    ┌───────────────────────────────────────────────────┤            │
│                    ▼                          ▼                        ▼            │
│             ┌───────────┐              ┌───────────┐            ┌───────────┐       │
│             │  MongoDB  │              │   Redis   │            │TimescaleDB│       │
│             │(metadata) │              │ (cache)   │            │(telemetry)│       │
│             └───────────┘              └───────────┘            └───────────┘       │
│                                                                                     │
│  Examples using this flow:                                                          │
│  • #1 python-cli          - REST API client                                         │
│  • #2 device-servertls    - HTTPS + API Key (C)                                     │
│  • #3 device-servertls/CLI- HTTPS + API Key (Python)                                │
│  • #10 ai-service-template- Third-party AI service                                  │
│  • #12-14 analytics-api/* - Python/JS/Rust API clients                              │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 3. Real-time WebSocket Flow

```ini
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        WEBSOCKET STREAMING DATA FLOW                                │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────┐        WSS         ┌─────────────┐   Upgrade   ┌───────────────┐   │
│  │  Browser/   │───────────────────►│ TESA Proxy Server  │────────────►│   WebSocket   │   │
│  │   Client    │   wss://...        │             │   /ws/*     │   B2B Server  │   │
│  └─────────────┘                    └─────────────┘             └───────┬───────┘   │
│                                                                         │           │
│  URL: wss://admin.tesaiot.com/ws/v1/stream/mqtt?api_token=...           │           │
│                                                                         │           │
│                                                                         ▼           │
│                                                               ┌─────────────────┐   │
│                                                               │    TESA MQTT Broker    │   │
│                                                               │  (MQTT Sub)     │   │
│                                                               └────────┬────────┘   │
│                                                                        │            │
│                                                                        ▼            │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                     Real-time Data Stream                                   │    │
│  │                                                                             │    │
│  │   {"type":"telemetry","device_id":"...","data":{"temp":25.5,...}}           │    │
│  │   {"type":"telemetry","device_id":"...","data":{"temp":25.6,...}}           │    │
│  │   {"type":"telemetry","device_id":"...","data":{"temp":25.7,...}}           │    │
│  │                                                                             │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                     │
│  Examples using this flow:                                                          │
│  • #15 wss-mqtt-streaming/python  - Python WebSocket client                         │
│  • #16 wss-mqtt-streaming/nodejs  - Node.js WebSocket client                        │
│  • #17 live-streaming-dashboard   - React real-time dashboard                       │
│  • #18 react-dashboard            - Edge AI dashboard                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Security Architecture

### Authentication Methods

```ini
┌───────────────────────────────────────────────────────────────────────────────────┐
│                         SECURITY AUTHENTICATION LAYERS                            │
├───────────────────────────────────────────────────────────────────────────────────┤
│                                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────┐ │
│  │  1. MUTUAL TLS (mTLS) - Strongest Security                                   │ │
│  │  ─────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                              │ │
│  │  ┌─────────────┐                              ┌─────────────────────────────┐│ │
│  │  │   Device    │                              │      TESA MQTT Broker              ││ │
│  │  │             │     1. TLS ClientHello       │                             ││ │
│  │  │ client_cert │─────────────────────────────►│  CA Chain validates         ││ │
│  │  │ client_key  │◄─────────────────────────────│  client certificate         ││ │
│  │  │ ca-chain    │     2. Server Cert +         │                             ││ │
│  │  │             │        CertificateRequest    │  Client validates           ││ │
│  │  │             │─────────────────────────────►│  server certificate         ││ │
│  │  │             │     3. Client Cert + Key     │                             ││ │
│  │  └─────────────┘                              └─────────────────────────────┘│ │
│  │                                                                              │ │
│  │  Protocol: MQTTS (Mutual TLS)                                                │ │
│  │  Examples: #6 device-mtls, #7 device-mtls/CLI_Python                         │ │
│  │  Security Level: ★★★★★ (Zero-Trust)                                          │ │
│  └──────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────┐ │
│  │  2. SERVER TLS + API KEY - Standard Security                                 │ │
│  │  ─────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                              │ │
│  │  ┌─────────────┐                              ┌─────────────────────────────┐│ │
│  │  │   Device    │     TLS Handshake            │      TESA Proxy Server             ││ │
│  │  │             │─────────────────────────────►│                             ││ │
│  │  │ ca-chain    │     (Server cert only)       │  Server presents cert       ││ │
│  │  │ api_key     │                              │  Client verifies            ││ │
│  │  │             │─────────────────────────────►│                             ││ │
│  │  │             │     API Request +            │  API Gateway validates      ││ │
│  │  │             │     X-API-Key header         │  API key (hashed in Redis)  ││ │
│  │  └─────────────┘                              └─────────────────────────────┘│ │
│  │                                                                              │ │
│  │  Protocol: HTTPS or MQTTS (Server TLS)                                       │ │
│  │  Examples: #1-3 python-cli, device-servertls, #4 rpi-servertls               │ │
│  │  Security Level: ★★★★☆ (Enterprise)                                          │ │
│  └──────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────┐ │
│  │  3. JWT TOKEN - User/Application Authentication                              │ │
│  │  ─────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                              │ │
│  │  ┌─────────────┐     Login                    ┌─────────────────────────────┐│ │
│  │  │   User/App  │─────────────────────────────►│      TESA Core API               ││ │
│  │  │             │     POST /api/v1/auth/login  │                             ││ │
│  │  │             │◄─────────────────────────────│  Returns JWT token          ││ │
│  │  │             │     {access_token: "..."}    │  (exp: 24h)                 ││ │
│  │  │             │                              │                             ││ │
│  │  │             │─────────────────────────────►│  Validate JWT signature     ││ │
│  │  │             │     Authorization: Bearer... │  Check expiration           ││ │
│  │  └─────────────┘                              └─────────────────────────────┘│ │
│  │                                                                              │ │
│  │  Used by: Admin UI, Third-party services, Analytics clients                  │ │
│  │  Examples: #10 ai-service-template, #12-14 analytics-api/*                   │ │
│  │  Security Level: ★★★☆☆ (Application)                                         │ │
│  └──────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                   │
└───────────────────────────────────────────────────────────────────────────────────┘
```

### Certificate Chain (PKI)

```ini
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                          PKI CERTIFICATE HIERARCHY                                  │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│                         ┌─────────────────────────────┐                             │
│                         │     TESAIoT Root CA         │                             │
│                         │     (Self-signed)           │                             │
│                         │     Validity: 10 years      │                             │
│                         │     Stored in: TESA Vault PKI   │                             │
│                         └──────────────┬──────────────┘                             │
│                                        │                                            │
│                    ┌───────────────────┼───────────────────┐                        │
│                    ▼                   ▼                   ▼                        │
│  ┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐            │
│  │  Intermediate CA    │ │  Intermediate CA    │ │  Intermediate CA    │            │
│  │  (Device Signing)   │ │  (Server Signing)   │ │  (CSR Signing)      │            │
│  │  Validity: 5 years  │ │  Validity: 5 years  │ │  Validity: 5 years  │            │
│  └──────────┬──────────┘ └──────────┬──────────┘ └──────────┬──────────┘            │
│             │                       │                       │                       │
│             ▼                       ▼                       ▼                       │
│  ┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐            │
│  │   Device Certs      │ │   Server Certs      │ │   Runtime Certs     │            │
│  │                     │ │                     │ │                     │            │
│  │ • client_cert.pem   │ │ • mqtt.tesaiot.com  │ │ • PSoC Edge         │            │
│  │ • client_key.pem    │ │ • admin.tesaiot.com │ │ • Trust M CSR       │            │
│  │ • Validity: 1 year  │ │ • Validity: 1 year  │ │ • Validity: 1 year  │            │
│  └─────────────────────┘ └─────────────────────┘ └─────────────────────┘            │
│                                                                                     │
│  Bundle Files (downloaded from Platform):                                           │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │  certs_credentials/                                                         │    │
│  │  ├── device_id.txt       # Device UUID                                      │    │
│  │  ├── client_cert.pem     # Device certificate (for mTLS)                    │    │
│  │  ├── client_key.pem      # Device private key (chmod 600)                   │    │
│  │  ├── ca-chain.pem        # Root + Intermediate CA chain                     │    │
│  │  └── endpoints.json      # Platform endpoints configuration                 │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Protocol Support

### Secure Protocols

```ini
┌────────────────────────────────────────────────────────────────────────────────────┐
│                           SECURE PROTOCOL SUPPORT                                  │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  MQTT Protocols (TESA MQTT Broker)                                                 │   │
│  │  ─────────────────────────────────────────────────────────────────────────  │   │
│  │                                                                             │   │
│  │  MQTTS        │  TLS 1.2+         │  Server-TLS or mTLS authentication      │   │
│  │  MQTT-WSS     │  WebSocket Secure │  Browser WebSocket connections          │   │
│  │  MQTT-QUIC    │  UDP + TLS 1.3    │  0-RTT, multi-stream, low latency       │   │
│  │                                                                             │   │
│  │  Topic Format: device/{device_id}/telemetry[/subtopic]                      │   │
│  │  QoS Levels: 0 (at-most-once), 1 (at-least-once), 2 (exactly-once)          │   │
│  │                                                                             │   │
│  │  Note: Only TLS-encrypted protocols are supported in production.            │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  HTTP/HTTPS Protocols (TESA Proxy Server + TESA API Gateway)                            │   │
│  │  ─────────────────────────────────────────────────────────────────────────  │   │
│  │                                                                             │   │
│  │  HTTPS        │  TLS 1.2+         │  All API and UI access                  │   │
│  │  WSS          │  WebSocket Secure │  Real-time streaming                    │   │
│  │                                                                             │   │
│  │  Endpoints:                                                                 │   │
│  │  • https://admin.tesaiot.com/api/v1/*         REST API                      │   │
│  │  • https://admin.tesaiot.com/                 Admin UI                      │   │
│  │  • wss://admin.tesaiot.com/ws/v1/*            WebSocket                     │   │
│  │                                                                             │   │
│  │  Note: HTTP requests are automatically redirected to HTTPS.                 │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  QUIC Protocol Benefits                                                     │   │
│  │  ─────────────────────────────────────────────────────────────────────────  │   │
│  │                                                                             │   │
│  │  Feature              │  TCP+TLS        │  QUIC                             │   │
│  │  ─────────────────────┼─────────────────┼─────────────────────────────────  │   │
│  │  Initial Connect      │  ~200ms         │  ~100ms (50% faster)              │   │
│  │  Reconnect (0-RTT)    │  ~200ms         │  ~30ms (85% faster)               │   │
│  │  Connection Migration │  Disconnect     │  Seamless (IP change OK)          │   │
│  │  Head-of-line Block   │  Yes            │  No (independent streams)         │   │
│  │  Packet Loss Recovery │  ~400ms         │  ~150ms (60% faster)              │   │
│  │                                                                             │   │
│  │  Ideal for: Mobile IoT, Vehicle IoT, Unreliable networks                    │   │
│  │  Examples: #5 mqtt-quic-python, #9 mqtt-quic-c, #21-22 advanced             │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                    │
└────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Example-to-Service Mapping

### Which Examples Connect to Which Services

```ini
┌────────────────────────────────────────────────────────────────────────────────────┐
│                        EXAMPLE → SERVICE DEPENDENCY MATRIX                         │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  EMBEDDED DEVICES - Entry Level (#1-5)                                        │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #1 python-cli              → TESA Proxy Server → TESA Core API → MongoDB                 │ │
│  │  #2 device-servertls (C)    → TESA Proxy Server → TESA Core API → MongoDB                 │ │
│  │  #3 device-servertls/CLI    → TESA Proxy Server → TESA Core API → MongoDB                 │ │
│  │  #4 rpi-servertls           → TESA MQTT Broker (MQTTS) → Bridge → TimescaleDB        │ │
│  │  #5 mqtt-quic-python        → TESA MQTT Broker (QUIC) → Bridge → TimescaleDB         │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  EMBEDDED DEVICES - Intermediate (#6-8)                                       │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #6 device-mtls (C)         → TESA MQTT Broker (mTLS) → Bridge → TimescaleDB         │ │
│  │  #7 device-mtls/CLI         → TESA MQTT Broker (mTLS) → Bridge → TimescaleDB         │ │
│  │  #8 esp32-servertls         → TESA MQTT Broker (MQTTS) → Bridge → TimescaleDB        │ │
│  │                                                                               │ │
│  │  Security: Requires client certificate from Vault PKI                         │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  INTEGRATIONS (#9-11)                                                         │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #9 mqtt-quic-c             → TESA MQTT Broker (QUIC) → Bridge → TimescaleDB         │ │
│  │  #10 ai-service-template    → TESA Proxy Server → TESA Core API → MongoDB/TimescaleDB     │ │
│  │  #11 n8n-automation         → TESA Proxy Server → TESA Core API (webhook triggers)        │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  ANALYTICS API (#12-14)                                                       │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #12 analytics-api/python   → TESA Proxy Server → TESA Core API → TimescaleDB (analytics) │ │
│  │  #13 analytics-api/js       → TESA Proxy Server → TESA Core API → TimescaleDB (analytics) │ │
│  │  #14 analytics-api/rust     → TESA Proxy Server → TESA Core API → TimescaleDB (analytics) │ │
│  │                                                                               │ │
│  │  Endpoints used:                                                              │ │
│  │  • GET /api/v1/analytics/devices/{id}/summary                                 │ │
│  │  • GET /api/v1/analytics/devices/{id}/time-series                             │ │
│  │  • GET /api/v1/analytics/devices/{id}/aggregates                              │ │
│  │  • GET /api/v1/analytics/devices/{id}/export                                  │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  APPLICATIONS - Real-time (#15-17)                                            │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #15 wss-mqtt-streaming/py  → TESA Proxy Server → WebSocket B2B → EMQX (subscribe)   │ │
│  │  #16 wss-mqtt-streaming/js  → TESA Proxy Server → WebSocket B2B → EMQX (subscribe)   │ │
│  │  #17 live-streaming-dash    → TESA Proxy Server → WebSocket B2B → EMQX (subscribe)   │ │
│  │                                                                               │ │
│  │  WebSocket URL: wss://admin.tesaiot.com/ws/v1/stream/mqtt?api_token=...       │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  APPLICATIONS - Visualization (#18-19)                                        │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #18 react-dashboard        → TESA Proxy Server → TESA Core API → TimescaleDB/MongoDB     │ │
│  │  #19 grafana-dashboard      → TESA Dashboards → TESA Metrics/TimescaleDB      │ │
│  │                                                                               │ │
│  │  Grafana Data Sources:                                                        │ │
│  │  • Prometheus: http://TESA Metrics:9090                                    │ │
│  │  • TimescaleDB: postgresql://TESA TimeDB:5432/tesaiot                    │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  APPLICATIONS - Automation (#20)                                              │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #20 nodered-integration    → TESA MQTT Broker (subscribe) + TESA Core API (HTTP)         │ │
│  │                                                                               │ │
│  │  Node-RED can:                                                                │ │
│  │  • Subscribe to MQTT topics from EMQX                                         │ │
│  │  • Call REST API endpoints                                                    │ │
│  │  • Trigger webhooks and automations                                           │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  ADVANCED (#21-22)                                                            │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #21 mqtt-quic/c_cpp        → TESA MQTT Broker (14567 QUIC + 8884 TCP fallback)      │ │
│  │  #22 mqtt-quic/python       → TESA MQTT Broker (14567 QUIC + 8884 TCP fallback)      │ │
│  │                                                                               │ │
│  │  Advanced Features:                                                           │ │
│  │  • 0-RTT session resumption (90% faster reconnect)                            │ │
│  │  • Multi-stream parallel publishing                                           │ │
│  │  • Automatic TCP fallback when QUIC unavailable                               │ │
│  │  • Connection health monitoring                                               │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
└────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Credential Bundle Types

### Download Options from Platform

```ini
┌────────────────────────────────────────────────────────────────────────────────────┐
│                        CREDENTIAL BUNDLE TYPES                                     │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  1. Server-TLS HTTPS Bundle                                                   │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  Contents:                                                                    │ │
│  │  • device_id.txt           Device UUID                                        │ │
│  │  • https-api-credentials.txt   API Key (X-API-Key header)                     │ │
│  │  • ca-chain.pem            CA certificate for server verification             │ │
│  │  • endpoints.json          API endpoints                                      │ │
│  │                                                                               │ │
│  │  Used by: #1 python-cli, #2-3 device-servertls                                │ │
│  │  Authentication: TLS + API Key header                                         │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  2. Server-TLS MQTT Bundle                                                    │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  Contents:                                                                    │ │
│  │  • device_id.txt           Device UUID (used as MQTT client ID)               │ │
│  │  • mqtt-credentials.txt    MQTT username/password                             │ │
│  │  • ca-chain.pem            CA certificate for server verification             │ │
│  │  • endpoints.json          MQTT broker endpoints                              │ │
│  │  • mqtt_client_config.h    C header for MCU integration                       │ │
│  │                                                                               │ │
│  │  Used by: #4 rpi-servertls, #8 esp32-servertls                                │ │
│  │  Authentication: TLS + MQTT username/password                                 │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  3. mTLS MQTT Bundle (Highest Security)                                       │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  Contents:                                                                    │ │
│  │  • device_id.txt           Device UUID                                        │ │
│  │  • client_cert.pem         Client X.509 certificate                           │ │
│  │  • client_key.pem          Client private key (chmod 600!)                    │ │
│  │  • ca-chain.pem            CA chain for mutual verification                   │ │
│  │  • endpoints.json          MQTT broker endpoints                              │ │
│  │                                                                               │ │
│  │  Used by: #6-7 device-mtls                                                    │ │
│  │  Authentication: Mutual TLS (both sides verify certificates)                  │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  4. MQTT-QUIC Bundle                                                          │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  Contents:                                                                    │ │
│  │  • device_id.txt           Device UUID                                        │ │
│  │  • ca-chain.pem            CA certificate                                     │ │
│  │  • endpoints.json          QUIC port (14567) + TCP fallback (8884)            │ │
│  │  • mqtt-quic-config.json   QUIC-specific settings                             │ │
│  │                                                                               │ │
│  │  Used by: #5 mqtt-quic-python, #9 mqtt-quic-c, #21-22 advanced                │ │
│  │  Authentication: Server-TLS with device_id/password                           │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
└────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick Reference: Getting Started

### 1. Choose Your Example Based on Use Case

| Use Case | Recommended Example | Difficulty |
|----------|---------------------|------------|
| REST API integration | #1 python-cli | Beginner |
| MQTT telemetry (simple) | #4 rpi-servertls | Beginner |
| MQTT with best performance | #5 mqtt-quic-python | Beginner |
| Maximum security (mTLS) | #6-7 device-mtls | Intermediate |
| Real-time dashboard | #17 live-streaming-dashboard | Intermediate |
| Analytics queries | #12-14 analytics-api | Beginner |
| Production MQTT client | #21-22 advanced mqtt-quic | Advanced |

### 2. Download Credentials

1. Login to https://admin.tesaiot.com
2. Navigate to **Devices** → Select device → **Credentials**
3. Download appropriate bundle for your example
4. Extract to example's `certs_credentials/` folder

### 3. Configure and Run

```bash
# Clone examples
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub/examples

# Navigate to chosen example
cd embedded-devices/entry/python-cli

# Copy credentials
cp ~/Downloads/*-bundle/* ./certs_credentials/

# Follow example README
cat README.md
```

---

## Security Compliance

TESAIoT Platform examples follow these security standards:

| Standard | Description | Examples Compliance |
|----------|-------------|---------------------|
| **ETSI EN 303 645** | Consumer IoT Security | All examples use TLS 1.2+ |
| **ISO/IEC 27402** | IoT Security Guidelines | mTLS examples (#6-7) |
| **NCSA Level 1-2** | Thailand IoT Security | #2, #4, #8 (Server-TLS) |
| **NCSA Level 3** | Thailand IoT Security | #6 device-mtls (mTLS) |

---

## Document Information

- **Version**: 1.0.0
- **Last Updated**: 2026-01-01
- **Examples Covered**: 22 tested examples
- **Platform Version**: v2026.01
