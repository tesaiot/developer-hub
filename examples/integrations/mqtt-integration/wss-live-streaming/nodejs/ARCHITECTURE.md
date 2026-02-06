# Node.js Implementation Architecture

## Overview

WebSocket-based MQTT client using the `mqtt.js` library for real-time telemetry streaming.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Node.js Application                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  index.js (Entry Point)               │  │
│  │                                                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐   │  │
│  │  │ dotenv      │  │ Config      │  │ Validation   │   │  │
│  │  │ loader      │  │ Parser      │  │ Layer        │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘   │  │
│  │         │                │                │           │  │
│  │         └────────────────┴────────────────┘           │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │             mqtt.js Client                    │    │  │
│  │  │                                               │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ WebSocket    │  │ Event Handlers      │    │    │  │
│  │  │  │ Transport    │  │ - on('connect')     │    │    │  │
│  │  │  │ (WSS)        │  │ - on('message')     │    │    │  │
│  │  │  │              │  │ - on('error')       │    │    │  │
│  │  │  │              │  │ - on('close')       │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │           Message Processing                  │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ Topic Parser │  │ processMessage()    │    │    │  │
│  │  │  │              │  │ (Custom Handler)    │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Code Flow

```javascript
// 1. Load configuration
require("dotenv").config();
const config = {
  token: process.env.MQTT_API_TOKEN,
  brokerUrl: "wss://mqtt.tesaiot.com:8085/mqtt",
  topic: "device/+/telemetry/#",
};

// 2. Validate token
if (!config.token?.startsWith("tesa_mqtt_")) {
  process.exit(1);
}

// 3. Create client
const client = mqtt.connect(config.brokerUrl, {
  username: config.token,
  password: config.token,
  reconnectPeriod: 5000,
  keepalive: 60,
});

// 4. Handle events
client.on("connect", () => {
  client.subscribe(config.topic, { qos: 1 });
});

client.on("message", (topic, payload) => {
  const data = JSON.parse(payload.toString());
  processMessage(topic, data);
});
```

## Event Lifecycle

```
┌─────────────────┐
│  Application    │
│    Start        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Load .env      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│  mqtt.connect() │────▶│  on('error')    │
└────────┬────────┘     │  Retry after    │
         │              │  5 seconds      │
         ▼              └────────┬────────┘
┌─────────────────┐              │
│  on('connect')  │◀─────────────┘
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  subscribe()    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  on('message')  │◀───┐
│  Loop           │    │
└────────┬────────┘    │
         │             │
         ▼             │
┌─────────────────┐    │
│ processMessage()│────┘
└─────────────────┘
```

## Key Design Decisions

| Decision          | Rationale                          |
| ----------------- | ---------------------------------- |
| Single file       | Simple example, easy to understand |
| dotenv for config | Standard Node.js practice          |
| QoS 1             | Ensures message delivery           |
| Auto-reconnect    | Resilient to network issues        |
| JSON parsing      | Standard telemetry format          |

## Error Handling

```javascript
// Connection errors
client.on("error", (err) => {
  console.error("Connection error:", err.message);
  // mqtt.js handles reconnection automatically
});

// Message parsing errors
client.on("message", (topic, payload) => {
  try {
    const data = JSON.parse(payload.toString());
    processMessage(topic, data);
  } catch (e) {
    console.error("Parse error:", e.message);
    // Log raw payload for debugging
    console.log("Raw:", payload.toString());
  }
});
```

## Memory Management

- mqtt.js handles connection pooling internally
- Messages are processed synchronously by default
- For high-throughput, consider using streams or queues
