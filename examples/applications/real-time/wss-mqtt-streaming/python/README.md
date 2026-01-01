# TESAIoT WSS MQTT - Python Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Python.

## Prerequisites

1. **Python** 3.9 or higher
2. **pip** package manager
3. **TESAIoT MQTT API Token** - Generate from Admin UI

## Getting Started

### 1. Create Virtual Environment (Recommended)

```bash
cd tutorial/examples/wss_based_MQTT_API/python

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

1. Log in to TESAIoT Admin UI (https://admin.tesaiot.com)
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
nano .env
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
Connecting to TESAIoT MQTT Broker via WSS...
  Broker: mqtt.tesaiot.com:8085/mqtt
  Client ID: tesaiot-python-1702123456
  Token: tesa_mqtt_yourorg_...xxxx
Connected to TESAIoT MQTT Broker!
Subscribing to: device/+/telemetry/#
Subscribed successfully!

Waiting for telemetry messages...
```

When devices publish telemetry, you'll see:

```
[2025-12-09T10:30:45.123456] Device: 5a96f40c-1762-4ff3-b570-bdf809e5e695
  Sensor: temperature
  Data: {
    "value": 25.5,
    "unit": "celsius",
    "timestamp": "2025-12-09T10:30:45.000Z"
  }
```

## Configuration Options

| Environment Variable | Description | Default |
|---------------------|-------------|---------|
| `MQTT_API_TOKEN` | Your MQTT API token (required) | - |
| `MQTT_BROKER_URL` | WSS broker URL | `wss://mqtt.tesaiot.com:8085/mqtt` |
| `MQTT_SUBSCRIBE_TOPIC` | Topics to subscribe | `device/+/telemetry/#` |
| `MQTT_CLIENT_ID` | Unique client identifier | Auto-generated |

## Topic Patterns

| Pattern | Description |
|---------|-------------|
| `device/+/telemetry/#` | All telemetry from all devices |
| `device/<device_id>/telemetry/#` | All telemetry from specific device |
| `device/<device_id>/telemetry/temperature` | Only temperature from specific device |
| `device/+/telemetry/+/sensor` | Specific sensor type from all devices |

## Extending the Example

### Store in PostgreSQL

```python
import psycopg2

conn = psycopg2.connect(os.getenv("DATABASE_URL"))

def process_telemetry(self, device_id, sensor_type, data):
    with conn.cursor() as cur:
        cur.execute(
            "INSERT INTO telemetry (device_id, sensor_type, data, timestamp) VALUES (%s, %s, %s, NOW())",
            (device_id, sensor_type, json.dumps(data))
        )
        conn.commit()
```

### Store in InfluxDB

```python
from influxdb_client import InfluxDBClient, Point

client = InfluxDBClient(url="http://localhost:8086", token="your-token", org="your-org")
write_api = client.write_api()

def process_telemetry(self, device_id, sensor_type, data):
    point = (
        Point("telemetry")
        .tag("device_id", device_id)
        .tag("sensor_type", sensor_type)
        .field("value", data.get("value", 0))
    )
    write_api.write(bucket="iot", record=point)
```

### Send Alerts via Email

```python
import smtplib
from email.mime.text import MIMEText

def process_telemetry(self, device_id, sensor_type, data):
    if sensor_type == "temperature" and data.get("value", 0) > 50:
        msg = MIMEText(f"High temperature: {data['value']}°C on device {device_id}")
        msg["Subject"] = "Temperature Alert"
        msg["From"] = "alerts@example.com"
        msg["To"] = "admin@example.com"

        with smtplib.SMTP("smtp.example.com") as server:
            server.send_message(msg)
```

### Async Version

```python
import asyncio
import asyncio_mqtt as aiomqtt

async def main():
    async with aiomqtt.Client(
        hostname="mqtt.tesaiot.com",
        port=8085,
        username=token,
        password=token,
        transport="websockets",
        tls_params=aiomqtt.TLSParameters(),
    ) as client:
        async with client.messages() as messages:
            await client.subscribe("device/+/telemetry/#")
            async for message in messages:
                print(f"Received: {message.payload.decode()}")

asyncio.run(main())
```

## Troubleshooting

### Connection Failed

```
Connection failed with reason code: 5
Authentication failed. Check your MQTT API Token.
```

**Solution:** Check your token is valid, not expired, and not revoked.

### SSL Certificate Error

```
ssl.SSLCertVerificationError: certificate verify failed
```

**Solution:** Ensure CA certificates are properly installed:
```bash
pip install certifi
export SSL_CERT_FILE=$(python -m certifi)
```

### No Messages Received

1. Verify devices are publishing telemetry
2. Check your subscription topic pattern
3. Ensure devices belong to your organization

### Import Error

```
ModuleNotFoundError: No module named 'paho'
```

**Solution:** Install dependencies:
```bash
pip install -r requirements.txt
```

## Security Notes

1. **Never commit `.env` file** - it contains your secret token
2. **Use environment variables** in production (Docker, K8s, etc.)
3. **Rotate tokens regularly** - generate new tokens before expiration
4. **Monitor token usage** - check Admin UI for suspicious activity

## Docker Deployment

```dockerfile
FROM python:3.11-slim

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY main.py .

CMD ["python", "main.py"]
```

```bash
docker build -t tesaiot-mqtt-client .
docker run -e MQTT_API_TOKEN=your_token tesaiot-mqtt-client
```

## License

MIT
