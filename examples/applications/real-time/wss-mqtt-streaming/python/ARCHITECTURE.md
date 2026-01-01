# WSS MQTT Streaming Python Client - Architecture

## Overview

Python client for real-time telemetry streaming from TESAIoT Platform via WebSocket Secure (WSS) MQTT. Uses paho-mqtt library with WebSocket transport for secure, bidirectional communication.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      Python Application                         │
├─────────────────────────────────────────────────────────────────┤
│  main.py                                                        │
│  ├── TESAIoTMQTTClient class                                    │
│  │   ├── __init__()           → Initialize client with config   │
│  │   ├── connect()            → Connect to WSS broker           │
│  │   ├── run_forever()        → Start event loop                │
│  │   ├── disconnect()         → Graceful shutdown               │
│  │   └── process_telemetry()  → Override for custom logic       │
│  └── Callbacks                                                  │
│      ├── _on_connect()        → Subscribe on connection         │
│      ├── _on_message()        → Parse and process telemetry     │
│      ├── _on_disconnect()     → Handle reconnection             │
│      └── _on_subscribe()      → Confirm subscription            │
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
│          │     │ MQTT_API_TOKEN│     │ tesa_mqtt_*     │
└──────────┘     └───────────────┘     └───────┬─────────┘
                                               │
                 ┌─────────────────────────────┘
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    WSS Connection Setup                          │
│  1. Parse broker URL (host, port, path)                         │
│  2. Create paho.mqtt.Client with WebSocket transport            │
│  3. Configure TLS (CERT_NONE for self-signed)                   │
│  4. Set username/password (both = token)                        │
│  5. Connect to mqtt.tesaiot.com:8085/mqtt                       │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Subscribe to Topics                           │
│  Pattern: device/+/telemetry/#                                  │
│  ├── + = wildcard for any device_id                             │
│  └── # = wildcard for any sensor_type                           │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Message Processing Loop                       │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ On Message Received:                                     │    │
│  │ 1. Parse topic → extract device_id, sensor_type         │    │
│  │ 2. Decode JSON payload                                   │    │
│  │ 3. Log timestamp, device, sensor, data                   │    │
│  │ 4. Call process_telemetry() for custom logic             │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

## Message Format

```
Topic: device/<device_id>/telemetry/<sensor_type>
Example: device/5a96f40c-1762-4ff3-b570-bdf809e5e695/telemetry/temperature

Payload (JSON):
{
  "value": 25.5,
  "unit": "celsius",
  "timestamp": "2025-12-09T10:30:45.000Z"
}
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

## paho-mqtt Compatibility

```python
# Supports both paho-mqtt 1.x and 2.x
try:
    # paho-mqtt 2.x style
    client = mqtt.Client(
        client_id=client_id,
        transport="websockets",
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    )
except (AttributeError, TypeError):
    # paho-mqtt 1.x fallback
    client = mqtt.Client(
        client_id=client_id,
        transport="websockets",
    )
```

## Dependencies

```
paho-mqtt>=1.6.0      # MQTT client with WebSocket support
python-dotenv>=1.0.0  # Environment variable management
pytest>=7.0.0         # Testing framework (optional)
```

## Files

```
wss-mqtt-streaming/python/
├── main.py              # Main client implementation
├── requirements.txt     # Python dependencies
├── .env.example         # Environment template
├── tests/
│   ├── __init__.py
│   └── test_client.py   # Unit tests
├── README.md            # Usage documentation
└── ARCHITECTURE.md      # This file
```

## Extensibility

Override `process_telemetry()` for custom business logic:

```python
def process_telemetry(self, device_id: str, sensor_type: str, data: Any):
    # Store in database
    # Send alerts
    # Update dashboard
    # Forward to cloud services
    pass
```

## Error Handling

| Error Code | Meaning | Solution |
|------------|---------|----------|
| 5 | Not authorized | Check token validity |
| SSL Error | Certificate verify failed | Install CA certs or use CERT_NONE |
| Timeout | Connection timeout | Check network/firewall |
