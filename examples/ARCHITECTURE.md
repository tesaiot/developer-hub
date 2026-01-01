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
│  │  │   tesa-nginx    │  │   tesa-apisix   │  │         tesa-emqx                │  │   │
│  │  │   (Reverse      │  │   (API Gateway) │  │   (MQTT Broker + QUIC)           │  │   │
│  │  │    Proxy)       │  │                 │  │                                  │  │   │
│  │  │   Port 443      │  │   Port 9180     │  │   Ports: 1883, 8883, 8084,       │  │   │
│  │  │   (HTTPS)       │  │   (Admin API)   │  │          14567 (QUIC)            │  │   │
│  │  └────────┬────────┘  └────────┬────────┘  └────────────────┬─────────────────┘  │   │
│  └───────────┼────────────────────┼────────────────────────────┼────────────────────┘   │
│              │                    │                            │                        │
│  ┌───────────┼────────────────────┼────────────────────────────┼────────────────────┐   │
│  │           ▼                    ▼                            ▼                    │   │
│  │  ┌──────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                         APPLICATION LAYER                                    ││   │
│  │  │                                                                              ││   │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐   ││   │
│  │  │  │    tesa-api     │  │  tesa-admin-ui  │  │    tesa-mqtt-bridge         │   ││   │
│  │  │  │  (FastAPI)      │  │  (React + Vite) │  │  (Python MQTT ↔ API)        │   ││   │
│  │  │  │  Port 8000      │  │  Port 3000      │  │  Internal Only              │   ││   │
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
│  │  │  tesa-mongodb   │  │ tesa-timescaledb│  │        tesa-redis                │  │   │
│  │  │  (Document DB)  │  │ (Time-Series)   │  │        (Cache/Session)           │  │   │
│  │  │  Port 27017     │  │  Port 5432      │  │        Port 6379                 │  │   │
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
│  │  │   tesa-vault    │  │    tesa-etcd    │  │      tesa-prometheus             │  │   │
│  │  │  (PKI/Secrets)  │  │   (Config KV)   │  │      (Metrics)                   │  │   │
│  │  │   Port 8200     │  │   Port 2379     │  │      Port 9090                   │  │   │
│  │  │                 │  │                 │  │                                  │  │   │
│  │  │  - CA certs     │  │  - APISIX conf  │  │  - Service metrics               │  │   │
│  │  │  - Device certs │  │  - Routes       │  │  - Health checks                 │  │   │
│  │  │  - Signing keys │  │  - Plugins      │  │                                  │  │   │
│  │  └─────────────────┘  └─────────────────┘  └────────────────┬─────────────────┘  │   │
│  │                                                             │                    │   │
│  │                                                             ▼                    │   │
│  │                                            ┌──────────────────────────────────┐  │   │
│  │                                            │       tesa-grafana               │  │   │
│  │                                            │       (Dashboards)               │  │   │
│  │                                            │       Port 3001                  │  │   │
│  │                                            └──────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## Service Summary Table

| Service | Container | Port(s) | Protocol | Description |
|---------|-----------|---------|----------|-------------|
| **tesa-nginx** | Reverse Proxy | 80, 443 | HTTP/HTTPS | SSL termination, routing |
| **tesa-apisix** | API Gateway | 9080, 9180, 9443 | HTTP/HTTPS | Rate limiting, auth plugins |
| **tesa-emqx** | MQTT Broker | 1883, 8883, 8084, 14567 | MQTT/MQTTS/WSS/QUIC | Real-time messaging |
| **tesa-api** | REST API | 8000 | HTTP | Device/User/Telemetry API |
| **tesa-admin-ui** | Web UI | 3000 | HTTP | Admin dashboard |
| **tesa-mqtt-bridge** | Bridge | Internal | - | MQTT ↔ API integration |
| **tesa-python-websocket-b2b** | WebSocket | 8765 | WSS | Real-time streaming |
| **tesa-mongodb** | Document DB | 27017 | MongoDB | Devices, users, orgs |
| **tesa-timescaledb** | Time-Series | 5432 | PostgreSQL | Telemetry, analytics |
| **tesa-redis** | Cache | 6379 | Redis | Sessions, rate limiting |
| **tesa-vault** | Secrets | 8200 | HTTP | PKI, certificates |
| **tesa-etcd** | Config | 2379 | gRPC | APISIX configuration |
| **tesa-prometheus** | Metrics | 9090 | HTTP | Service monitoring |
| **tesa-grafana** | Dashboards | 3001 | HTTP | Visualization |

---

## Data Flow Diagrams

### 1. Device Telemetry Flow (MQTT Path)

```ini
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         MQTT TELEMETRY DATA FLOW                                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────┐     MQTTS/QUIC      ┌─────────────┐     Internal    ┌───────────┐  │
│  │   Device    │────────────────────►│  tesa-emqx  │────────────────►│  Bridge   │  │
│  │ (IoT/MCU)   │   Port 8883/14567   │             │   MQTT Sub      │  (Python) │  │
│  └─────────────┘                     └─────────────┘                 └─────┬─────┘  │
│                                                                            │        │
│  Topic: device/{device_id}/telemetry                                       │        │
│  Payload: {"device_id":"...","timestamp":"...","data":{...}}               │        │
│                                                                            ▼        │
│                                                                     ┌───────────┐   │
│                                                                     │ tesa-api  │   │
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
│  │   Client    │──────────────────►│ tesa-nginx  │─────────────►│  tesa-apisix  │   │
│  │ (Python/JS) │   Port 443        │ (SSL Term)  │  /api/v1/*   │  (Gateway)    │   │
│  └─────────────┘                   └─────────────┘              └───────┬───────┘   │
│                                                                         │           │
│  Headers:                                                               │           │
│  • X-API-Key: <device_api_key>                                          │           │
│  • Authorization: Bearer <jwt_token>                                    ▼           │
│                                                                  ┌───────────┐      │
│                                                                  │ tesa-api  │      │
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
│  │  Browser/   │───────────────────►│ tesa-nginx  │────────────►│   WebSocket   │   │
│  │   Client    │   wss://...        │             │   /ws/*     │   B2B Server  │   │
│  └─────────────┘                    └─────────────┘             └───────┬───────┘   │
│                                                                         │           │
│  URL: wss://admin.tesaiot.com/ws/v1/stream/mqtt?api_token=...           │           │
│                                                                         │           │
│                                                                         ▼           │
│                                                               ┌─────────────────┐   │
│                                                               │    tesa-emqx    │   │
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
│  │  │   Device    │                              │      tesa-emqx              ││ │
│  │  │             │     1. TLS ClientHello       │                             ││ │
│  │  │ client_cert │─────────────────────────────►│  CA Chain validates         ││ │
│  │  │ client_key  │◄─────────────────────────────│  client certificate         ││ │
│  │  │ ca-chain    │     2. Server Cert +         │                             ││ │
│  │  │             │        CertificateRequest    │  Client validates           ││ │
│  │  │             │─────────────────────────────►│  server certificate         ││ │
│  │  │             │     3. Client Cert + Key     │                             ││ │
│  │  └─────────────┘                              └─────────────────────────────┘│ │
│  │                                                                              │ │
│  │  Port: 8883 (MQTTS)                                                          │ │
│  │  Examples: #6 device-mtls, #7 device-mtls/CLI_Python                         │ │
│  │  Security Level: ★★★★★ (Zero-Trust)                                          │ │
│  └──────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────┐ │
│  │  2. SERVER TLS + API KEY - Standard Security                                 │ │
│  │  ─────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                              │ │
│  │  ┌─────────────┐                              ┌─────────────────────────────┐│ │
│  │  │   Device    │     TLS Handshake            │      tesa-nginx             ││ │
│  │  │             │─────────────────────────────►│                             ││ │
│  │  │ ca-chain    │     (Server cert only)       │  Server presents cert       ││ │
│  │  │ api_key     │                              │  Client verifies            ││ │
│  │  │             │─────────────────────────────►│                             ││ │
│  │  │             │     API Request +            │  API Gateway validates      ││ │
│  │  │             │     X-API-Key header         │  API key (hashed in Redis)  ││ │
│  │  └─────────────┘                              └─────────────────────────────┘│ │
│  │                                                                              │ │
│  │  Port: 443 (HTTPS) or 8884 (MQTT with password)                              │ │
│  │  Examples: #1-3 python-cli, device-servertls, #4 rpi-servertls               │ │
│  │  Security Level: ★★★★☆ (Enterprise)                                          │ │
│  └──────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────────┐ │
│  │  3. JWT TOKEN - User/Application Authentication                              │ │
│  │  ─────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                              │ │
│  │  ┌─────────────┐     Login                    ┌─────────────────────────────┐│ │
│  │  │   User/App  │─────────────────────────────►│      tesa-api               ││ │
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
│                         │     Stored in: tesa-vault   │                             │
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

### Port Mapping

```ini
┌────────────────────────────────────────────────────────────────────────────────────┐
│                            PROTOCOL & PORT MAPPING                                 │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  MQTT Protocols (tesa-emqx)                                                 │   │
│  │  ─────────────────────────────────────────────────────────────────────────  │   │
│  │                                                                             │   │
│  │  Port 1883   │  MQTT (Plain)      │  Development only - NOT for production  │   │
│  │  Port 8883   │  MQTTS (TLS 1.2+)  │  Server-TLS or mTLS authentication      │   │
│  │  Port 8084   │  MQTT over WSS     │  Browser WebSocket connections          │   │
│  │  Port 14567  │  MQTT over QUIC    │  UDP-based, 0-RTT, multi-stream         │   │
│  │                                                                             │   │
│  │  Topic Format: device/{device_id}/telemetry[/subtopic]                      │   │
│  │  QoS Levels: 0 (at-most-once), 1 (at-least-once), 2 (exactly-once)          │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  HTTP/HTTPS Protocols (tesa-nginx + tesa-apisix)                            │   │
│  │  ─────────────────────────────────────────────────────────────────────────  │   │
│  │                                                                             │   │
│  │  Port 80    │  HTTP              │  Redirect to HTTPS only                  │   │
│  │  Port 443   │  HTTPS (TLS 1.2+)  │  All API and UI access                   │   │
│  │                                                                             │   │
│  │  Endpoints:                                                                 │   │
│  │  • https://admin.tesaiot.com/api/v1/*         REST API                      │   │
│  │  • https://admin.tesaiot.com/                 Admin UI                      │   │
│  │  • wss://admin.tesaiot.com/ws/v1/*            WebSocket                     │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  QUIC Protocol Benefits (Port 14567)                                        │   │
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
│  │  #1 python-cli              → tesa-nginx → tesa-api → MongoDB                 │ │
│  │  #2 device-servertls (C)    → tesa-nginx → tesa-api → MongoDB                 │ │
│  │  #3 device-servertls/CLI    → tesa-nginx → tesa-api → MongoDB                 │ │
│  │  #4 rpi-servertls           → tesa-emqx (8884) → Bridge → TimescaleDB         │ │
│  │  #5 mqtt-quic-python        → tesa-emqx (14567 QUIC) → Bridge → TimescaleDB   │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  EMBEDDED DEVICES - Intermediate (#6-8)                                       │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #6 device-mtls (C)         → tesa-emqx (8883 mTLS) → Bridge → TimescaleDB    │ │
│  │  #7 device-mtls/CLI         → tesa-emqx (8883 mTLS) → Bridge → TimescaleDB    │ │
│  │  #8 esp32-servertls         → tesa-emqx (8884) → Bridge → TimescaleDB         │ │
│  │                                                                               │ │
│  │  Security: Requires client certificate from Vault PKI                         │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  INTEGRATIONS (#9-11)                                                         │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #9 mqtt-quic-c             → tesa-emqx (14567 QUIC) → Bridge → TimescaleDB   │ │
│  │  #10 ai-service-template    → tesa-nginx → tesa-api → MongoDB/TimescaleDB     │ │
│  │  #11 n8n-automation         → tesa-nginx → tesa-api (webhook triggers)        │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  ANALYTICS API (#12-14)                                                       │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #12 analytics-api/python   → tesa-nginx → tesa-api → TimescaleDB (analytics) │ │
│  │  #13 analytics-api/js       → tesa-nginx → tesa-api → TimescaleDB (analytics) │ │
│  │  #14 analytics-api/rust     → tesa-nginx → tesa-api → TimescaleDB (analytics) │ │
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
│  │  #15 wss-mqtt-streaming/py  → tesa-nginx → WebSocket B2B → EMQX (subscribe)   │ │
│  │  #16 wss-mqtt-streaming/js  → tesa-nginx → WebSocket B2B → EMQX (subscribe)   │ │
│  │  #17 live-streaming-dash    → tesa-nginx → WebSocket B2B → EMQX (subscribe)   │ │
│  │                                                                               │ │
│  │  WebSocket URL: wss://admin.tesaiot.com/ws/v1/stream/mqtt?api_token=...       │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  APPLICATIONS - Visualization (#18-19)                                        │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #18 react-dashboard        → tesa-nginx → tesa-api → TimescaleDB/MongoDB     │ │
│  │  #19 grafana-dashboard      → tesa-grafana → tesa-prometheus/TimescaleDB      │ │
│  │                                                                               │ │
│  │  Grafana Data Sources:                                                        │ │
│  │  • Prometheus: http://tesa-prometheus:9090                                    │ │
│  │  • TimescaleDB: postgresql://tesa-timescaledb:5432/tesaiot                    │ │
│  │                                                                               │ │
│  └───────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────┐ │
│  │  APPLICATIONS - Automation (#20)                                              │ │
│  │  ──────────────────────────────────────────────────────────────────────────── │ │
│  │                                                                               │ │
│  │  #20 nodered-integration    → tesa-emqx (subscribe) + tesa-api (HTTP)         │ │
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
│  │  #21 mqtt-quic/c_cpp        → tesa-emqx (14567 QUIC + 8884 TCP fallback)      │ │
│  │  #22 mqtt-quic/python       → tesa-emqx (14567 QUIC + 8884 TCP fallback)      │ │
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
