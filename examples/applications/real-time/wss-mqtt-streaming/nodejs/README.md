# TESAIoT WSS MQTT - Node.js Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Node.js.

## Prerequisites

1. **Node.js** v16 or higher
2. **npm** or **yarn**
3. **TESAIoT MQTT API Token** - Generate from Admin UI

## Getting Started

### 1. Install Dependencies

**macOS:**
```bash
# Install Node.js via Homebrew (if not installed)
brew install node

# Install npm packages
npm install
```

**Linux (Ubuntu/Debian):**
```bash
# Install Node.js (v18+)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Install npm packages
npm install
```

**Raspberry Pi:**
```bash
# Install Node.js
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Install npm packages
npm install
```

**Windows (PowerShell):**
```powershell
# Download Node.js from https://nodejs.org/
# Then run:
npm install
```

### 2. Generate MQTT API Token

1. Log in to TESAIoT Admin UI (https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Dashboard App")
5. Set expiration (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

### 3. Configure Environment

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

### 4. Run the Example

```bash
npm start
```

You should see:

```
Connecting to TESAIoT MQTT Broker via WSS...
  Broker URL: wss://mqtt.tesaiot.com:8085/mqtt
  Client ID: tesaiot-third-party-1702123456789
  Token: tesa_mqtt_yourorg_...xxxx
Connected to TESAIoT MQTT Broker!
Subscribed to topics:
  - device/+/telemetry/# (QoS 1)

Waiting for telemetry messages...
```

When devices publish telemetry, you'll see:

```
[2025-12-09T10:30:45.123Z] Device: 5a96f40c-1762-4ff3-b570-bdf809e5e695
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

### Store in Database

```javascript
const { Pool } = require('pg');
const pool = new Pool({ connectionString: process.env.DATABASE_URL });

function processTelemetry(deviceId, sensorType, data) {
  pool.query(
    'INSERT INTO telemetry (device_id, sensor_type, data, timestamp) VALUES ($1, $2, $3, NOW())',
    [deviceId, sensorType, JSON.stringify(data)]
  );
}
```

### Send Alerts

```javascript
const nodemailer = require('nodemailer');

function processTelemetry(deviceId, sensorType, data) {
  if (sensorType === 'temperature' && data.value > 50) {
    sendAlert(`High temperature alert: ${data.value}°C on device ${deviceId}`);
  }
}
```

### Forward to Cloud

```javascript
const AWS = require('aws-sdk');
const iot = new AWS.IotData({ endpoint: 'your-iot-endpoint.amazonaws.com' });

function processTelemetry(deviceId, sensorType, data) {
  iot.publish({
    topic: `tesaiot/${deviceId}/${sensorType}`,
    payload: JSON.stringify(data)
  }).promise();
}
```

## Troubleshooting

### Connection Failed

```
MQTT Error: Not authorized
```

**Solution:** Check your token is valid, not expired, and not revoked.

### No Messages Received

1. Verify devices are publishing telemetry
2. Check your subscription topic pattern
3. Ensure devices belong to your organization

### Certificate Error

```
MQTT Error: unable to verify the first certificate
```

**Solution:** Set `rejectUnauthorized: false` for development only. In production, ensure proper CA certificates are installed.

## Security Notes

1. **Never commit `.env` file** - it contains your secret token
2. **Use environment variables** in production (Docker, K8s, etc.)
3. **Rotate tokens regularly** - generate new tokens before expiration
4. **Monitor token usage** - check Admin UI for suspicious activity

## License

MIT
