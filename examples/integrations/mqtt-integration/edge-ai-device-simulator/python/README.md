# TESAIoT Edge AI Device Simulator - Python

This example simulates an IoT device (Infineon XENSIV DPS368) that publishes telemetry data to an MQTT broker with periodic anomaly injection for Edge AI testing.

## Prerequisites

| Item   | Version | Description        |
| ------ | ------- | ------------------ |
| Python | 3.9+    | Python interpreter |
| pip    | 21+     | Package manager    |
| MQTT   | -       | Local MQTT broker  |

## Sensor Information

**Device:** Infineon XENSIV™ DPS368

The DPS368 is a high-performance digital barometric pressure and temperature sensor from Infineon. It offers:

- Pressure measurement range: 300-1200 hPa
- Temperature measurement range: -40°C to +85°C
- High precision: ±0.002 hPa (or ±0.02 m)
- Very low power consumption

## Getting Started

### 1. Start MQTT Broker

Before running the simulator, ensure an MQTT broker is running on localhost:1883.

**Option A: Using Docker (Recommended)**

```bash
# Start Eclipse Mosquitto broker
docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto:2 \
  mosquitto -c /mosquitto-no-auth.conf

# Or with EMQX
docker run -d --name emqx -p 1883:1883 -p 18083:18083 emqx/emqx
```

**Option B: Install Locally (macOS)**

```bash
brew install mosquitto
brew services start mosquitto
```

**Option C: Install Locally (Ubuntu/Debian)**

```bash
sudo apt update && sudo apt install -y mosquitto mosquitto-clients
sudo systemctl start mosquitto
```

### 2. Create Virtual Environment (Recommended)

```bash
# Create virtual environment
python -m venv venv

# Activate virtual environment
# On Linux/macOS:
source venv/bin/activate
# On Windows:
venv\Scripts\activate
```

### 3. Install Dependencies

```bash
pip install -r requirements.txt
```

### 4. Configure Environment (Optional)

```bash
# Copy example config
cp .env.example .env

# Edit if you need to customize settings
nano .env
```

Default configuration:

- Broker: `localhost:1883`
- Device ID: `infineon-sensor-01`
- Topic: `device/infineon-sensor-01/telemetry`
- Publish interval: 1 second
- Anomaly injection: every 10th packet

### 5. Run the Simulator

```bash
python main.py
```

## Expected Output

```
============================================================
  TESAIoT Edge AI Device Simulator
  Sensor: Infineon XENSIV DPS368
============================================================

Connecting to MQTT Broker at localhost:1883...

[2026-02-05 10:00:00] Connected to MQTT Broker: localhost:1883
[2026-02-05 10:00:00] Device ID: infineon-sensor-01
[2026-02-05 10:00:00] Topic: device/infineon-sensor-01/telemetry
[2026-02-05 10:00:00] Publishing every 1.0s (anomaly every 10 packets)
------------------------------------------------------------
[2026-02-05 10:00:01] [NORMAL] Published: Pressure=1013.25 hPa, Temperature=25.43 °C
[2026-02-05 10:00:02] [NORMAL] Published: Pressure=1012.87 hPa, Temperature=24.98 °C
...
[2026-02-05 10:00:10] [ANOMALY] Published: Pressure=1015.32 hPa, Temperature=87.25 °C
```

## Data Schema

Each telemetry message follows this JSON schema:

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

| Field              | Type   | Description                        |
| ------------------ | ------ | ---------------------------------- |
| `timestamp`        | string | ISO-8601 formatted timestamp (UTC) |
| `device_id`        | string | Unique device identifier           |
| `sensor_type`      | string | Sensor model (DPS368)              |
| `data.pressure`    | float  | Barometric pressure in hPa         |
| `data.temperature` | float  | Temperature in °C                  |

## Anomaly Injection

For Edge AI testing, the simulator injects anomalous values every Nth packet (default: 10):

| Type        | Normal Range | Anomaly Value |
| ----------- | ------------ | ------------- |
| Temperature | 20-35 °C     | > 80 °C       |
| Pressure    | 950-1050 hPa | > 1200 hPa    |

Anomaly types are randomly selected:

- Temperature spike only
- Pressure spike only
- Both temperature and pressure spikes

## Subscribing to Telemetry

To verify the simulator is working, subscribe to the topic using another terminal:

```bash
# Using mosquitto_sub
mosquitto_sub -h localhost -p 1883 -t "device/+/telemetry" -v

# Using MQTT.fx, MQTTX, or similar GUI client
# Connect to localhost:1883 and subscribe to device/+/telemetry
```

## Environment Variables

| Variable           | Default                               | Description                    |
| ------------------ | ------------------------------------- | ------------------------------ |
| `MQTT_BROKER_HOST` | `localhost`                           | MQTT broker hostname           |
| `MQTT_BROKER_PORT` | `1883`                                | MQTT broker port               |
| `DEVICE_ID`        | `infineon-sensor-01`                  | Device identifier              |
| `MQTT_TOPIC`       | `device/infineon-sensor-01/telemetry` | Publish topic                  |
| `PUBLISH_INTERVAL` | `1.0`                                 | Seconds between publishes      |
| `ANOMALY_INTERVAL` | `10`                                  | Inject anomaly every N packets |
| `MQTT_CLIENT_ID`   | auto-generated                        | MQTT client identifier         |

## Troubleshooting

### Connection Refused

```
Failed to connect to broker: [Errno 111] Connection refused
```

**Solution:** Ensure MQTT broker is running:

```bash
# Check if broker is running
netstat -an | grep 1883

# Start broker (if using Docker)
docker start mosquitto
```

### No Messages Received

Verify with MQTT subscriber:

```bash
mosquitto_sub -h localhost -p 1883 -t "#" -v
```

## Next Steps

- Connect this simulator to the [WSS Live Streaming](../wss-live-streaming/) example
- Integrate with [Edge AI Service](../../ai-services/) for anomaly detection
- Visualize data on [Grafana Dashboard](../../../applications/visualization/grafana-dashboard/)
