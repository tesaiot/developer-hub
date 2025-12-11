# TESAIoT WSS MQTT API Token - Complete How-To Guide

This guide provides step-by-step instructions for integrating third-party applications with TESAIoT Platform using WebSocket Secure (WSS) MQTT connections.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Step 1: Generate MQTT API Token](#step-1-generate-mqtt-api-token)
4. [Step 2: Configure Your Application](#step-2-configure-your-application)
5. [Step 3: Connect via WSS](#step-3-connect-via-wss)
6. [Step 4: Subscribe to Telemetry](#step-4-subscribe-to-telemetry)
7. [API Reference](#api-reference)
8. [Code Examples](#code-examples)
9. [Troubleshooting](#troubleshooting)

---

## Overview

TESAIoT Platform provides a secure WebSocket (WSS) interface for third-party applications to subscribe to real-time device telemetry data. This enables:

- **Real-time Data Access**: Subscribe to device sensor data as it arrives
- **Secure Communication**: TLS-encrypted WebSocket connections
- **Organization Scoping**: Tokens are scoped to your organization's devices only
- **Read-Only Access**: Subscribe-only permissions for security

### Architecture

```
┌─────────────────┐     WSS:8085     ┌──────────────┐
│ Third-Party App │ ───────────────► │  TESAIoT     │
│ (Python/Node.js)│                  │  EMQX Broker │
└─────────────────┘                  └──────────────┘
        │                                   │
        │ Token Auth                        │ Telemetry
        ▼                                   ▼
┌─────────────────┐              ┌──────────────────┐
│  TESAIoT API    │              │   IoT Devices    │
│ Token Validation│              │ (PSoC/ESP32/etc) │
└─────────────────┘              └──────────────────┘
```

---

## Prerequisites

Before you begin, ensure you have:

1. **TESAIoT Account** with Organization Admin role
2. **Organization** created in TESAIoT Platform
3. **Devices** registered and sending telemetry
4. **Development Environment**:
   - Python 3.8+ or Node.js 16+
   - Network access to `mqtt.tesaiot.com:8085`

---

## Step 1: Generate MQTT API Token

### Using TESAIoT Admin UI (Recommended)

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Navigate to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter configuration:
   - **Label**: Descriptive name (e.g., "Production Dashboard")
   - **Expiration**: Token validity period (default: 90 days)
5. Click **Generate**
6. **Copy the token immediately** - it will only be shown once!

### Using REST API

```bash
# Login to get JWT token
JWT_TOKEN=$(curl -sk -X POST https://admin.tesaiot.com/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email": "your@email.com", "password": "your-password"}' \
  | jq -r '.token')

# Generate MQTT API Token
curl -sk -X POST "https://admin.tesaiot.com/api/v1/organizations/YOUR_ORG_ID/mqtt-tokens" \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "My Application Token",
    "description": "Token for data analytics dashboard",
    "expires_in_days": 90
  }'
```

**Response:**
```json
{
  "success": true,
  "token_id": "a2672ec3-291b-41ce-acce-00f26fe106d7",
  "token": "tesa_mqtt_yourorg_fPuLMEWonUFAfwctwteXoNlKcu84sqgt",
  "expires_at": "2026-03-09T16:42:25.327193",
  "permissions": {
    "subscribe": ["device/+/telemetry/#"],
    "publish": []
  },
  "message": "Token generated successfully. Save this token now - it will not be shown again."
}
```

### Token Format

```
tesa_mqtt_<org_prefix>_<random_32_chars>
         │            │
         │            └─ Secure random string
         └─ Organization identifier (6-10 chars)
```

---

## Step 2: Configure Your Application

### Environment Variables

Create a `.env` file in your project:

```env
# Required
MQTT_API_TOKEN=tesa_mqtt_yourorg_xxxxxxxxxxxxxxxxxxxxxxxxxxxx

# Optional (defaults shown)
MQTT_BROKER_URL=wss://mqtt.tesaiot.com:8085/mqtt
MQTT_SUBSCRIBE_TOPIC=device/+/telemetry/#
MQTT_CLIENT_ID=my-app-unique-id
```

### Connection Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Protocol | WSS | WebSocket Secure |
| Host | `mqtt.tesaiot.com` | MQTT broker hostname |
| Port | `8085` | WSS port |
| Path | `/mqtt` | WebSocket path |
| Username | `<token>` | Your MQTT API token |
| Password | `<token>` | Same as username |
| TLS | Required | Certificate verification enabled |

---

## Step 3: Connect via WSS

### Python Example

```python
import paho.mqtt.client as mqtt
from datetime import datetime

# Configuration
token = "tesa_mqtt_yourorg_xxxxxxxxxxxxxxxxxxxxxxxxxxxx"
broker_host = "mqtt.tesaiot.com"
broker_port = 8085

# Create client with WebSocket transport
client = mqtt.Client(
    client_id=f"my-app-{datetime.now().timestamp():.0f}",
    transport="websockets",
    protocol=mqtt.MQTTv311,
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
)

# Configure WebSocket path
client.ws_set_options(path="/mqtt")

# Set credentials (token for both username and password)
client.username_pw_set(username=token, password=token)

# Enable TLS for secure connection
client.tls_set()

# Connect
client.connect(broker_host, broker_port, keepalive=60)
```

### Node.js Example

```javascript
const mqtt = require('mqtt');

const token = 'tesa_mqtt_yourorg_xxxxxxxxxxxxxxxxxxxxxxxxxxxx';
const brokerUrl = 'wss://mqtt.tesaiot.com:8085/mqtt';

const client = mqtt.connect(brokerUrl, {
    username: token,
    password: token,
    clientId: `my-app-${Date.now()}`,
    clean: true,
    connectTimeout: 30000,
    reconnectPeriod: 5000,
    rejectUnauthorized: true,  // Verify TLS certificate
});

client.on('connect', () => {
    console.log('Connected to TESAIoT MQTT Broker!');
});
```

---

## Step 4: Subscribe to Telemetry

### Topic Structure

```
device/<device_id>/telemetry/<sensor_type>
       │                      │
       │                      └─ Sensor name (temperature, humidity, etc.)
       └─ Device UUID (e.g., 4797831a-e4cb-41f0-8dbc-e7de2dffe696)
```

### Available Topic Patterns

| Pattern | Description | Example |
|---------|-------------|---------|
| `device/+/telemetry/#` | All telemetry from all devices | All sensors, all devices |
| `device/DEVICE_ID/telemetry/#` | All telemetry from specific device | All sensors, one device |
| `device/+/telemetry/temperature` | Specific sensor from all devices | Temperature only |
| `device/DEVICE_ID/telemetry/temperature` | Specific sensor from specific device | One sensor, one device |

### Subscribe Example (Python)

```python
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected!")
        # Subscribe to all telemetry from organization's devices
        client.subscribe("device/+/telemetry/#", qos=1)
    else:
        print(f"Connection failed: {reason_code}")

def on_message(client, userdata, msg):
    topic_parts = msg.topic.split("/")
    device_id = topic_parts[1]
    sensor_type = topic_parts[3] if len(topic_parts) > 3 else "default"

    import json
    data = json.loads(msg.payload.decode())

    print(f"Device: {device_id}")
    print(f"Sensor: {sensor_type}")
    print(f"Data: {data}")

client.on_connect = on_connect
client.on_message = on_message
client.loop_forever()
```

---

## API Reference

### Token Management Endpoints

#### Generate Token

```http
POST /api/v1/organizations/{org_id}/mqtt-tokens
Authorization: Bearer <jwt_token>
Content-Type: application/json

{
  "name": "Token Label",
  "description": "Optional description",
  "expires_in_days": 90
}
```

#### List Tokens

```http
GET /api/v1/organizations/{org_id}/mqtt-tokens
Authorization: Bearer <jwt_token>
```

#### Get Token Details

```http
GET /api/v1/organizations/{org_id}/mqtt-tokens/{token_id}
Authorization: Bearer <jwt_token>
```

#### Revoke Token

```http
DELETE /api/v1/organizations/{org_id}/mqtt-tokens/{token_id}
Authorization: Bearer <jwt_token>
```

### Response Formats

**Success Response:**
```json
{
  "success": true,
  "token_id": "uuid",
  "token": "tesa_mqtt_...",
  "expires_at": "2026-01-08T16:42:25.327193",
  "permissions": {
    "subscribe": ["device/+/telemetry/#"],
    "publish": []
  }
}
```

**Error Response:**
```json
{
  "success": false,
  "error": "Error message",
  "code": "ERROR_CODE"
}
```

---

## Code Examples

### Complete Python Application

See [python/main.py](./python/main.py) for a full working example.

Key features:
- Automatic reconnection
- Graceful shutdown handling
- Message parsing and processing
- Environment variable configuration

### Complete Node.js Application

See [nodejs/index.js](./nodejs/index.js) for a full working example.

Key features:
- Event-driven architecture
- Promise-based connection
- Message buffering
- Error handling

---

## Troubleshooting

### Connection Issues

| Error | Cause | Solution |
|-------|-------|----------|
| `Connection refused` | Network/firewall issue | Check port 8085 is open |
| `Authentication failed (code 5)` | Invalid or expired token | Generate new token |
| `TLS handshake failed` | Certificate issue | Ensure TLS is enabled |
| `Timeout` | Network latency | Increase connect timeout |

### No Messages Received

1. **Verify devices are active**: Check device dashboard in Admin UI
2. **Check topic pattern**: Ensure wildcard matches device topics
3. **Confirm organization**: Token only receives data from your org's devices
4. **Check QoS level**: Use QoS 1 for guaranteed delivery

### Token Issues

| Issue | Solution |
|-------|----------|
| Token not working | Check expiration date in Admin UI |
| Token revoked | Generate new token |
| Permission denied | Token is subscribe-only; cannot publish |
| Wrong organization | Use token for correct organization |

### Debug Mode

Enable verbose logging:

**Python:**
```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

**Node.js:**
```javascript
const client = mqtt.connect(url, {
    ...options,
    debug: true
});
```

---

## Security Best Practices

1. **Never commit tokens** to version control
2. **Use environment variables** for token storage
3. **Rotate tokens** before expiration
4. **Use unique client IDs** per application instance
5. **Enable TLS verification** in production
6. **Monitor token usage** in Admin UI

---

## Support

- **Documentation**: [docs.tesaiot.com](https://docs.tesaiot.com)
- **Issues**: [GitHub Issues](https://github.com/tesaiot/platform/issues)
- **Email**: support@tesaiot.com

---

## License

MIT License - See [LICENSE](./LICENSE) for details.

---

*Last updated: December 2025*
