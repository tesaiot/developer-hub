# WSS MQTT Streaming Architecture

## Overview

WebSocket-based MQTT client for subscribing to real-time telemetry from TESAIoT Platform.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Streaming Application                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                 Main Application                         │   │
│  └─────────────────────────┬────────────────────────────────┘   │
│                            │                                    │
│              ┌─────────────┴──────────────┐                     │
│              │                            │                     │
│              ▼                            ▼                     │
│  ┌─────────────────────┐       ┌─────────────────────┐          │
│  │  Python Version     │       │  Node.js Version    │          │
│  │  (paho-mqtt)        │       │  (mqtt.js)          │          │
│  └──────────┬──────────┘       └──────────┬──────────┘          │
│             │                             │                     │
└─────────────┼─────────────────────────────┼─────────────────────┘
              │                             │
              │        WSS (Port 8085)      │
              │           /mqtt             │
              └──────────────┬──────────────┘
                             ▼
                  ┌──────────────────────┐
                  │   EMQX Broker        │
                  │   (WebSocket)        │
                  └──────────────────────┘
```

## Data Flow

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Device     │───▶│  EMQX Broker │───▶│  WebSocket   │
│  (MQTTS)     │    │              │    │  Listener    │
└──────────────┘    └──────────────┘    └──────┬───────┘
                                               │
                                               ▼
                                    ┌──────────────────────┐
                                    │  Message Callback    │
                                    │  (onMessage handler) │
                                    └──────────┬───────────┘
                                               │
                      ┌────────────────────────┼────────────────────────┐
                      │                        │                        │
                      ▼                        ▼                        ▼
              ┌─────────────┐          ┌─────────────┐          ┌─────────────┐
              │  Console    │          │  Database   │          │  Webhook    │
              │  Output     │          │  Storage    │          │  Forward    │
              └─────────────┘          └─────────────┘          └─────────────┘
```

## Python Implementation

```
python/
├── main.py          # Entry point
├── config.py        # Configuration loader
├── mqtt_client.py   # WebSocket MQTT client
└── handlers.py      # Message handlers
```

```python
import paho.mqtt.client as mqtt

client = mqtt.Client(transport="websockets")
client.tls_set()
client.username_pw_set(username=org_token)
client.connect("mqtt.tesaiot.com", 8085)

client.subscribe("device/+/telemetry/#")

@client.on_message
def on_message(client, userdata, msg):
    print(f"{msg.topic}: {msg.payload.decode()}")
```

## Node.js Implementation

```
nodejs/
├── index.js         # Entry point
├── config.js        # Configuration
└── mqtt-client.js   # MQTT client wrapper
```

```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('wss://mqtt.tesaiot.com:8085/mqtt', {
  username: org_token,
  password: ''
});

client.subscribe('device/+/telemetry/#');

client.on('message', (topic, message) => {
  console.log(`${topic}: ${message.toString()}`);
});
```

## Topic Subscriptions

| Pattern | Description |
|---------|-------------|
| `device/+/telemetry/#` | All telemetry from all devices |
| `device/{id}/telemetry/+` | All telemetry types from one device |
| `device/+/status` | Online/offline status all devices |

## Authentication

```
┌─────────────────────────────────────────┐
│         Organization Token Auth         │
├─────────────────────────────────────────┤
│                                         │
│  Username: tesa_mqtt_org_xxxxx          │
│  Password: (empty)                      │
│                                         │
│  Token encodes:                         │
│  - Organization ID                      │
│  - Permissions (subscribe topics)       │
│  - Expiration                           │
│                                         │
└─────────────────────────────────────────┘
```

## Dependencies

### Python
- paho-mqtt - MQTT client with WebSocket support
- python-dotenv - Environment configuration

### Node.js
- mqtt - MQTT.js client
- dotenv - Environment configuration

## Running

```bash
# Python
cd python
pip install -r requirements.txt
python main.py

# Node.js
cd nodejs
npm install
node index.js
```
