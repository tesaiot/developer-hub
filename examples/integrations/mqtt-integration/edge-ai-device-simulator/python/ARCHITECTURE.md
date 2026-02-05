# Python Implementation Architecture

## Overview

MQTT-based IoT device simulator using the `paho-mqtt` library to publish sensor telemetry with anomaly injection for Edge AI testing.

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
│  │  │ dotenv      │  │ Config      │  │ Signal       │   │  │
│  │  │ loader      │  │ Parser      │  │ Handler      │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘   │  │
│  │         │                │                │           │  │
│  │         └────────────────┴────────────────┘           │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │       InfineonDPS368Simulator Class           │    │  │
│  │  │                                               │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ paho.mqtt    │  │ Callback Methods    │    │    │  │
│  │  │  │ Client       │  │ - _on_connect()     │    │    │  │
│  │  │  │              │  │ - _on_disconnect()  │    │    │  │
│  │  │  │              │  │ - _on_publish()     │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │         Data Generation Layer                 │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ Normal Data  │  │ Anomaly Data        │    │    │  │
│  │  │  │ Generator    │  │ Generator           │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │              Output Layer                     │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ MQTT Publish │  │ Console Logger      │    │    │  │
│  │  │  │ (QoS 1)      │  │                     │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Class Structure

```python
class InfineonDPS368Simulator:
    """IoT Device Simulator for Infineon XENSIV DPS368 Sensor."""

    # Sensor specifications
    PRESSURE_MIN = 950.0    # hPa (normal range)
    PRESSURE_MAX = 1050.0   # hPa (normal range)
    TEMP_MIN = 20.0         # °C (normal range)
    TEMP_MAX = 35.0         # °C (normal range)
    ANOMALY_TEMP = 85.0     # High temperature anomaly
    ANOMALY_PRESSURE = 1200.0  # High pressure anomaly

    def __init__(self):
        # Load configuration from environment
        # Create paho-mqtt client
        # Set callbacks
        # Configure auto-reconnect

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        # Handle connection success/failure
        # Log connection status

    def _on_disconnect(self, client, userdata, rc, properties=None):
        # Handle disconnection
        # Trigger reconnect if needed

    def _on_publish(self, client, userdata, mid, reason_code=None, properties=None):
        # Track published messages

    def _generate_normal_data(self) -> dict:
        # Generate random pressure and temperature within normal range

    def _generate_anomaly_data(self) -> dict:
        # Generate anomalous values for Edge AI testing

    def _generate_telemetry(self, is_anomaly: bool = False) -> dict:
        # Create complete telemetry payload with schema

    def _publish_telemetry(self) -> None:
        # Determine if anomaly should be injected
        # Generate and publish payload

    def _handle_shutdown(self, signum, frame) -> None:
        # Graceful shutdown on SIGINT/SIGTERM

    def run(self) -> None:
        # Main execution loop
        # Connect, publish, repeat
```

## paho-mqtt Version Handling

```python
# Support both paho-mqtt 1.x and 2.x
try:
    # paho-mqtt 2.x style
    self.client = mqtt.Client(
        client_id=self.client_id,
        protocol=mqtt.MQTTv311,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    )
    self._paho_v2 = True
except (AttributeError, TypeError):
    # paho-mqtt 1.x fallback
    self.client = mqtt.Client(
        client_id=self.client_id,
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
│  (DPS368Sim)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Register       │
│  Signal Handler │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│  client.connect │────▶│  _on_disconnect │
│  (localhost)    │     │  Auto-reconnect │
└────────┬────────┘     └────────┬────────┘
         │                       │
         ▼                       │
┌─────────────────┐              │
│  _on_connect    │◀─────────────┘
│  (connected)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Publishing     │
│    Loop         │◀───┐
└────────┬────────┘    │
         │             │
         ▼             │
┌─────────────────┐    │
│ _publish_telem  │    │
│ (every 1 sec)   │────┘
└────────┬────────┘
         │
    SIGINT/SIGTERM
         │
         ▼
┌─────────────────┐
│  Graceful       │
│  Shutdown       │
└─────────────────┘
```

## Data Generation Flow

```
┌──────────────────────────────────────────────────────────────┐
│                    Data Generation Logic                     │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  _publish_telemetry()                                        │
│         │                                                    │
│         ▼                                                    │
│  ┌─────────────────────┐                                     │
│  │ packet_count += 1   │                                     │
│  └──────────┬──────────┘                                     │
│             │                                                │
│             ▼                                                │
│  ┌──────────────────────────────┐                            │
│  │  packet_count % 10 == 0 ?    │                            │
│  └──────────┬───────────────────┘                            │
│             │                                                │
│     ┌───────┴───────┐                                        │
│     │               │                                        │
│    YES              NO                                       │
│     │               │                                        │
│     ▼               ▼                                        │
│  ┌────────┐   ┌──────────┐                                   │
│  │ Anomaly│   │  Normal  │                                   │
│  │ Data   │   │  Data    │                                   │
│  └───┬────┘   └────┬─────┘                                   │
│      │             │                                         │
│      └──────┬──────┘                                         │
│             │                                                │
│             ▼                                                │
│  ┌──────────────────────┐                                    │
│  │ _generate_telemetry  │                                    │
│  │   - timestamp        │                                    │
│  │   - device_id        │                                    │
│  │   - sensor_type      │                                    │
│  │   - data{}           │                                    │
│  └──────────┬───────────┘                                    │
│             │                                                │
│             ▼                                                │
│  ┌──────────────────────┐                                    │
│  │  json.dumps(payload) │                                    │
│  └──────────┬───────────┘                                    │
│             │                                                │
│             ▼                                                │
│  ┌──────────────────────┐                                    │
│  │  client.publish()    │                                    │
│  │  topic, payload,qos=1│                                    │
│  └──────────────────────┘                                    │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

## Configuration Layer

```python
# Environment-based configuration
class InfineonDPS368Simulator:
    def __init__(self):
        self.broker_host = os.getenv("MQTT_BROKER_HOST", "localhost")
        self.broker_port = int(os.getenv("MQTT_BROKER_PORT", "1883"))
        self.device_id = os.getenv("DEVICE_ID", "infineon-sensor-01")
        self.topic = os.getenv("MQTT_TOPIC", f"device/{self.device_id}/telemetry")
        self.publish_interval = float(os.getenv("PUBLISH_INTERVAL", "1.0"))
        self.anomaly_interval = int(os.getenv("ANOMALY_INTERVAL", "10"))
```

## Error Handling

```python
def _on_connect(self, client, userdata, flags, rc, properties=None):
    # paho 1.x passes rc as int
    # paho 2.x passes reason_code object
    rc_value = (
        rc if isinstance(rc, int)
        else rc.value if hasattr(rc, 'value') else 0
    )

    if rc_value == 0:
        self.connected = True
        print(f"Connected to MQTT Broker")
    else:
        self.connected = False
        print(f"Connection failed: {rc_value}")
```

## Signal Handling (Graceful Shutdown)

```python
def _handle_shutdown(self, signum: int, frame) -> None:
    """Handle graceful shutdown on SIGINT/SIGTERM."""
    print(f"Shutting down...")
    print(f"Total packets published: {self.total_published}")
    print(f"Total anomalies injected: {self.total_anomalies}")
    self.running = False

def run(self) -> None:
    # Register signal handlers
    signal.signal(signal.SIGINT, self._handle_shutdown)
    signal.signal(signal.SIGTERM, self._handle_shutdown)

    # ... rest of run method
```

## Key Design Decisions

| Decision           | Rationale                                      |
| ------------------ | ---------------------------------------------- |
| Class-based design | Encapsulation, state management, extensibility |
| paho-mqtt 1.x/2.x  | Support older systems and newer installations  |
| loop_start()       | Non-blocking network loop in background thread |
| Environment config | 12-factor app compliance, easy deployment      |
| Signal handling    | Clean shutdown, statistics reporting           |
| Type hints         | Better IDE support, documentation              |
| docstrings         | Self-documenting code                          |

## Dependencies

```
paho-mqtt>=2.0.0      # MQTT client library
python-dotenv>=1.0.0  # Environment file loader
```

## Performance Characteristics

| Metric          | Value          | Notes                             |
| --------------- | -------------- | --------------------------------- |
| Startup Time    | < 1 sec        | Client initialization             |
| Memory Usage    | ~20 MB         | Python runtime + libraries        |
| CPU Usage       | < 1%           | Mostly sleeping between publishes |
| Network I/O     | ~150 bytes/sec | JSON payload size                 |
| Reconnect Delay | 1-30 sec       | Exponential backoff               |
