# TESAIoT Platform - Open Source Examples

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-TESAIoT-green.svg)](https://www.tesaiot.com)
[![Examples](https://img.shields.io/badge/Examples-22%20Tested-brightgreen.svg)](./examples/)

Official integration examples for TESAIoT Platform - a secure, enterprise-grade IoT platform for device management, telemetry, and AI-powered analytics.

## Platform Architecture Overview

All examples connect to TESAIoT Platform's 14 core services. Here's a simplified view:

```ini
                              ┌─────────────────────────────────────┐
                              │       TESAIoT Platform              │
┌─────────────┐               │                                     │
│   Device    │──MQTTS/QUIC──►│  TESA MQTT Broker ──► TESA Bridge   │
│  Examples   │               │       │              │              │
│  #4-9,21-22 │               │       ▼              ▼              │
└─────────────┘               │  TimescaleDB    TESA Core API       │
                              │  (telemetry)    (FastAPI)           │
┌─────────────┐               │                     │               │
│   REST API  │───HTTPS──────►│ TESA Proxy Server ──► TESA Core API │
│  Examples   │               │       │             │               │
│  #1-3,10-14 │               │       ▼             ▼               │
└─────────────┘               │   TESA API Gateway   MongoDB        │
                              │   (gateway)     (metadata)          │
┌─────────────┐               │                                     │
│  Real-time  │────WSS───────►│ TESA Proxy Server ──► WebSocket B2B │
│  Examples   │               │                     │               │
│  #15-17     │               │                     ▼               │
└─────────────┘               │            TESA MQTT Broker (sub)   │
                              └─────────────────────────────────────┘
```

**[View Full Architecture Documentation](./ARCHITECTURE.md)** - Detailed diagrams, service mapping, and security layers.

### Key Services Used by Examples

| Service | Port | Protocol | Used By Examples |
|---------|------|----------|------------------|
| **TESA MQTT Broker** | 8883, 14567 | MQTTS, QUIC | #4-9, #21-22 (MQTT clients) |
| **TESA Core API** | 443 (via nginx) | HTTPS | #1-3, #10-14 (REST API) |
| **TESA Proxy Server** | 443 | HTTPS/WSS | All examples (SSL termination) |
| **WebSocket B2B** | 443/ws | WSS | #15-17 (real-time streaming) |
| **TESA Vault PKI** | 8200 | HTTPS | mTLS examples (#6-7) for PKI |

### Security Authentication Methods

| Method | Security Level | Examples | Description |
|--------|----------------|----------|-------------|
| **mTLS** | Highest | #6-7 | Mutual TLS with client certificate |
| **Server-TLS + API Key** | Standard | #1-5, #8 | TLS + X-API-Key header |
| **JWT Token** | Application | #10-14 | Bearer token authentication |

**[Read more about Security Architecture](./ARCHITECTURE.md#security-architecture)**

---

## Examples Overview

### Embedded Devices - Entry Level

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 1 | [python-cli](./examples/embedded-devices/entry/python-cli/) | Python | REST API client with CLI interface |
| 2 | [nodejs-mqtt](./examples/embedded-devices/entry/nodejs-mqtt/) | Node.js | MQTT telemetry publisher |
| 3 | [micropython-esp32](./examples/embedded-devices/entry/micropython-esp32/) | MicroPython | ESP32 with DHT22 sensor |
| 4 | [python-mqtt](./examples/embedded-devices/entry/python-mqtt/) | Python | Basic MQTT with paho-mqtt |
| 5 | [mqtt-quic-python](./examples/embedded-devices/entry/mqtt-quic-python/) | Python | MQTT over QUIC with aioquic |

### Embedded Devices - Intermediate Level

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 6 | [device-mtls/CLI_Python](./examples/embedded-devices/intermediate/device-mtls/CLI_Python/) | Bash/Python | mTLS MQTT CLI with mosquitto_pub |
| 7 | [device-mtls/Linux_C](./examples/embedded-devices/intermediate/device-mtls/Linux_C/) | C | mTLS MQTT client with libmosquitto |
| 8 | [device-servertls](./examples/embedded-devices/intermediate/device-servertls/) | C | Server TLS with API key auth |

### Integrations

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 9 | [nodered-integration](./examples/integrations/nodered-integration/) | Node-RED | Visual workflow integration |
| 10 | [mqtt-quic-c](./examples/integrations/mqtt-integration/mqtt-quic-c/) | C | NanoSDK MQTT over QUIC |
| 11 | [wss-mqtt-streaming](./examples/integrations/wss-mqtt-streaming/) | Node.js | Real-time MQTT via WebSocket Secure |

### Analytics API

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 12 | [javascript](./examples/analytics-api/javascript/) | JavaScript | Fetch-based API client |
| 13 | [python](./examples/analytics-api/python/) | Python | httpx async API client |
| 14 | [rust](./examples/analytics-api/rust/) | Rust | reqwest async API client |

### Applications - Real-time

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 15 | [react-dashboard](./examples/applications/real-time/react-dashboard/) | React/TS | Edge AI visualization dashboard |
| 16 | [live-streaming-dashboard](./examples/applications/real-time/live-streaming-dashboard/) | React/TS | Real-time MQTT dashboard |
| 17 | [wss-python-streaming](./examples/applications/real-time/wss-python-streaming/) | Python | WebSocket Secure streaming client |

### Applications - Visualization

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 18 | [grafana-influxdb](./examples/applications/visualization/grafana-influxdb/) | Grafana | InfluxDB time-series dashboards |
| 19 | [grafana-timescaledb](./examples/applications/visualization/grafana-timescaledb/) | Grafana | TimescaleDB real-time dashboards |

### Applications - Automation

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 20 | [ai-service-template](./examples/applications/automation/ai-service-template/) | Python | Third-party AI service template |

### Advanced

| # | Example | Language | Description |
|---|---------|----------|-------------|
| 21 | [mqtt-quic/c_cpp](./examples/embedded-devices/advanced/mqtt-quic/c_cpp/) | C/C++ | TCP fallback, 0-RTT, multi-stream |
| 22 | [mqtt-quic/python](./examples/embedded-devices/advanced/mqtt-quic/python/) | Python | Advanced aioquic with TCP fallback |

---

## Choosing the Right Example

| Your Use Case | Recommended Example | Why |
|---------------|---------------------|-----|
| **Just starting out** | #1 python-cli | Simple REST API, no MQTT setup |
| **Basic MQTT telemetry** | #4 python-mqtt | Standard MQTT with paho-mqtt |
| **Best performance** | #5 mqtt-quic-python | QUIC is faster than TCP |
| **Maximum security** | #6 device-mtls | mTLS for zero-trust |
| **Production MQTT** | #21-22 advanced | Auto-reconnect, fallback, monitoring |
| **Real-time dashboard** | #16 live-streaming | WebSocket streaming with React |
| **Analytics queries** | #12-14 analytics-api | Query historical telemetry |

**[See detailed Example-to-Service mapping](./ARCHITECTURE.md#example-to-service-mapping)**

---

## Quick Start

### 1. Get Your Credentials

1. Sign up at [admin.tesaiot.com](https://admin.tesaiot.com)
2. Create an organization
3. Register a device
4. Download credentials bundle (choose based on your example):
   - **Server-TLS HTTPS** - For REST API examples (#1-3)
   - **Server-TLS MQTT** - For basic MQTT (#4, #8)
   - **mTLS MQTT** - For secure MQTT (#6-7)
   - **MQTT-QUIC** - For QUIC examples (#5, #9, #21-22)

### 2. Choose an Example

```bash
# Clone the repository
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub/examples

# Navigate to your chosen example
cd embedded-devices/entry/python-cli

# Follow the example's README
cat README.md
```

### 3. Configure and Run

Each example includes:

- `.env.example` - Configuration template
- `README.md` - Quick start guide
- `HOWTO.md` - Detailed tutorial
- `ARCHITECTURE.md` - System design explanation

---

## Platform Features

TESAIoT Platform provides:

- **Device Management** - Register, configure, and monitor IoT devices
- **Secure Connectivity** - mTLS, TLS 1.3, MQTT over QUIC
- **Real-time Telemetry** - Sub-second data streaming via MQTT/WebSocket
- **AI/ML Integration** - Edge AI inference and anomaly detection
- **Certificate Management** - Automated PKI with Vault integration
- **API Gateway** - Rate limiting, authentication, routing via APISIX

## Protocol Support

| Protocol | Port | Use Case | Examples |
|----------|------|----------|----------|
| **MQTTS** | 8883 | Secure MQTT over TLS | #4, #6-8 |
| **MQTT-QUIC** | 14567 | Low-latency, 0-RTT | #5, #9, #21-22 |
| **HTTPS** | 443 | REST API calls | #1-3, #10-14 |
| **WSS** | 443 | Real-time streaming | #15-17 |

**[View full Protocol & Port mapping](./ARCHITECTURE.md#protocol-support)**

## Security Standards

- ETSI EN 303 645 (Consumer IoT Security)
- ISO/IEC 27402 (IoT Security Guidelines)
- NCSA Thailand IoT Security Level 1-3
- TLS 1.2+ enforced for all connections
- Zero-trust architecture

---

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.

### Setup Git Hooks (Required for Contributors)

After cloning, enable the commit validation hooks:

```bash
# Enable repository hooks
git config core.hooksPath .githooks

# Verify hooks are active
ls -la .githooks/
```

This ensures commit messages follow our standards and prevents accidental inclusion of AI tool attributions.

### Contribution Areas

- Bug fixes and improvements
- New example implementations
- Documentation enhancements
- Translations

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](./LICENSE) file for details.

### Attribution Requirement

If you use this code, please include attribution to TESAIoT Platform. See [NOTICE](./NOTICE) for details.

**Example attribution:**

```markdown
Built with [TESAIoT Platform Examples](https://github.com/tesaiot/developer-hub/tree/main/examples)
Copyright 2025 TESAIoT Platform by TESA
```

## Support

- **Issues**: [GitHub Issues](https://github.com/tesaiot/developer-hub/issues)
- **Discussions**: [GitHub Discussions](https://github.com/tesaiot/developer-hub/discussions)

---

**Built with security in mind for the IoT future.**

Copyright 2025 TESAIoT Platform by TESA
