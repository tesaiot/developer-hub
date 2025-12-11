---
layout: home
title: "TESAIoT Developer Hub"
header:
  overlay_color: "#1a1a2e"
  overlay_filter: "0.5"
  actions:
    - label: "Get Started"
      url: "/developer-hub/getting-started"
    - label: "View on GitHub"
      url: "https://github.com/tesaiot/developer-hub"
excerpt: "Open source examples and templates for building secure IoT applications on TESAIoT Platform"
feature_row:
  - title: "Device Examples"
    excerpt: "MQTT clients for Infineon PSoC, Arduino, STM32, NXP, Renesas, ESP32, Raspberry Pi, and more with mTLS security"
    url: "/developer-hub/examples#device-examples"
    btn_label: "Learn More"
    btn_class: "btn--primary"
  - title: "Integration Templates"
    excerpt: "Node-RED flows, React dashboards, and AI service templates ready to deploy"
    url: "/developer-hub/examples#integration-templates"
    btn_label: "Learn More"
    btn_class: "btn--primary"
  - title: "Live Streaming"
    excerpt: "Real-time MQTT WebSocket streaming with React visualization dashboard"
    url: "https://github.com/tesaiot/developer-hub/tree/main/examples/live-streaming-dashboard"
    btn_label: "Learn More"
    btn_class: "btn--primary"
---

{% include feature_row %}

## Quick Start

Get up and running with TESAIoT Platform in minutes.

### 1. Clone the Repository

```bash
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub/examples
```

### 2. Choose an Example

| Example | Description | Language |
|---------|-------------|----------|
| [python-cli](https://github.com/tesaiot/developer-hub/tree/main/examples/python-cli) | Command-line MQTT client | Python |
| [device-mtls](https://github.com/tesaiot/developer-hub/tree/main/examples/device-mtls) | Mutual TLS device connection | C/Python |
| [device-servertls](https://github.com/tesaiot/developer-hub/tree/main/examples/device-servertls) | Server TLS device connection | C/Python |
| [react-dashboard](https://github.com/tesaiot/developer-hub/tree/main/examples/react-dashboard) | IoT monitoring dashboard | React/TS |
| [mqtt-quic](https://github.com/tesaiot/developer-hub/tree/main/examples/mqtt-quic) | MQTT over QUIC transport | C/Python |
| [nodered-integration](https://github.com/tesaiot/developer-hub/tree/main/examples/nodered-integration) | Node-RED integration flows | Node-RED |
| [wss-mqtt-streaming](https://github.com/tesaiot/developer-hub/tree/main/examples/wss-mqtt-streaming) | WebSocket MQTT streaming | JavaScript |
| [ai-service-template](https://github.com/tesaiot/developer-hub/tree/main/examples/ai-service-template) | AI inference service | Python/FastAPI |
| [live-streaming-dashboard](https://github.com/tesaiot/developer-hub/tree/main/examples/live-streaming-dashboard) | Real-time telemetry dashboard | React/TS |

### 3. Configure and Run

Each example includes:
- `README.md` - Setup instructions
- `.env.example` - Configuration template
- `Dockerfile` - Container deployment

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

## Platform Features

### Security First

- **mTLS Authentication**: Certificate-based device authentication
- **Vault PKI Integration**: Automatic certificate management
- **RBAC**: Role-based access control for users and devices
- **Hardware Security**: OPTIGA Trust M, TPM 2.0 support

### Edge AI Ready

- **Local Inference**: Run AI models at the edge
- **Anomaly Detection**: Real-time sensor anomaly detection
- **Model Management**: Deploy and update models remotely

### Enterprise Scale

- **Multi-tenant**: Organization isolation
- **Time-Series Storage**: TimescaleDB for sensor data
- **Real-time Streaming**: MQTT with WebSocket support

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TESAIoT Platform                              │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────┐ │
│  │   EMQX      │  │  REST API   │  │   Vault     │  │ TimescaleDB│ │
│  │   Broker    │  │   (FastAPI) │  │   PKI       │  │            │ │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬─────┘ │
│         │                │                │                │        │
│  ┌──────┴────────────────┴────────────────┴────────────────┴──────┐ │
│  │                     Internal Network                           │ │
│  └────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
           ▲                ▲                ▲
           │ MQTT/TLS       │ HTTPS          │ WSS
           │                │                │
    ┌──────┴──────┐  ┌──────┴──────┐  ┌──────┴──────┐
    │   Devices   │  │   Admin     │  │  Dashboard  │
    │  PSoC/STM32 │  │   Portal    │  │             │
    │  Arduino/Pi │  └─────────────┘  └─────────────┘
    └─────────────┘
```

## Community

- **Issues**: [Report bugs or request features](https://github.com/tesaiot/developer-hub/issues)
- **Discussions**: [Ask questions and share ideas](https://github.com/tesaiot/developer-hub/discussions)
- **Contributing**: [Contribution guidelines](https://github.com/tesaiot/developer-hub/blob/main/CONTRIBUTING.md)

## License

Apache 2.0 - See [LICENSE](https://github.com/tesaiot/developer-hub/blob/main/LICENSE) for details.

---

Built with TESAIoT Platform Examples | Copyright 2025 TESAIoT Platform by TESA
