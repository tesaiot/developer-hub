# WSS MQTT Streaming Node.js Client - Architecture

## Overview

Node.js client for real-time telemetry streaming from TESAIoT Platform via WebSocket Secure (WSS) MQTT. Uses the mqtt.js library for secure, event-driven communication with automatic reconnection.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Node.js Application                         │
├─────────────────────────────────────────────────────────────────┤
│  index.js                                                       │
│  ├── Configuration                                              │
│  │   ├── MQTT_API_TOKEN        → Token authentication           │
│  │   ├── MQTT_BROKER_URL       → WSS endpoint                   │
│  │   ├── MQTT_SUBSCRIBE_TOPIC  → Topic pattern                  │
│  │   └── MQTT_CLIENT_ID        → Unique client ID               │
│  ├── mqtt.connect()            → Create WSS connection          │
│  └── Event Handlers                                             │
│      ├── on('connect')         → Subscribe to topics            │
│      ├── on('message')         → Process telemetry              │
│      ├── on('error')           → Handle connection errors       │
│      ├── on('offline')         → Handle disconnect              │
│      └── on('reconnect')       → Auto-reconnect                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ WSS (TLS 1.2+) Port 8085
                              │ Token Authentication
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform (EMQX)                      │
├─────────────────────────────────────────────────────────────────┤
│  WSS Endpoint: wss://mqtt.tesaiot.com:8085/mqtt                 │
│  ├── Token Validation                                           │
│  │   └── Verify tesa_mqtt_* format token                        │
│  ├── Topic Authorization                                        │
│  │   └── Organization-scoped device access                      │
│  └── Message Routing                                            │
│      └── device/<device_id>/telemetry/<sensor_type>             │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │ MQTT Publish
                              │
┌─────────────────────────────────────────────────────────────────┐
│                      IoT Devices                                │
│  ├── Temperature sensors                                        │
│  ├── Humidity sensors                                           │
│  ├── Motion detectors                                           │
│  └── Custom telemetry                                           │
└─────────────────────────────────────────────────────────────────┘
```

## Connection Flow

```
┌──────────┐     ┌───────────────┐     ┌─────────────────┐
│  Start   │────►│ Load .env     │────►│ Validate Token  │
│          │     │ dotenv.config │     │ tesa_mqtt_*     │
└──────────┘     └───────────────┘     └───────┬─────────┘
                                               │
                 ┌─────────────────────────────┘
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    mqtt.connect() Options                        │
│  {                                                              │
│    username: token,           // Token as username              │
│    password: token,           // Token as password              │
│    clientId: 'tesaiot-...',   // Unique per connection          │
│    rejectUnauthorized: false, // For self-signed certs          │
│    reconnectPeriod: 5000,     // 5 second reconnect             │
│    connectTimeout: 30000,     // 30 second timeout              │
│    keepalive: 60,             // 60 second keepalive            │
│    clean: true                // Clean session                  │
│  }                                                              │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Event-Driven Processing                       │
│                                                                 │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌──────────┐     │
│  │ connect │───►│subscribe│───►│ message │───►│ process  │     │
│  │  event  │    │ topics  │    │  event  │    │telemetry │     │
│  └─────────┘    └─────────┘    └─────────┘    └──────────┘     │
│                                                                 │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐                     │
│  │  error  │    │ offline │    │reconnect│                     │
│  │  event  │    │  event  │    │  event  │                     │
│  └─────────┘    └─────────┘    └─────────┘                     │
└─────────────────────────────────────────────────────────────────┘
```

## Message Processing

```javascript
// Topic parsing
const topic = 'device/5a96f40c-1762-4ff3-b570-bdf809e5e695/telemetry/temperature';
const topicParts = topic.split('/');
// topicParts[0] = 'device'
// topicParts[1] = '5a96f40c-1762-4ff3-b570-bdf809e5e695'  // device_id
// topicParts[2] = 'telemetry'
// topicParts[3] = 'temperature'                           // sensor_type

// Payload parsing
const payload = '{"value": 25.5, "unit": "celsius"}';
const data = JSON.parse(payload);
```

## Authentication

```
┌────────────────────────────────────────────────────────────────┐
│                    Token-Based Authentication                   │
├────────────────────────────────────────────────────────────────┤
│  Token Format: tesa_mqtt_<org_prefix>_<32_random_chars>        │
│  Example: tesa_mqtt_myorg_a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6     │
│                                                                 │
│  MQTT Connection:                                               │
│  ├── Username: <token>                                          │
│  └── Password: <token>                                          │
│                                                                 │
│  Generated from: Admin UI > Organization Settings > MQTT Tokens │
│  Expiration: Configurable (default 90 days)                    │
│  Scope: Organization-level device access                       │
└────────────────────────────────────────────────────────────────┘
```

## Topic Patterns

```
┌──────────────────────────────────────────────────────────────┐
│  Pattern                          │ Description              │
├──────────────────────────────────────────────────────────────┤
│  device/+/telemetry/#             │ All devices, all sensors │
│  device/<device_id>/telemetry/#   │ Specific device          │
│  device/<device_id>/telemetry/temp│ Specific sensor          │
│  device/+/telemetry/temperature   │ All devices, temp only   │
└──────────────────────────────────────────────────────────────┘

Wildcards:
  + = Single-level (matches one segment)
  # = Multi-level (matches zero or more segments)
```

## Dependencies

```json
{
  "mqtt": "^5.3.0",     // MQTT client with WebSocket support
  "dotenv": "^16.3.0"   // Environment variable management
}
```

## Files

```
wss-mqtt-streaming/nodejs/
├── index.js             # Main client implementation
├── package.json         # Node.js dependencies
├── package-lock.json    # Dependency lock file
├── .env.example         # Environment template
├── node_modules/        # Installed dependencies
├── README.md            # Usage documentation
└── ARCHITECTURE.md      # This file
```

## Extensibility

Implement `processTelemetry()` for custom business logic:

```javascript
function processTelemetry(deviceId, sensorType, data) {
  // Store in database (PostgreSQL, MongoDB, InfluxDB)
  // Send alerts (email, SMS, push notification)
  // Update real-time dashboard (WebSocket, Socket.io)
  // Forward to cloud (AWS IoT, Azure IoT Hub, GCP IoT Core)
}
```

## Error Handling

| Error | Meaning | Solution |
|-------|---------|----------|
| `Not authorized` | Invalid/expired token | Regenerate token |
| `unable to verify certificate` | SSL cert issue | Set `rejectUnauthorized: false` or install CA |
| `ECONNREFUSED` | Broker unreachable | Check network/firewall |
| `ETIMEDOUT` | Connection timeout | Increase `connectTimeout` |

## Graceful Shutdown

```javascript
process.on('SIGINT', () => {
  client.end(true, () => {
    process.exit(0);
  });
});
```
