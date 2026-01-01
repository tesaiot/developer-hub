---
title: "Getting Started"
permalink: /getting-started/
toc: true
toc_sticky: true
---

# Getting Started with TESAIoT Developer Hub

This guide will help you get started with TESAIoT Platform examples.

## Prerequisites

- **TESAIoT Platform Account**: Sign up at [admin.tesaiot.com](https://admin.tesaiot.com)
- **Docker**: For containerized deployment (recommended)
- **Git**: For cloning the repository

### Language-specific Requirements

| Example Type | Requirements |
|--------------|-------------|
| Python examples | Python 3.10+ |
| React dashboards | Node.js 18+ |
| Node-RED | Node.js 16+ |

## Installation

### Clone the Repository

```bash
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub
```

### Project Structure

```
developer-hub/
├── examples/
│   ├── python-cli/           # Python MQTT client
│   ├── device-mtls/          # mTLS device example
│   ├── device-servertls/     # Server TLS device
│   ├── react-dashboard/      # React monitoring UI
│   ├── mqtt-quic/            # MQTT over QUIC
│   ├── nodered-integration/  # Node-RED flows
│   ├── wss-mqtt-streaming/   # WebSocket MQTT
│   ├── ai-service-template/  # AI service template
│   └── live-streaming-dashboard/  # Real-time dashboard
├── docs/                     # This documentation
├── LICENSE                   # Apache 2.0
├── NOTICE                    # Attribution requirements
└── CONTRIBUTING.md           # Contribution guide
```

## Quick Start Examples

### 1. Python CLI Client

The simplest way to connect to TESAIoT Platform:

```bash
cd examples/python-cli

# Install dependencies
pip install -r requirements.txt

# Configure
cp .env.example .env
# Edit .env with your credentials

# Run
python main.py
```

### 2. Live Streaming Dashboard

Real-time telemetry visualization:

```bash
cd examples/live-streaming-dashboard

# Install dependencies
npm install

# Start development server
npm run dev

# Open http://localhost:3000
```

### 3. AI Service Template

Deploy AI inference as a service:

```bash
cd examples/ai-service-template

# Build and run with Docker
docker-compose up -d

# Test health endpoint
curl http://localhost:8000/health
```

## Authentication Methods

TESAIoT Platform supports multiple authentication methods:

### 1. mTLS (Mutual TLS)

Best for: Production devices, high-security environments

```python
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.tls_set(
    ca_certs="ca.crt",
    certfile="device.crt",
    keyfile="device.key"
)
client.connect("mqtt.tesaiot.com", 8883)
```

### 2. Server TLS with Token

Best for: Development, testing, web applications

```python
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.tls_set(ca_certs="ca.crt")
client.username_pw_set(
    username="device_id",
    password="api_token"
)
client.connect("mqtt.tesaiot.com", 8883)
```

### 3. WebSocket Secure (WSS)

Best for: Browser applications, real-time dashboards

```javascript
import mqtt from 'mqtt';

const client = mqtt.connect('wss://mqtt.tesaiot.com:8085/mqtt', {
    username: 'tesa_mqtt_org_token123...',
    password: '',
});
```

## MQTT Topics

TESAIoT Platform uses a structured topic hierarchy:

```
device/{device_id}/telemetry/{type}   # Publish sensor data
device/{device_id}/commands/{cmd}     # Receive commands
device/{device_id}/status             # Device status updates
```

### Topic Wildcards

- `+` = Single level wildcard
- `#` = Multi-level wildcard

Examples:
```
device/+/telemetry/#        # All telemetry from all devices
device/sensor-001/telemetry/+  # All telemetry types from sensor-001
```

## Next Steps

1. **Explore Examples**: Browse the [examples directory](../examples/)
2. **Read the API Docs**: Check the [API Reference](../api/)
3. **Join the Community**: [GitHub Discussions](https://github.com/tesaiot/developer-hub/discussions)

## Getting Help

- **Documentation**: You're reading it!
- **Issues**: [GitHub Issues](https://github.com/tesaiot/developer-hub/issues)
- **Support**: [admin.tesaiot.com](https://admin.tesaiot.com) → Help

## Common Issues

### Connection Refused

**Symptom**: Cannot connect to MQTT broker

**Solutions**:
1. Check network connectivity to mqtt.tesaiot.com
2. Verify certificates are valid and not expired
3. Ensure firewall allows outbound port 8883/8085

### Certificate Errors

**Symptom**: SSL/TLS handshake fails

**Solutions**:
1. Download fresh certificates from Admin Portal
2. Check certificate chain includes CA certificate
3. Verify certificate is for correct device ID

### Token Expired

**Symptom**: Authentication fails after some time

**Solutions**:
1. Tokens expire after configured period
2. Generate new token from Admin Portal
3. Implement token refresh in your application
