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

[View Example →](python-cli/)

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
- Raspberry Pi deployments

[View Example →](device-mtls/)

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

[View Example →](device-servertls/)

---

### MQTT over QUIC

Experimental MQTT client using QUIC transport protocol.

**Features**:
- QUIC transport for low latency
- Better performance on lossy networks
- Connection migration support

**Use Cases**:
- Mobile devices
- Unreliable networks
- Edge computing scenarios

[View Example →](mqtt-quic/)

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

[View Example →](nodered-integration/)

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

[View Example →](react-dashboard/)

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

[View Example →](wss-mqtt-streaming/)

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

[View Example →](ai-service-template/)

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

[View Example →](live-streaming-dashboard/)

---

## Example Matrix

| Example | Language | Auth Method | Docker | Complexity |
|---------|----------|-------------|--------|------------|
| python-cli | Python | mTLS/Token | ✅ | ⭐ Easy |
| device-mtls | Python | mTLS | ✅ | ⭐⭐ Medium |
| device-servertls | Python | Token | ✅ | ⭐ Easy |
| react-dashboard | TypeScript | Token | ✅ | ⭐⭐ Medium |
| mqtt-quic | Python | mTLS | ✅ | ⭐⭐⭐ Advanced |
| nodered-integration | Node-RED | Token | ✅ | ⭐ Easy |
| wss-mqtt-streaming | JavaScript | Token | ✅ | ⭐ Easy |
| ai-service-template | Python | - | ✅ | ⭐⭐ Medium |
| live-streaming-dashboard | TypeScript | Token | ✅ | ⭐⭐ Medium |

---

## Contributing New Examples

Want to add your own example? We welcome contributions!

1. Fork the repository
2. Create a new directory under `examples/`
3. Include README.md, Dockerfile, and example code
4. Submit a pull request

See our [Contributing Guide](../CONTRIBUTING.md) for details.
