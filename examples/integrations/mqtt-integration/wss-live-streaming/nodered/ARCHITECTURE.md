# Node-RED Implementation Architecture

## Overview

Visual flow-based MQTT client using Node-RED's built-in MQTT nodes for real-time telemetry streaming.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Node-RED Runtime                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                    Flow Editor                        │  │
│  │                 http://localhost:1880                 │  │
│  └───────────────────────────────────────────────────────┘  │
│                              │                              │
│                              ▼                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  WSS Streaming Flow                   │  │
│  │                                                       │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐   │  │
│  │  │ mqtt-broker  │  │ mqtt in      │  │ json       │   │  │
│  │  │ (config)     │──│ (subscribe)  │──│ (parse)    │   │  │
│  │  └──────────────┘  └──────────────┘  └─────┬──────┘   │  │
│  │                                            │          │  │
│  │                     ┌──────────────────────┼──────┐   │  │
│  │                     │                      │      │   │  │
│  │                     ▼                      ▼      ▼   │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐   │  │
│  │  │ function     │  │ debug        │  │ dashboard  │   │  │
│  │  │ (process)    │  │ (output)     │  │ (chart)    │   │  │
│  │  └──────────────┘  └──────────────┘  └────────────┘   │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Flow Structure

```json
{
  "nodes": [
    {
      "type": "mqtt-broker",
      "name": "TESAIoT WSS",
      "broker": "mqtt.tesaiot.com",
      "port": 8085,
      "tls": true,
      "protocol": "mqtt"
    },
    {
      "type": "mqtt in",
      "topic": "device/+/telemetry/#",
      "qos": 1
    },
    {
      "type": "json"
    },
    {
      "type": "function",
      "func": "// Parse topic and extract data"
    },
    {
      "type": "debug",
      "active": true
    }
  ]
}
```

## Data Flow

```
┌─────────────────┐
│  EMQX Broker    │
│  (WSS :8085)    │
└────────┬────────┘
         │ WSS + TLS
         ▼
┌─────────────────┐
│  mqtt in node   │
│  (Subscribe)    │
└────────┬────────┘
         │ msg.topic
         │ msg.payload (Buffer)
         ▼
┌─────────────────┐
│  json node      │
│  (Parse JSON)   │
└────────┬────────┘
         │ msg.payload (Object)
         ▼
┌─────────────────┐
│  function node  │
│  (Process)      │
└────────┬────────┘
         │ msg.device_id
         │ msg.sensor_type
         │ msg.data
         │
    ┌────┴────┬────────────┐
    ▼         ▼            ▼
┌────────┐ ┌────────┐ ┌────────┐
│ debug  │ │ chart  │ │ storage│
└────────┘ └────────┘ └────────┘
```

## MQTT Broker Configuration

```
┌─────────────────────────────────────────────────────────────┐
│                  mqtt-broker Config Node                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Connection Tab:                                            │
│  ├─ Server: mqtt.tesaiot.com                                │
│  ├─ Port: 8085                                              │
│  ├─ Connect automatically: ✓                                │
│  ├─ Use TLS: ✓                                              │
│  └─ Protocol: MQTT V3.1.1                                   │
│                                                             │
│  Security Tab:                                              │
│  ├─ Username: tesa_mqtt_yourorg_xxxxx                       │
│  └─ Password: tesa_mqtt_yourorg_xxxxx                       │
│                                                             │
│  Messages Tab:                                              │
│  ├─ Keep alive: 60                                          │
│  └─ Clean session: ✓                                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Function Node Examples

### Parse Topic and Payload

```javascript
// Extract device ID and sensor from topic
// Topic format: device/<device_id>/telemetry/<sensor_type>
const parts = msg.topic.split("/");
msg.device_id = parts[1];
msg.sensor_type = parts.slice(3).join("/") || "default";

// Add timestamp
msg.timestamp = new Date().toISOString();

// Data is already parsed by json node
msg.data = msg.payload;

return msg;
```

### Format for Dashboard

```javascript
// Format for chart display
msg.payload = {
  topic: msg.sensor_type,
  payload: msg.data.value,
  timestamp: new Date(),
};

return msg;
```

### Threshold Alert

```javascript
// Check temperature threshold
if (msg.sensor_type === "temperature" && msg.data.value > 30) {
  msg.payload = {
    alert: "High temperature",
    device: msg.device_id,
    value: msg.data.value,
    threshold: 30,
  };
  return msg;
}
return null; // Don't forward normal readings
```

## Docker Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    docker-compose.yml                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  nodered service                                      │  │
│  │  ├─ Image: nodered/node-red:latest                    │  │
│  │  ├─ Ports: 1880:1880                                  │  │
│  │  ├─ Volumes:                                          │  │
│  │  │   └─ ./data:/data                                  │  │
│  │  └─ Environment:                                      │  │
│  │      ├─ TZ=Asia/Bangkok                               │  │
│  │      └─ MQTT_API_TOKEN=${MQTT_API_TOKEN}              │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Key Features

| Feature        | Implementation                    |
| -------------- | --------------------------------- |
| WSS Connection | mqtt-broker node with TLS enabled |
| Auto-reconnect | Built into Node-RED MQTT nodes    |
| Visual Debug   | debug node with sidebar output    |
| Dashboard      | node-red-dashboard integration    |
| Persistence    | Docker volume for flow storage    |

## Extension Points

1. **Database Storage**: Add MongoDB/PostgreSQL nodes
2. **Alerts**: Add email/Telegram/webhook nodes
3. **Dashboard**: Add charts, gauges, tables
4. **Analytics**: Add function nodes for aggregation
5. **Integration**: Add HTTP request nodes for webhooks
