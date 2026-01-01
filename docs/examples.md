---
title: "Examples"
permalink: /examples/
toc: true
toc_sticky: true
---

# TESAIoT Examples

Browse our collection of open source examples to accelerate your IoT development.

## Device Examples

Examples for connecting devices to TESAIoT Platform.

### Python CLI Client

Simple command-line MQTT client for testing and scripting.

**Features**:
- Publish telemetry data
- Subscribe to commands
- mTLS and token authentication

**Use Cases**:
- Quick device testing
- Scripted data ingestion
- Development debugging

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/python-cli)

---

### Device mTLS

Production-ready device client with mutual TLS authentication.

**Features**:
- Certificate-based authentication
- Automatic reconnection
- Structured telemetry publishing

**Use Cases**:
- Production IoT devices
- High-security environments
- Infineon PSoC, STM32, Raspberry Pi deployments

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/device-mtls)

---

### Device Server TLS

Device client with server TLS and token authentication.

**Features**:
- Server certificate validation
- Token-based authentication
- Simplified setup

**Use Cases**:
- Development environments
- Shared test devices
- Quick prototyping

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/device-servertls)

---

### MQTT over QUIC (Entry-Level)

Entry-level MQTT client using QUIC transport protocol.

**Features**:
- QUIC transport for low latency
- Better performance on lossy networks
- Connection migration support

**Use Cases**:
- Mobile devices
- Unreliable networks
- Edge computing scenarios

**Languages**: Python, C/C++

[View Python Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/integrations/mqtt-integration/mqtt-quic-python)

[View C/C++ Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/integrations/mqtt-integration/mqtt-quic-c)

---

### MQTT over QUIC (Advanced)

Production-ready MQTT over QUIC clients with advanced features.

**Features**:
- Automatic QUIC → TCP+TLS fallback
- 0-RTT session resumption for fast reconnection
- Multi-stream parallel publishing
- Exponential backoff reconnection with jitter
- Connection health monitoring and statistics
- Thread-safe operations (C++) / asyncio-based (Python)

**Use Cases**:
- Production IoT deployments
- Environments with firewall restrictions (UDP blocked)
- Applications requiring fast reconnection (mobile, automotive)
- High-throughput multi-topic publishing

**Languages**: Python (aioquic), C/C++ (NanoSDK)

| Feature | Entry-Level | Advanced |
|---------|-------------|----------|
| Transport | QUIC only | QUIC + TCP+TLS fallback |
| Reconnection | Manual | Automatic with backoff |
| Multi-Stream | Single stream | Parallel publishing |
| Session Resume | None | 0-RTT support |
| Health Monitor | None | Built-in statistics |

[View Advanced Python →](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/advanced/mqtt-quic/python)

[View Advanced C/C++ →](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/advanced/mqtt-quic/c_cpp)

---

## Integration Templates

Ready-to-use integration patterns for common platforms.

### Node-RED Integration

Node-RED flows for TESAIoT Platform integration.

**Features**:
- Pre-built MQTT nodes
- Dashboard templates
- Alert and notification flows

**Use Cases**:
- Visual workflow automation
- Rapid prototyping
- Non-developer IoT projects

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/nodered-integration)

---

### React Dashboard

React-based IoT monitoring dashboard.

**Features**:
- Real-time data display
- Device management UI
- Responsive design

**Use Cases**:
- Custom monitoring portals
- White-label dashboards
- Admin interfaces

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/react-dashboard)

---

### WSS MQTT Streaming

WebSocket MQTT streaming for browser applications.

**Features**:
- Browser-based MQTT
- Real-time data streaming
- Token authentication

**Use Cases**:
- Web dashboards
- Single-page applications
- Real-time visualizations

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/wss-mqtt-streaming)

---

## AI Service Templates

Templates for building AI-powered IoT applications.

### AI Service Template

FastAPI-based AI inference service template.

**Features**:
- RESTful inference API
- Health monitoring
- Docker deployment
- Extensible processor architecture

**Use Cases**:
- Edge AI inference
- Anomaly detection services
- Custom ML model hosting

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/ai-service-template)

---

### Live Streaming Dashboard

Real-time telemetry visualization dashboard with MQTT WebSocket.

**Features**:
- Live chart visualization
- Multi-series support
- Raw data terminal view
- Dark theme UI

**Use Cases**:
- Real-time monitoring
- Telemetry debugging
- Demo dashboards

[View Example →](https://github.com/tesaiot/developer-hub/tree/main/examples/live-streaming-dashboard)

---

## Supported Platforms

### MCU & Edge Devices

| Platform | Status | Notes |
|----------|--------|-------|
| **Infineon PSoC 6** | ✅ Supported | Cortex-M4/M0+ with OPTIGA Trust M |
| **Infineon PSoC Edge (E84)** | ✅ Supported | Cortex-M55/M33 with Trust M |
| **Arduino** | ✅ Supported | ESP32, MKR WiFi 1010, Portenta |
| **STM32** | ✅ Supported | STM32F4, STM32L4, STM32H7 series |
| **NXP** | ✅ Supported | i.MX RT, LPC, Kinetis series |
| **Renesas** | ✅ Supported | RA, RX, Synergy series |
| **ESP32** | ✅ Supported | ESP32, ESP32-S2, ESP32-C3 |
| **Raspberry Pi** | ✅ Supported | Pi 3/4/5, Pi Pico W |
| **FPGA** | 🔶 Planned | Xilinx, Intel/Altera |

### Security Hardware

| Module | Integration |
|--------|-------------|
| **Infineon OPTIGA Trust M** | Certificate storage, crypto acceleration |
| **Microchip ATECC608** | Secure key storage |
| **NXP EdgeLock SE050** | Secure element integration |
| **TPM 2.0** | Platform attestation |

---

## Example Matrix

| Example | Language | Auth Method | Docker | Complexity |
|---------|----------|-------------|--------|------------|
| python-cli | Python | mTLS/Token | ✅ | ⭐ Easy |
| device-mtls | C/Python | mTLS | ✅ | ⭐⭐ Medium |
| device-servertls | C/Python | Token | ✅ | ⭐ Easy |
| react-dashboard | TypeScript | Token | ✅ | ⭐⭐ Medium |
| mqtt-quic (entry) | C/Python | Server-TLS | ✅ | ⭐⭐ Medium |
| **mqtt-quic (advanced)** | C/Python | Server-TLS | ✅ | ⭐⭐⭐ Advanced |
| nodered-integration | Node-RED | Token | ✅ | ⭐ Easy |
| wss-mqtt-streaming | JavaScript | Token | ✅ | ⭐ Easy |
| ai-service-template | Python | - | ✅ | ⭐⭐ Medium |
| live-streaming-dashboard | TypeScript | Token | ✅ | ⭐⭐ Medium |

### Advanced Examples (New)

| Example | Features | Platforms |
|---------|----------|-----------|
| mqtt-quic-advanced (C/C++) | QUIC/TCP fallback, 0-RTT, multi-stream, health monitoring | Linux, Raspberry Pi |
| mqtt-quic-advanced (Python) | QUIC/TCP fallback, 0-RTT, asyncio multi-stream | Any (cross-platform) |

---

## Contributing New Examples

Want to add your own example? We welcome contributions!

1. Fork the repository
2. Create a new directory under `examples/`
3. Include README.md, Dockerfile, and example code
4. Submit a pull request

See our [Contributing Guide](https://github.com/tesaiot/developer-hub/blob/main/CONTRIBUTING.md) for details.
