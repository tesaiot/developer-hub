# TESAIoT WSS Live Data Streaming

Real-time MQTT data streaming via WebSocket Secure (WSS) for TESAIoT Platform. Multi-language examples for subscribing to device telemetry in real-time.

## Overview

This collection provides production-ready examples for connecting to the TESAIoT MQTT broker via WebSocket Secure (WSS) protocol. Each example demonstrates best practices for authentication, error handling, and message processing.

### WSS Connection Details

| Property | Value                              |
| -------- | ---------------------------------- |
| Protocol | WSS (WebSocket Secure)             |
| Host     | `mqtt.tesaiot.com`                 |
| Port     | `8085`                             |
| Path     | `/mqtt`                            |
| Full URL | `wss://mqtt.tesaiot.com:8085/mqtt` |

### Authentication

Third-party applications authenticate using **MQTT API Tokens** generated from the TESAIoT Admin UI.

| Parameter    | Value                              |
| ------------ | ---------------------------------- |
| Username     | `<token>`                          |
| Password     | `<token>` (same value)             |
| Token Format | `tesa_mqtt_<org_prefix>_<32chars>` |

## Available Examples

| Language | Folder                 | Library        | Platform Support                    | Status   |
| -------- | ---------------------- | -------------- | ----------------------------------- | -------- |
| Node.js  | [nodejs/](./nodejs/)   | `mqtt.js`      | macOS, Linux, Windows, Raspberry Pi | ✅ Ready |
| Python   | [python/](./python/)   | `paho-mqtt`    | macOS, Linux, Windows, Raspberry Pi | ✅ Ready |
| Rust     | [rust/](./rust/)       | `rumqttc`      | macOS, Linux, Windows               | ✅ Ready |
| C        | [c/](./c/)             | `libmosquitto` | Linux, Raspberry Pi, macOS          | ✅ Ready |
| Node-RED | [nodered/](./nodered/) | Built-in MQTT  | All platforms via Docker            | ✅ Ready |

## Quick Start

### Step 1: Generate MQTT API Token

1. Log in to [TESAIoT Admin Portal](https://admin.tesaiot.com)
2. Navigate to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Streaming App")
5. Set expiration period (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

### Step 2: Choose Your Language

#### Node.js

```bash
cd nodejs
npm install
cp .env.example .env
# Edit .env with your token
npm start
```

#### Python

```bash
cd python
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt
cp .env.example .env
# Edit .env with your token
python main.py
```

#### Rust

```bash
cd rust
cp .env.example .env
# Edit .env with your token
cargo run
```

#### C

```bash
cd c
make
cp .env.example .env
# Edit .env with your token
./wss_mqtt_client
```

#### Node-RED

```bash
cd nodered
cp .env.example .env
# Edit .env with your token
docker-compose up -d
# Open http://localhost:1880
```

## Topic Structure

| Topic Pattern                       | Description                        | Access          |
| ----------------------------------- | ---------------------------------- | --------------- |
| `device/+/telemetry/#`              | All telemetry from all org devices | Subscribe only  |
| `device/<id>/telemetry/#`           | All telemetry from specific device | Subscribe only  |
| `device/<id>/telemetry/temperature` | Specific sensor type               | Subscribe only  |
| `device/<id>/commands/*`            | Device commands                    | **Not allowed** |

> **Note:** API tokens can only **subscribe** to telemetry topics. Publishing is not permitted.

## Message Format

Telemetry messages are JSON-encoded:

```json
{
  "timestamp": "2026-02-04T10:30:45.123Z",
  "device_id": "5a96f40c-1762-4ff3-b570-bdf809e5e695",
  "sensor": "temperature",
  "value": 25.5,
  "unit": "°C",
  "metadata": {
    "location": "Factory-A",
    "quality": 0.98
  }
}
```

## Security Considerations

1. **Token Security**
   - Never commit tokens to version control
   - Use environment variables in production
   - Rotate tokens before expiration

2. **Connection Security**
   - Always use WSS (not WS) in production
   - Verify TLS certificates in production
   - Use secure credential storage

3. **Best Practices**
   - Implement reconnection logic with exponential backoff
   - Handle disconnection gracefully
   - Log connection events for debugging

## Troubleshooting

| Issue                 | Solution                                     |
| --------------------- | -------------------------------------------- |
| Connection refused    | Verify token format starts with `tesa_mqtt_` |
| Authentication failed | Check token is not expired in Admin UI       |
| No messages received  | Confirm devices are publishing telemetry     |
| TLS errors            | Ensure system CA certificates are up to date |

## Further Reading

- [TESAIoT Documentation](https://docs.tesaiot.com)
- [MQTT.js Documentation](https://github.com/mqttjs/MQTT.js)
- [Paho MQTT Python](https://eclipse.dev/paho/index.php?page=clients/python/index.php)
- [rumqttc (Rust)](https://docs.rs/rumqttc/latest/rumqttc/)
- [Eclipse Mosquitto](https://mosquitto.org/)

## License

Apache 2.0 - See [LICENSE](../../../LICENSE)
