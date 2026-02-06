# TESAIoT WSS Live Streaming - Node.js Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Node.js.

## Prerequisites

| Item           | Version | Description                    |
| -------------- | ------- | ------------------------------ |
| Node.js        | 18+     | JavaScript runtime             |
| npm            | 9+      | Package manager                |
| MQTT API Token | -       | Generate from TESAIoT Admin UI |

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

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Node.js App")
5. Set expiration (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

### 3. Configure Environment

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

### 4. Run the Example

```bash
npm start
```

You should see:

```
┌─────────────────────────────────────────────────┐
│  TESAIoT WSS Live Streaming - Node.js Client    │
└─────────────────────────────────────────────────┘

Connecting to TESAIoT MQTT Broker via WSS...
  Broker URL: wss://mqtt.tesaiot.com:8085/mqtt
  Client ID: tesaiot-nodejs-1738678800000
  Token: tesa_mqtt_yourorg_...xxxx

✅ Connected to TESAIoT MQTT Broker!
📡 Subscribed to: device/+/telemetry/# (QoS 1)

Waiting for telemetry messages...
```

When devices publish telemetry:

```
[2026-02-04T10:30:45.123Z] device/abc123/telemetry/temperature
  Device: abc123
  Sensor: temperature
  Data: { "value": 25.5, "unit": "°C" }
```

## Project Structure

```
nodejs/
├── index.js         # Main application entry point
├── package.json     # Dependencies and scripts
├── .env.example     # Environment variable template
├── README.md        # This file
└── ARCHITECTURE.md  # Implementation architecture
```

## Configuration Options

| Environment Variable    | Default                            | Description                   |
| ----------------------- | ---------------------------------- | ----------------------------- |
| `MQTT_API_TOKEN`        | (required)                         | Your MQTT API token           |
| `MQTT_BROKER_URL`       | `wss://mqtt.tesaiot.com:8085/mqtt` | WSS broker URL                |
| `MQTT_TOPIC`            | `device/+/telemetry/#`             | Topic to subscribe            |
| `MQTT_CLIENT_ID`        | Auto-generated                     | Unique client identifier      |
| `MQTT_RECONNECT_PERIOD` | `5000`                             | Reconnect interval (ms)       |
| `MQTT_KEEPALIVE`        | `60`                               | Keep-alive interval (seconds) |

## Extending the Example

### Custom Message Handler

```javascript
// In index.js, modify the processMessage function:
function processMessage(deviceId, sensorType, data) {
  // Store in database
  await database.insert({
    device_id: deviceId,
    sensor: sensorType,
    data: data,
    timestamp: new Date()
  });

  // Forward to webhook
  await fetch('https://your-api.com/webhook', {
    method: 'POST',
    body: JSON.stringify({ deviceId, sensorType, data })
  });

  // Trigger alerts
  if (data.value > threshold) {
    sendAlert(deviceId, data);
  }
}
```

### Subscribe to Specific Device

```bash
# In .env
MQTT_TOPIC=device/5a96f40c-1762-4ff3-b570-bdf809e5e695/telemetry/#
```

## Troubleshooting

| Issue                        | Solution                            |
| ---------------------------- | ----------------------------------- |
| `MQTT_API_TOKEN is required` | Set token in `.env` file            |
| `Invalid token format`       | Token must start with `tesa_mqtt_`  |
| `Connection refused`         | Check network and firewall settings |
| `Authentication failed`      | Verify token is not expired         |
| `No messages received`       | Confirm devices are publishing      |

## Dependencies

| Package  | Version | Purpose                            |
| -------- | ------- | ---------------------------------- |
| `mqtt`   | ^5.0.0  | MQTT client with WebSocket support |
| `dotenv` | ^16.0.0 | Environment variable loader        |

## License

Apache 2.0
