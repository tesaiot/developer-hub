# TESAIoT WSS Live Streaming - Python Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Python.

## Prerequisites

| Item           | Version | Description                    |
| -------------- | ------- | ------------------------------ |
| Python         | 3.9+    | Python interpreter             |
| pip            | 21+     | Package manager                |
| MQTT API Token | -       | Generate from TESAIoT Admin UI |

## Getting Started

### 1. Create Virtual Environment (Recommended)

```bash
# Create virtual environment
python -m venv venv

# Activate virtual environment
# On Linux/macOS:
source venv/bin/activate
# On Windows:
venv\Scripts\activate
```

### 2. Install Dependencies

**macOS:**

```bash
pip install -r requirements.txt
```

**Linux (Ubuntu/Debian):**

```bash
sudo apt update && sudo apt install -y python3-pip python3-venv
pip install -r requirements.txt
```

**Raspberry Pi:**

```bash
sudo apt update && sudo apt install -y python3-pip python3-venv
pip install -r requirements.txt
```

**Windows (PowerShell):**

```powershell
pip install -r requirements.txt
```

### 3. Generate MQTT API Token

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Python App")
5. Set expiration (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

### 4. Configure Environment

```bash
# Copy example config
cp .env.example .env

# Edit .env with your token
nano .env  # or use your preferred editor
```

Set your token in `.env`:

```env
MQTT_API_TOKEN=tesa_mqtt_yourorg_xxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### 5. Run the Example

```bash
python main.py
```

You should see:

```
┌─────────────────────────────────────────────────┐
│  TESAIoT WSS Live Streaming - Python Client     │
└─────────────────────────────────────────────────┘

Connecting to TESAIoT MQTT Broker via WSS...
  Broker: mqtt.tesaiot.com:8085
  Path: /mqtt
  Client ID: tesaiot-python-1738678800
  Token: tesa_mqtt_yourorg_...xxxx

✅ Connected to TESAIoT MQTT Broker!
📡 Subscribed to: device/+/telemetry/# (QoS 1)

Waiting for telemetry messages...
──────────────────────────────────────────────────
```

When devices publish telemetry:

```
[2026-02-04T10:30:45.123456] device/abc123/telemetry/temperature
  Device: abc123
  Sensor: temperature
  Data: {"value": 25.5, "unit": "°C"}
```

## Project Structure

```
python/
├── main.py            # Main application entry point
├── requirements.txt   # Dependencies
├── .env.example       # Environment variable template
├── README.md          # This file
└── ARCHITECTURE.md    # Implementation architecture
```

## Configuration Options

| Environment Variable | Default                            | Description              |
| -------------------- | ---------------------------------- | ------------------------ |
| `MQTT_API_TOKEN`     | (required)                         | Your MQTT API token      |
| `MQTT_BROKER_URL`    | `wss://mqtt.tesaiot.com:8085/mqtt` | WSS broker URL           |
| `MQTT_TOPIC`         | `device/+/telemetry/#`             | Topic to subscribe       |
| `MQTT_CLIENT_ID`     | Auto-generated                     | Unique client identifier |

## Extending the Example

### Custom Message Handler

```python
# In main.py, modify the process_message method:
def process_message(self, device_id: str, sensor_type: str, data: dict):
    # Store in database
    self.db.insert({
        "device_id": device_id,
        "sensor": sensor_type,
        "data": data,
        "timestamp": datetime.now()
    })

    # Forward to webhook
    requests.post("https://your-api.com/webhook", json={
        "device_id": device_id,
        "sensor": sensor_type,
        "data": data
    })

    # Trigger alerts
    if data.get("value", 0) > threshold:
        self.send_alert(device_id, data)
```

### Subscribe to Specific Device

```bash
# In .env
MQTT_TOPIC=device/5a96f40c-1762-4ff3-b570-bdf809e5e695/telemetry/#
```

## Troubleshooting

| Issue                            | Solution                                              |
| -------------------------------- | ----------------------------------------------------- |
| `MQTT_API_TOKEN is required`     | Set token in `.env` file                              |
| `Invalid token format`           | Token must start with `tesa_mqtt_`                    |
| `Connection refused`             | Check network and firewall settings                   |
| `Authentication failed`          | Verify token is not expired                           |
| `SSL: CERTIFICATE_VERIFY_FAILED` | Update CA certificates or set `CERT_NONE` for testing |
| `No messages received`           | Confirm devices are publishing                        |

## Dependencies

| Package         | Version | Purpose                            |
| --------------- | ------- | ---------------------------------- |
| `paho-mqtt`     | >=2.0.0 | MQTT client with WebSocket support |
| `python-dotenv` | >=1.0.0 | Environment variable loader        |

## paho-mqtt Version Compatibility

This example supports both paho-mqtt 1.x and 2.x. The code automatically detects the version and uses the appropriate API.

## License

Apache 2.0
