# TESAIoT Edge AI Device Simulator

IoT device simulator for generating realistic sensor telemetry with anomaly injection for Edge AI testing. Demonstrates end-to-end IoT data flow from device to dashboard.

## Overview

This example simulates an **Infineon XENSIV™ DPS368** barometric pressure and temperature sensor, publishing telemetry data to an MQTT broker. The simulator includes a fault injection mechanism that periodically generates anomalous values to test Edge AI anomaly detection systems.

### Use Cases

- **Edge AI Development**: Test anomaly detection algorithms with controlled data
- **Dashboard Development**: Generate realistic telemetry for visualization testing
- **Integration Testing**: Validate MQTT broker and message processing pipelines
- **Training & Demos**: Demonstrate complete IoT data flow scenarios

### Key Features

| Feature               | Description                                    |
| --------------------- | ---------------------------------------------- |
| Realistic Sensor Data | Simulates Infineon DPS368 pressure/temp sensor |
| Auto-Reconnect        | Robust connection with automatic reconnection  |
| Anomaly Injection     | Periodic fault injection for AI testing        |
| Configurable          | Environment-based configuration                |
| Cross-Platform        | Runs on Linux, macOS, Windows, Raspberry Pi    |

## MQTT Connection Details

| Property      | Value                                 |
| ------------- | ------------------------------------- |
| Protocol      | MQTT v3.1.1                           |
| Host          | `localhost` (configurable)            |
| Port          | `1883` (standard MQTT)                |
| Topic Pattern | `device/<device_id>/telemetry`        |
| Default Topic | `device/infineon-sensor-01/telemetry` |
| QoS           | 1 (At least once)                     |

## Data Schema

```json
{
  "timestamp": "2026-02-05T10:00:01.123456+00:00",
  "device_id": "infineon-sensor-01",
  "sensor_type": "DPS368",
  "data": {
    "pressure": 1013.25,
    "temperature": 25.43
  }
}
```

### Field Descriptions

| Field              | Type   | Unit | Normal Range | Description              |
| ------------------ | ------ | ---- | ------------ | ------------------------ |
| `timestamp`        | string | -    | -            | ISO-8601 UTC timestamp   |
| `device_id`        | string | -    | -            | Unique device identifier |
| `sensor_type`      | string | -    | -            | Sensor model identifier  |
| `data.pressure`    | float  | hPa  | 950-1050     | Barometric pressure      |
| `data.temperature` | float  | °C   | 20-35        | Ambient temperature      |

## Available Examples

| Language | Folder               | Library     | Status   |
| -------- | -------------------- | ----------- | -------- |
| Python   | [python/](./python/) | `paho-mqtt` | ✅ Ready |

## Quick Start

### Prerequisites

1. **MQTT Broker** running on localhost:1883

```bash
# Option 1: Docker (recommended)
docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto:2 \
  mosquitto -c /mosquitto-no-auth.conf

# Option 2: macOS
brew install mosquitto && brew services start mosquitto

# Option 3: Ubuntu/Debian
sudo apt install mosquitto && sudo systemctl start mosquitto
```

### Python

```bash
cd python
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt
cp .env.example .env
python main.py
```

## Edge AI Testing Scenario

The simulator is designed to work with Edge AI anomaly detection systems:

```
┌─────────────────┐     ┌────────────────┐     ┌─────────────────┐
│  This Example   │────▶│  MQTT Broker   │────▶│  Edge AI        │
│  (Simulator)    │     │  (EMQX)        │     │  (Anomaly Det.) │
└─────────────────┘     └────────────────┘     └────────┬────────┘
                                                        │
                                                        ▼
                                               ┌─────────────────┐
                                               │  TimescaleDB    │
                                               │  (Storage)      │
                                               └────────┬────────┘
                                                        │
                                                        ▼
                                               ┌─────────────────┐
                                               │  Dashboard      │
                                               │  (Visualization)│
                                               └─────────────────┘
```

### Anomaly Injection Pattern

Every 10th packet (configurable) contains anomalous values:

| Anomaly Type | Description                          | Value Range       |
| ------------ | ------------------------------------ | ----------------- |
| Temperature  | High temperature spike               | > 80 °C           |
| Pressure     | High pressure spike                  | > 1200 hPa        |
| Combined     | Both temperature and pressure spikes | Both out of range |

## Integration with TESAIoT Platform

This simulator can be used with:

1. **[WSS Live Streaming](../wss-live-streaming/)** - Subscribe to simulated data via WebSocket
2. **[AI Services](../../ai-services/)** - Process data through Edge AI anomaly detection
3. **[Grafana Dashboard](../../../applications/visualization/grafana-dashboard/)** - Visualize telemetry and anomalies

## Sensor Reference

### Infineon XENSIV™ DPS368 Specifications

| Parameter            | Min | Typical | Max  | Unit |
| -------------------- | --- | ------- | ---- | ---- |
| Pressure Range       | 300 | -       | 1200 | hPa  |
| Pressure Accuracy    | -   | ±1      | -    | hPa  |
| Temperature Range    | -40 | -       | +85  | °C   |
| Temperature Accuracy | -   | ±0.5    | -    | °C   |
| Sampling Rate        | -   | 128     | -    | Hz   |
| Power Consumption    | -   | 1.7     | -    | μA   |

For more information, see the [Infineon DPS368 Product Page](https://www.infineon.com/cms/en/product/sensor/pressure-sensors/pressure-sensors-for-iot/dps368/).

## Contributing

See [CONTRIBUTING.md](../../../../CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the Apache 2.0 License - see [LICENSE](../../../../LICENSE) for details.
