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

<style>
/* Wide Layout Override - Match Landing Page */
.page{max-width:100%!important;padding:0!important}
.page__content{max-width:80rem;margin:0 auto;padding:0 1.5rem}
.page__hero--overlay{padding:3rem 1.5rem}
.page__hero--overlay .page__title,.page__hero--overlay .page__lead,.page__hero--overlay .page__hero-caption{max-width:80rem;margin-left:auto;margin-right:auto}
#main{max-width:100%!important;padding:0!important}
.initial-content,.inner-wrap{max-width:100%!important}

/* Hide default masthead */
.masthead{display:none!important}

/* Feature Cards - Landing Page Use Cases Style */
.feature-cards{display:grid;grid-template-columns:1fr;gap:1rem;max-width:80rem;margin:2rem auto;padding:0 1rem}
@media(min-width:768px){.feature-cards{grid-template-columns:repeat(3,1fr);gap:1.5rem}}
.feature-card{display:flex;flex-direction:column;background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.1);border-radius:0.75rem;padding:1.25rem;text-decoration:none!important;transition:all 0.2s ease}
.feature-card:hover{background:rgba(255,255,255,0.06);border-color:rgba(255,255,255,0.2);transform:translateY(-2px)}
.feature-card--red{border-color:rgba(239,68,68,0.3)}
.feature-card--red .feature-card__icon{background:rgba(239,68,68,0.1);color:#ef4444}
.feature-card--red .feature-card__list li::before{color:#ef4444}
.feature-card--blue{border-color:rgba(59,130,246,0.3)}
.feature-card--blue .feature-card__icon{background:rgba(59,130,246,0.1);color:#3b82f6}
.feature-card--blue .feature-card__list li::before{color:#3b82f6}
.feature-card--green{border-color:rgba(16,185,129,0.3)}
.feature-card--green .feature-card__icon{background:rgba(16,185,129,0.1);color:#10b981}
.feature-card--green .feature-card__list li::before{color:#10b981}
.feature-card__icon{display:flex;align-items:center;justify-content:center;width:2.5rem;height:2.5rem;border-radius:0.75rem;margin-bottom:0.75rem}
.feature-card__icon svg{width:1.5rem;height:1.5rem}
.feature-card__content{margin-bottom:0.75rem}
.feature-card__title{font-size:1.125rem;font-weight:600;color:white!important;margin:0 0 0.25rem 0}
.feature-card__subtitle{font-size:0.75rem;color:rgba(255,255,255,0.5);margin:0}
.feature-card__list{list-style:none;padding:0;margin:0 0 1rem 0;flex-grow:1}
.feature-card__list li{display:flex;align-items:flex-start;gap:0.5rem;font-size:0.875rem;color:rgba(255,255,255,0.7);margin-bottom:0.5rem}
.feature-card__list li::before{content:"✓";flex-shrink:0;font-weight:bold}
.feature-card__btn{display:inline-flex;align-items:center;justify-content:center;padding:0.5rem 1rem;background:#14b8a6;color:#081225!important;font-size:0.875rem;font-weight:500;border-radius:0.5rem;transition:background 0.2s;margin-top:auto}
.feature-card__btn:hover{background:#0d9488}

/* Layered Architecture */
.layered-architecture{background:rgba(255,255,255,0.02);border:1px solid rgba(255,255,255,0.1);border-radius:1rem;padding:1.5rem;margin:2rem 0}
.arch-header{display:flex;align-items:center;gap:0.75rem;margin-bottom:1.5rem;padding-bottom:1rem;border-bottom:1px solid rgba(255,255,255,0.1)}
.arch-header__icon{display:flex;align-items:center;justify-content:center;width:3rem;height:3rem;background:linear-gradient(135deg,#14b8a6 0%,#0d9488 100%);border-radius:0.75rem;color:white}
.arch-header__title{font-size:1.25rem;font-weight:600;color:white!important;margin:0}
.arch-services{display:grid;grid-template-columns:repeat(2,1fr);gap:0.75rem;margin-bottom:0.75rem}
@media(min-width:768px){.arch-services{grid-template-columns:repeat(4,1fr)}}
.arch-service{display:flex;align-items:flex-start;gap:0.75rem;background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.1);border-radius:0.5rem;padding:0.875rem}
.arch-service svg{flex-shrink:0;margin-top:0.125rem}
.arch-service--teal svg{color:#14b8a6}
.arch-service--yellow svg{color:#f59e0b}
.arch-service--blue svg{color:#3b82f6}
.arch-service--purple svg{color:#8b5cf6}
.arch-service--red svg{color:#ef4444}
.arch-service--orange svg{color:#f97316}
.arch-service__title{display:block;font-size:0.875rem;font-weight:500;color:white}
.arch-service__desc{display:block;font-size:0.75rem;color:rgba(255,255,255,0.5)}
.arch-device-mgmt{display:flex;align-items:center;gap:0.75rem;background:linear-gradient(135deg,rgba(139,92,246,0.1) 0%,rgba(59,130,246,0.1) 100%);border:1px solid rgba(139,92,246,0.3);border-radius:0.5rem;padding:1rem;margin:0.75rem 0}
.arch-device-mgmt svg{flex-shrink:0;color:#a78bfa}
.arch-device-mgmt__title{display:block;font-size:0.875rem;font-weight:500;color:white}
.arch-device-mgmt__desc{display:block;font-size:0.75rem;color:rgba(255,255,255,0.5)}
.arch-bottom{display:grid;grid-template-columns:1fr;gap:1rem;margin-top:1rem}
@media(min-width:768px){.arch-bottom{grid-template-columns:repeat(3,1fr)}}
.arch-stack{background:rgba(255,255,255,0.02);border:1px solid rgba(255,255,255,0.1);border-radius:0.5rem;overflow:hidden}
.arch-stack__header{display:flex;align-items:center;gap:0.5rem;padding:0.625rem 0.875rem;font-size:0.8rem;font-weight:500;color:white;border-bottom:1px solid rgba(255,255,255,0.1)}
.arch-stack__header svg{flex-shrink:0}
.arch-stack__header--purple{background:rgba(139,92,246,0.15)}
.arch-stack__header--purple svg{color:#a78bfa}
.arch-stack__header--gray{background:rgba(255,255,255,0.05)}
.arch-stack__header--teal{background:rgba(20,184,166,0.15)}
.arch-stack__header--teal svg{color:#14b8a6}
.arch-stack__items{padding:0.5rem}
.arch-stack__item{padding:0.5rem 0.75rem;font-size:0.8rem;color:rgba(255,255,255,0.7);background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.08);border-radius:0.375rem;margin-bottom:0.375rem;text-align:center}
.arch-stack__item:last-child{margin-bottom:0}
.arch-stack__item--arrow{background:transparent;border:none;color:rgba(255,255,255,0.3);padding:0.125rem}
.arch-stack__item--highlight{background:rgba(20,184,166,0.15);border-color:rgba(20,184,166,0.3);color:#14b8a6}
.arch-stack__icons{display:flex;justify-content:center;gap:0.5rem;padding:0.5rem;font-size:1.25rem;margin-bottom:0.375rem}

/* Table - Fix overflow and symmetry */
.page__content{max-width:76rem!important;margin:0 auto!important;padding:0 2rem!important;box-sizing:border-box}
table{width:calc(50% - 2rem)!important;max-width:calc(50% - 2rem)!important;margin-left:auto!important;margin-right:auto!important;table-layout:auto!important;box-sizing:border-box}
table th,table td{padding:0.5rem 0.75rem!important;font-size:0.875rem;white-space:normal}
pre,div.highlighter-rouge{max-width:100%!important;margin-left:auto!important;margin-right:auto!important;box-sizing:border-box}
</style>


<!-- Custom Feature Cards - Matching Landing Page Use Cases Style -->
<div class="feature-cards">
  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices" class="feature-card feature-card--red">
    <div class="feature-card__icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect><line x1="9" y1="1" x2="9" y2="4"></line><line x1="15" y1="1" x2="15" y2="4"></line><line x1="9" y1="20" x2="9" y2="23"></line><line x1="15" y1="20" x2="15" y2="23"></line><line x1="20" y1="9" x2="23" y2="9"></line><line x1="20" y1="14" x2="23" y2="14"></line><line x1="1" y1="9" x2="4" y2="9"></line><line x1="1" y1="14" x2="4" y2="14"></line></svg>
    </div>
    <div class="feature-card__content">
      <h3 class="feature-card__title">Device Examples</h3>
      <p class="feature-card__subtitle">mTLS & Server-TLS Security</p>
    </div>
    <ul class="feature-card__list">
      <li>Infineon PSoC 6/PSoC Edge</li>
      <li>ESP32, Raspberry Pi</li>
      <li>MQTT over QUIC</li>
      <li>Hardware security modules</li>
    </ul>
    <span class="feature-card__btn">Learn More</span>
  </a>

  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/applications/visual-automation/nodered-integration" class="feature-card feature-card--blue">
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
      <li>WebSocket streaming</li>
    </ul>
    <span class="feature-card__btn">Learn More</span>
  </a>

  <a href="https://github.com/tesaiot/developer-hub/tree/main/examples/applications/real-time/live-streaming-dashboard" class="feature-card feature-card--green">
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

**22 Examples, 100% Tested and Working!**

#### Entry Level - Device Examples

| # | Example | Description | Language |
|---|---------|-------------|----------|
| 1 | [python-cli](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/entry/python-cli) | Command-line MQTT client | Python |
| 2 | [device-servertls](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/entry/device-servertls) | Server TLS device connection | C/Python |
| 3 | [mqtt-quic-python](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/entry/mqtt-quic-python) | MQTT over QUIC (Python) | Python |
| 4 | [rpi-servertls](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/entry/rpi-servertls) | Raspberry Pi with Server-TLS | Python |

#### Intermediate Level - Secure Devices

| # | Example | Description | Language |
|---|---------|-------------|----------|
| 5 | [device-mtls](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/intermediate/device-mtls) | Mutual TLS device connection | C/Python |
| 6 | [esp32-servertls](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/intermediate/esp32-servertls) | ESP32 with Server-TLS | C/Arduino |

#### Advanced Level - Production Ready

| # | Example | Features | Language |
|---|---------|----------|----------|
| 7 | [mqtt-quic-advanced (C/C++)](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/advanced/mqtt-quic/c_cpp) | QUIC/TCP fallback, 0-RTT, multi-stream | C/C++ |
| 8 | [mqtt-quic-advanced (Python)](https://github.com/tesaiot/developer-hub/tree/main/examples/embedded-devices/advanced/mqtt-quic/python) | QUIC/TCP fallback, 0-RTT, asyncio | Python |

#### Integration Templates

| # | Example | Description | Language |
|---|---------|-------------|----------|
| 9 | [nodered-integration](https://github.com/tesaiot/developer-hub/tree/main/examples/applications/visual-automation/nodered-integration) | Node-RED integration flows | Node-RED |
| 10 | [wss-mqtt-streaming](https://github.com/tesaiot/developer-hub/tree/main/examples/applications/real-time/wss-mqtt-streaming) | WebSocket MQTT streaming | JS/Python |
| 11 | [ai-service-template](https://github.com/tesaiot/developer-hub/tree/main/examples/integrations/ai-services/ai-service-template) | AI inference service | Python/FastAPI |
| 12 | [mqtt-quic-c](https://github.com/tesaiot/developer-hub/tree/main/examples/integrations/mqtt-integration/mqtt-quic-c) | MQTT over QUIC (C) | C |

#### Applications - Dashboards

| # | Example | Description | Language |
|---|---------|-------------|----------|
| 13 | [react-dashboard](https://github.com/tesaiot/developer-hub/tree/main/examples/applications/visualization/react-dashboard) | IoT monitoring dashboard | React/TS |
| 14 | [live-streaming-dashboard](https://github.com/tesaiot/developer-hub/tree/main/examples/applications/real-time/live-streaming-dashboard) | Real-time telemetry dashboard | React/TS |
| 15 | [grafana-dashboard](https://github.com/tesaiot/developer-hub/tree/main/examples/applications/visualization/grafana-dashboard) | Grafana dashboards | Grafana |

#### Analytics API Examples

| # | Example | Description | Language |
|---|---------|-------------|----------|
| 16 | [analytics-api-python](https://github.com/tesaiot/developer-hub/tree/main/examples/analytics-api/python) | Analytics API client | Python |
| 17 | [analytics-api-javascript](https://github.com/tesaiot/developer-hub/tree/main/examples/analytics-api/javascript) | Analytics API client | JavaScript |
| 18 | [analytics-api-rust](https://github.com/tesaiot/developer-hub/tree/main/examples/analytics-api/rust) | Analytics API client | Rust |

### 3. Configure and Run

Each example includes:
- `README.md` - Setup instructions
- `.env.example` - Configuration template
- `Dockerfile` - Container deployment

## Supported Platforms

### MCU & Edge Devices

| Platform | Status | Notes |
|----------|--------|-------|
| **Infineon PSoC 6** | Supported | Cortex-M4/M0+ with OPTIGA Trust M |
| **Infineon PSoC Edge (E84)** | Supported | Cortex-M55/M33 with Trust M |
| **ESP32** | Supported | ESP32, ESP32-S2, ESP32-C3 |
| **Raspberry Pi** | Supported | Pi 3/4/5, Pi Pico W |
| **Arduino** | Supported | ESP32, MKR WiFi 1010, Portenta |
| **STM32** | Supported | STM32F4, STM32L4, STM32H7 series |

### Security Hardware

| Module | Integration |
|--------|-------------|
| **Infineon OPTIGA Trust M** | Certificate storage, crypto acceleration |

## Platform Features

### Security First

- **mTLS Authentication**: Certificate-based device authentication
- **Vault PKI Integration**: Automatic certificate management
- **RBAC**: Role-based access control for users and devices
- **Hardware Security**: OPTIGA Trust M support

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

