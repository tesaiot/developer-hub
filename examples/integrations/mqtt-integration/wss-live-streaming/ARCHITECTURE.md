# WSS Live Streaming Architecture

## Overview

WebSocket-based MQTT clients for subscribing to real-time telemetry from TESAIoT Platform.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          TESAIoT Platform                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        EMQX MQTT Broker                                 │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │    │
│  │  │   MQTTS      │  │     WSS      │  │  MQTT-QUIC   │                   │    │
│  │  │  :8884       │  │    :8085     │  │   :14567     │                   │    │
│  │  └──────────────┘  └──────┬───────┘  └──────────────┘                   │    │
│  └───────────────────────────┼─────────────────────────────────────────────┘    │
│                              │                                                  │
└──────────────────────────────┼──────────────────────────────────────────────────┘
                               │ WSS (TLS 1.2+)
                               │ wss://mqtt.tesaiot.com:8085/mqtt
                               │
        ┌──────────────────────┴──────────────────────┐
        │                                             │
        ▼                                             ▼
┌───────────────────┐                     ┌───────────────────┐
│  Third-Party App  │                     │   IoT Dashboard   │
│  (This Example)   │                     │   (React/Vue)     │
└───────────────────┘                     └───────────────────┘
```

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    WSS Streaming Application                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Configuration Layer                    │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │  │
│  │  │ .env File   │  │ Env Vars    │  │ Config Parser   │    │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                   MQTT Client Layer                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │  │
│  │  │ Connection  │  │ TLS/WSS     │  │ Authentication  │    │  │
│  │  │ Manager     │  │ Handler     │  │ (Token)         │    │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                   Message Processing                      │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │  │
│  │  │ Subscribe   │  │ JSON Parse  │  │ Event Handler   │    │  │
│  │  │ Handler     │  │             │  │                 │    │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Output Layer                           │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │  │
│  │  │ Console     │  │ Database    │  │ Webhook/API     │    │  │
│  │  │ Logger      │  │ Writer      │  │ Forwarder       │    │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
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
                                    │  WSS Client          │
                                    │  (mqtt.js/paho/etc)  │
                                    └──────────┬───────────┘
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

## Language-Specific Implementations

### Node.js

```
nodejs/
├── index.js           # Entry point with mqtt.js client
├── package.json       # Dependencies (mqtt, dotenv)
├── .env.example       # Environment template
├── README.md          # Node.js specific docs
└── ARCHITECTURE.md    # Implementation details
```

**Key Components:**

- `mqtt.js` - WebSocket MQTT client
- `dotenv` - Environment configuration
- Native JSON parsing

### Python

```
python/
├── main.py            # Entry point with paho-mqtt
├── requirements.txt   # Dependencies
├── .env.example       # Environment template
├── README.md          # Python specific docs
└── ARCHITECTURE.md    # Implementation details
```

**Key Components:**

- `paho-mqtt` - Eclipse Paho MQTT client
- `python-dotenv` - Environment configuration
- `ssl` - TLS configuration

### Rust

```
rust/
├── Cargo.toml         # Dependencies
├── src/
│   └── main.rs        # rumqttc async client
├── .env.example       # Environment template
├── README.md          # Rust specific docs
└── ARCHITECTURE.md    # Implementation details
```

**Key Components:**

- `rumqttc` - Async MQTT client
- `tokio` - Async runtime
- `dotenv` - Environment configuration
- `serde_json` - JSON parsing

### C

```
c/
├── wss_mqtt_client.c  # libmosquitto client
├── Makefile           # Build configuration
├── .env.example       # Environment template
├── README.md          # C specific docs
└── ARCHITECTURE.md    # Implementation details
```

**Key Components:**

- `libmosquitto` - Eclipse Mosquitto client
- `libwebsockets` - WebSocket support
- `openssl` - TLS support

### Node-RED

```
nodered/
├── flows/
│   └── wss-streaming-flow.json  # Pre-built flow
├── docker-compose.yml           # Container config
├── .env.example                 # Environment template
├── README.md                    # Node-RED specific docs
└── ARCHITECTURE.md              # Implementation details
```

**Key Components:**

- Built-in MQTT nodes
- Dashboard nodes for visualization
- Function nodes for processing

## Connection Lifecycle

```
┌─────────────┐
│   START     │
└──────┬──────┘
       │
       ▼
┌─────────────────────┐
│  Load Configuration │
│  (Token, URL, Topic)│
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Validate Token     │
│  (tesa_mqtt_prefix) │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐     ┌─────────────────┐
│  Connect to Broker  │────▶│  On Error       │
│  (WSS + TLS)        │     │  Retry with     │
└──────────┬──────────┘     │  Backoff        │
           │                └────────┬────────┘
           │                         │
           ▼                         │
┌─────────────────────┐              │
│  Authenticate       │◀─────────────┘
│  (username=token)   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Subscribe to       │
│  Telemetry Topic    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Message Loop       │◀───┐
│  (Process Messages) │    │
└──────────┬──────────┘    │
           │               │
           ▼               │
┌─────────────────────┐    │
│  Handle Message     │────┘
│  (Parse & Output)   │
└─────────────────────┘
```

## Security Model

```
┌─────────────────────────────────────────────────────────────┐
│                     Security Layers                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Layer 1: Transport Security (TLS 1.2+)               │  │
│  │  - Certificate verification                           │  │
│  │  - Encrypted WebSocket connection                     │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Layer 2: Authentication (MQTT API Token)             │  │
│  │  - Token-based authentication                         │  │
│  │  - Organization-scoped access                         │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Layer 3: Authorization (Topic ACL)                   │  │
│  │  - Subscribe-only access                              │  │
│  │  - Organization device filtering                      │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Performance Considerations

| Aspect          | Recommendation                                |
| --------------- | --------------------------------------------- |
| Reconnection    | Exponential backoff (1s, 2s, 4s, 8s, max 30s) |
| Keep-alive      | 60 seconds (default)                          |
| QoS Level       | QoS 1 for reliable delivery                   |
| Message Buffer  | Implement in-memory buffer for burst handling |
| Connection Pool | Single connection per application instance    |
