# Python Implementation Architecture

## Overview

WebSocket-based MQTT client using the `paho-mqtt` library for real-time telemetry streaming.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Python Application                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  main.py (Entry Point)                │  │
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
│  │  │         TESAIoTMQTTClient Class               │    │  │
│  │  │                                               │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ paho.mqtt    │  │ Callback Methods    │    │    │  │
│  │  │  │ Client       │  │ - _on_connect()     │    │    │  │
│  │  │  │ (websockets) │  │ - _on_message()     │    │    │  │
│  │  │  │              │  │ - _on_disconnect()  │    │    │  │
│  │  │  │              │  │ - _on_subscribe()   │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │           Message Processing                  │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ Topic Parser │  │ process_message()   │    │    │  │
│  │  │  │              │  │ (Custom Handler)    │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Class Structure

```python
class TESAIoTMQTTClient:
    """MQTT client for TESAIoT Platform via WSS."""

    def __init__(self):
        # Load configuration
        # Validate token
        # Create paho-mqtt client
        # Configure TLS
        # Set callbacks

    def _parse_broker_url(self, url: str) -> tuple:
        # Extract host, port, path from URL

    def _validate_config(self):
        # Ensure token is valid

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        # Handle connection success/failure
        # Subscribe to topics

    def _on_message(self, client, userdata, msg):
        # Parse topic and payload
        # Call process_message()

    def _on_disconnect(self, client, userdata, rc, properties=None):
        # Handle disconnection

    def process_message(self, device_id: str, sensor_type: str, data: dict):
        # Custom message handling

    def run(self):
        # Connect and start loop
```

## paho-mqtt Version Handling

```python
# Support both paho-mqtt 1.x and 2.x
try:
    # paho-mqtt 2.x style
    self.client = mqtt.Client(
        client_id=self.client_id,
        transport="websockets",
        protocol=mqtt.MQTTv311,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    )
    self._paho_v2 = True
except (AttributeError, TypeError):
    # paho-mqtt 1.x fallback
    self.client = mqtt.Client(
        client_id=self.client_id,
        transport="websockets",
        protocol=mqtt.MQTTv311,
    )
    self._paho_v2 = False
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
│  (load_dotenv)  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Create Client  │
│  (TESAIoTMQTT)  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│  client.connect │────▶│  _on_disconnect │
└────────┬────────┘     │  Auto-reconnect │
         │              └────────┬────────┘
         ▼                       │
┌─────────────────┐              │
│  _on_connect    │◀─────────────┘
│  Subscribe      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  loop_forever() │
│  _on_message    │◀───┐
└────────┬────────┘    │
         │             │
         ▼             │
┌─────────────────┐    │
│ process_message │────┘
└─────────────────┘
```

## TLS Configuration

```python
import ssl

# Option 1: Use system CA certificates (production)
self.client.tls_set(cert_reqs=ssl.CERT_REQUIRED)

# Option 2: Custom CA certificate
self.client.tls_set(
    ca_certs="/path/to/ca-chain.pem",
    cert_reqs=ssl.CERT_REQUIRED
)

# Option 3: Skip verification (development only)
self.client.tls_set(cert_reqs=ssl.CERT_NONE)
self.client.tls_insecure_set(True)
```

## Key Design Decisions

| Decision                 | Rationale                          |
| ------------------------ | ---------------------------------- |
| Class-based design       | Encapsulation, extensibility       |
| paho-mqtt 1.x/2.x compat | Support older systems              |
| Graceful shutdown        | Clean disconnect on SIGINT/SIGTERM |
| Type hints               | Better IDE support, documentation  |
| docstrings               | Self-documenting code              |

## Error Handling

```python
def _on_connect(self, client, userdata, flags, rc, properties=None):
    # paho 1.x passes rc as int
    # paho 2.x passes reason_code object
    rc_value = rc if isinstance(rc, int) else getattr(rc, 'value', 0)

    if rc_value == 0:
        # Success
        client.subscribe(self.topic, qos=1)
    elif rc_value == 5:
        # Not authorized
        print("Authentication failed - check token")
    else:
        print(f"Connection failed: {rc_value}")
```

## Signal Handling

```python
import signal

def setup_signal_handlers(self):
    def handler(signum, frame):
        print("Shutting down...")
        self.running = False
        self.client.disconnect()

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
```
