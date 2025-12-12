---
layout: home
title: "TESAIoT Developer Hub"
classes: wide
header:
  overlay_color: "#0b1220"
  overlay_filter: "0.3"
  actions:
    - label: "Get Started"
      url: "https://github.com/tesaiot/developer-hub/blob/main/docs/getting-started.md"
    - label: "View on GitHub"
      url: "https://github.com/tesaiot/developer-hub"
excerpt: "Open source examples and templates for building secure IoT applications on TESAIoT Platform"
---

<!-- Custom Feature Cards - Matching Landing Page Use Cases Style -->
<div class="feature-cards">
  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/device-mtls" class="feature-card feature-card--red">
    <div class="feature-card__icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect><line x1="9" y1="1" x2="9" y2="4"></line><line x1="15" y1="1" x2="15" y2="4"></line><line x1="9" y1="20" x2="9" y2="23"></line><line x1="15" y1="20" x2="15" y2="23"></line><line x1="20" y1="9" x2="23" y2="9"></line><line x1="20" y1="14" x2="23" y2="14"></line><line x1="1" y1="9" x2="4" y2="9"></line><line x1="1" y1="14" x2="4" y2="14"></line></svg>
    </div>
    <div class="feature-card__content">
      <h3 class="feature-card__title">Device Examples</h3>
      <p class="feature-card__subtitle">mTLS Security</p>
    </div>
    <ul class="feature-card__list">
      <li>Infineon PSoC 6/PSoC Edge</li>
      <li>Arduino, STM32, NXP, Renesas</li>
      <li>ESP32, Raspberry Pi</li>
      <li>Hardware security modules</li>
    </ul>
    <span class="feature-card__btn">Learn More</span>
  </a>

  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/nodered-integration" class="feature-card feature-card--blue">
    <div class="feature-card__icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="16 18 22 12 16 6"></polyline><polyline points="8 6 2 12 8 18"></polyline></svg>
    </div>
    <div class="feature-card__content">
      <h3 class="feature-card__title">Integration Templates</h3>
      <p class="feature-card__subtitle">Ready to Deploy</p>
    </div>
    <ul class="feature-card__list">
      <li>Node-RED flows</li>
      <li>React dashboards</li>
      <li>AI service templates</li>
      <li>FastAPI backends</li>
    </ul>
    <span class="feature-card__btn">Learn More</span>
  </a>

  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/live-streaming-dashboard" class="feature-card feature-card--green">
    <div class="feature-card__icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline></svg>
    </div>
    <div class="feature-card__content">
      <h3 class="feature-card__title">Live Streaming</h3>
      <p class="feature-card__subtitle">Real-time Data</p>
    </div>
    <ul class="feature-card__list">
      <li>MQTT WebSocket streaming</li>
      <li>React visualization</li>
      <li>Time-series charts</li>
      <li>Alert notifications</li>
    </ul>
    <span class="feature-card__btn">Learn More</span>
  </a>
</div>

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

<!-- Layered Platform Architecture - Matching Landing Page Style -->
<div class="layered-architecture">
  <div class="arch-header">
    <div class="arch-header__icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><ellipse cx="12" cy="5" rx="9" ry="3"></ellipse><path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3"></path><path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5"></path></svg>
    </div>
    <h3 class="arch-header__title">TESAIoT Foundation Platform</h3>
  </div>

  <div class="arch-services">
    <div class="arch-service arch-service--teal">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline></svg>
      <div>
        <span class="arch-service__title">Data Analytics</span>
        <span class="arch-service__desc">Real-time streaming & batch</span>
      </div>
    </div>
    <div class="arch-service arch-service--yellow">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect></svg>
      <div>
        <span class="arch-service__title">Digital Twin</span>
        <span class="arch-service__desc">AI Agents & simulation</span>
      </div>
    </div>
    <div class="arch-service arch-service--blue">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>
      <div>
        <span class="arch-service__title">API Gateway</span>
        <span class="arch-service__desc">Multi-protocol unified access</span>
      </div>
    </div>
    <div class="arch-service arch-service--purple">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"></path></svg>
      <div>
        <span class="arch-service__title">Edge AI Model</span>
        <span class="arch-service__desc">ETL & inference pipeline</span>
      </div>
    </div>
  </div>

  <div class="arch-services">
    <div class="arch-service arch-service--red">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"></path></svg>
      <div>
        <span class="arch-service__title">National AIoT Root CA & PKI</span>
        <span class="arch-service__desc">4-Tier certificate hierarchy</span>
      </div>
    </div>
    <div class="arch-service arch-service--orange">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4"></path></svg>
      <div>
        <span class="arch-service__title">Key Provisioning Service</span>
        <span class="arch-service__desc">Zero-touch enrollment</span>
      </div>
    </div>
  </div>

  <div class="arch-device-mgmt">
    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"></path><circle cx="9" cy="7" r="4"></circle><path d="M23 21v-2a4 4 0 0 0-3-3.87"></path><path d="M16 3.13a4 4 0 0 1 0 7.75"></path></svg>
    <div>
      <span class="arch-device-mgmt__title">AIoT Device Provisioning & Management</span>
      <span class="arch-device-mgmt__desc">Lifecycle management, OTA updates, and fleet operations</span>
    </div>
  </div>

  <div class="arch-bottom">
    <div class="arch-stack">
      <div class="arch-stack__header arch-stack__header--purple">
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect></svg>
        IoT Device Stack
      </div>
      <div class="arch-stack__items">
        <div class="arch-stack__item">IoT Application</div>
        <div class="arch-stack__item">Embedded OS</div>
        <div class="arch-stack__item">Crypto Library</div>
        <div class="arch-stack__item">Hardware (HSM)</div>
      </div>
    </div>
    <div class="arch-stack">
      <div class="arch-stack__header arch-stack__header--gray">
        <span>Connection Flow</span>
      </div>
      <div class="arch-stack__items">
        <div class="arch-stack__item">IoT Application</div>
        <div class="arch-stack__item arch-stack__item--arrow">↓</div>
        <div class="arch-stack__item">IoT Product</div>
        <div class="arch-stack__item arch-stack__item--arrow">↓</div>
        <div class="arch-stack__item arch-stack__item--highlight">IoT Platform</div>
      </div>
    </div>
    <div class="arch-stack">
      <div class="arch-stack__header arch-stack__header--teal">
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"></path></svg>
        Cloud Infrastructure
      </div>
      <div class="arch-stack__items">
        <div class="arch-stack__icons">
          <span title="DigitalOcean">🌊</span>
          <span title="AWS">☁️</span>
          <span title="GCP">🔷</span>
        </div>
        <div class="arch-stack__item">Secure Gateway</div>
        <div class="arch-stack__item">Analytics Engine</div>
        <div class="arch-stack__item">Dashboard</div>
      </div>
    </div>
  </div>
</div>

## Community

- **Issues**: [Report bugs or request features](https://github.com/tesaiot/developer-hub/issues)
- **Discussions**: [Ask questions and share ideas](https://github.com/tesaiot/developer-hub/discussions)
- **Contributing**: [Contribution guidelines](https://github.com/tesaiot/developer-hub/blob/main/CONTRIBUTING.md)

## License

Apache 2.0 - See [LICENSE](https://github.com/tesaiot/developer-hub/blob/main/LICENSE) for details.

---

Built with TESAIoT Platform Examples | Copyright 2025 TESAIoT Platform by TESA
