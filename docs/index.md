---
layout: splash
title: "TESAIoT Developer Hub"
header:
  overlay_color: "#1a1a2e"
  overlay_filter: "0.5"
  actions:
    - label: "Get Started"
      url: "/developer-hub/getting-started/"
    - label: "View on GitHub"
      url: "https://github.com/tesaiot/developer-hub"
excerpt: "Open source examples and templates for building secure IoT applications on TESAIoT Platform"
feature_row:
  - title: "Device Examples"
    excerpt: "MQTT clients for Raspberry Pi, ESP32, and Python applications with mTLS security"
    url: "/developer-hub/examples/#device-examples"
    btn_label: "Learn More"
    btn_class: "btn--primary"
  - title: "Integration Templates"
    excerpt: "Node-RED flows, React dashboards, and AI service templates ready to deploy"
    url: "/developer-hub/examples/#integration-templates"
    btn_label: "Learn More"
    btn_class: "btn--primary"
  - title: "Live Streaming"
    excerpt: "Real-time MQTT WebSocket streaming with React visualization dashboard"
    url: "/developer-hub/examples/live-streaming-dashboard/"
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
| [python-cli](examples/python-cli/) | Command-line MQTT client | Python |
| [device-mtls](examples/device-mtls/) | Mutual TLS device connection | Python |
| [device-servertls](examples/device-servertls/) | Server TLS device connection | Python |
| [react-dashboard](examples/react-dashboard/) | IoT monitoring dashboard | React/TS |
| [mqtt-quic](examples/mqtt-quic/) | MQTT over QUIC transport | Python |
| [nodered-integration](examples/nodered-integration/) | Node-RED integration flows | Node-RED |
| [wss-mqtt-streaming](examples/wss-mqtt-streaming/) | WebSocket MQTT streaming | JavaScript |
| [ai-service-template](examples/ai-service-template/) | AI inference service | Python/FastAPI |
| [live-streaming-dashboard](examples/live-streaming-dashboard/) | Real-time telemetry dashboard | React/TS |

### 3. Configure and Run

Each example includes:
- `README.md` - Setup instructions
- `.env.example` - Configuration template
- `Dockerfile` - Container deployment

## Platform Features

### Security First

- **mTLS Authentication**: Certificate-based device authentication
- **Vault PKI Integration**: Automatic certificate management
- **RBAC**: Role-based access control for users and devices

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
    │             │  │   Portal    │  │             │
    └─────────────┘  └─────────────┘  └─────────────┘
```

## Community

- **Issues**: [Report bugs or request features](https://github.com/tesaiot/developer-hub/issues)
- **Discussions**: [Ask questions and share ideas](https://github.com/tesaiot/developer-hub/discussions)
- **Contributing**: [Contribution guidelines](CONTRIBUTING.md)

## License

Apache 2.0 - See [LICENSE](LICENSE) for details.

Built with TESAIoT Platform Examples
