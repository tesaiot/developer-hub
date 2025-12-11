---
title: "API Reference"
permalink: /api/
toc: true
toc_sticky: true
---

# TESAIoT Platform API Reference

Quick reference for TESAIoT Platform APIs used in the examples.

## MQTT API

### Connection Parameters

| Parameter | Value |
|-----------|-------|
| Broker | `mqtt.tesaiot.com` |
| TLS Port | `8883` |
| WSS Port | `8085` |
| Protocol | MQTT 3.1.1 / 5.0 |

### Authentication

#### mTLS (Certificate)

```
CA Certificate: ca.crt
Client Certificate: device.crt
Client Key: device.key
```

#### Token (Username/Password)

```
Username: device_id or tesa_mqtt_<org>_<token>
Password: api_token (or empty for MQTT tokens)
```

### Topic Structure

```
# Telemetry (Device → Platform)
device/{device_id}/telemetry/{type}

# Commands (Platform → Device)
device/{device_id}/commands/{command}

# Status (Device → Platform)
device/{device_id}/status

# Events (Platform → Device)
device/{device_id}/events/{event_type}
```

### Telemetry Payload Format

```json
{
  "timestamp": "2024-01-15T10:30:00.000Z",
  "temperature": 25.5,
  "humidity": 60.0,
  "pressure": 1013.25,
  "battery": 85
}
```

### QoS Levels

| QoS | Name | Use Case |
|-----|------|----------|
| 0 | At most once | High-frequency telemetry |
| 1 | At least once | Commands, events |
| 2 | Exactly once | Critical operations |

---

## REST API

### Base URL

```
https://admin.tesaiot.com/api/v1
```

### Authentication

Include API token in header:

```
Authorization: Bearer <api_token>
```

### Common Endpoints

#### Health Check

```http
GET /health
```

Response:
```json
{
  "status": "healthy",
  "version": "2024.12",
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

#### Get Device

```http
GET /devices/{device_id}
```

Response:
```json
{
  "id": "device-001",
  "name": "Sensor Alpha",
  "type": "environmental",
  "status": "online",
  "last_seen": "2024-01-15T10:30:00.000Z",
  "metadata": {
    "location": "Building A",
    "firmware": "1.2.3"
  }
}
```

#### Get Telemetry

```http
GET /devices/{device_id}/telemetry?from={timestamp}&to={timestamp}
```

Query Parameters:
- `from`: Start timestamp (ISO 8601)
- `to`: End timestamp (ISO 8601)
- `type`: Telemetry type filter
- `limit`: Maximum records (default: 100)

Response:
```json
{
  "device_id": "device-001",
  "telemetry": [
    {
      "timestamp": "2024-01-15T10:30:00.000Z",
      "type": "environment",
      "data": {
        "temperature": 25.5,
        "humidity": 60.0
      }
    }
  ],
  "pagination": {
    "total": 1000,
    "limit": 100,
    "offset": 0
  }
}
```

---

## WebSocket API

### Streaming Endpoint

```
wss://mqtt.tesaiot.com:8085/mqtt
```

### MQTT over WebSocket

Use mqtt.js library for browser connections:

```javascript
import mqtt from 'mqtt';

const client = mqtt.connect('wss://mqtt.tesaiot.com:8085/mqtt', {
  username: 'tesa_mqtt_your-org_token...',
  password: '',
  protocolVersion: 5,
  clean: true,
  keepalive: 60,
});

client.on('connect', () => {
  client.subscribe('device/+/telemetry/#');
});

client.on('message', (topic, payload) => {
  const data = JSON.parse(payload.toString());
  console.log(topic, data);
});
```

---

## MQTT API Tokens

### Token Format

```
tesa_mqtt_{organization}_{32_character_token}
```

Example:
```
tesa_mqtt_acme-corp_a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
```

### Obtaining Tokens

1. Log in to [Admin Portal](https://admin.tesaiot.com)
2. Navigate to **Settings → MQTT API Tokens**
3. Click **Create Token**
4. Set permissions and expiration
5. Copy the generated token

### Token Permissions

Tokens can be scoped to:
- Specific organizations
- Specific topic patterns
- Read-only or read-write
- Time-limited expiration

---

## Error Codes

### MQTT Error Codes

| Code | Description | Action |
|------|-------------|--------|
| 0x04 | Bad username/password | Check credentials |
| 0x05 | Not authorized | Check topic permissions |
| 0x87 | Session taken over | Another client with same ID |
| 0x8F | Connection rate exceeded | Implement backoff |

### REST API Error Codes

| Status | Code | Description |
|--------|------|-------------|
| 400 | BAD_REQUEST | Invalid request format |
| 401 | UNAUTHORIZED | Missing or invalid token |
| 403 | FORBIDDEN | Insufficient permissions |
| 404 | NOT_FOUND | Resource not found |
| 429 | RATE_LIMITED | Too many requests |
| 500 | INTERNAL_ERROR | Server error |

---

## Rate Limits

| API | Limit | Window |
|-----|-------|--------|
| REST API | 100 requests | per minute |
| MQTT Connect | 10 connections | per minute |
| MQTT Publish | 1000 messages | per minute |
| WebSocket | 50 connections | per IP |

---

## SDKs and Libraries

### Official

- **Python**: paho-mqtt + requests
- **JavaScript**: mqtt.js + axios
- **Go**: eclipse/paho.mqtt.golang

### Community

Check [GitHub Discussions](https://github.com/tesaiot/developer-hub/discussions) for community SDKs.
