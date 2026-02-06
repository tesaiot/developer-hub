# TESAIoT WSS Live Streaming - Node-RED Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Node-RED.

## Prerequisites

| Item           | Version | Description                    |
| -------------- | ------- | ------------------------------ |
| Node.js        | 18+     | Required for Node-RED          |
| Docker         | 20+     | (Optional) Container runtime   |
| MQTT API Token | -       | Generate from TESAIoT Admin UI |

## Platform Support

| Platform            | Method        | Status         |
| ------------------- | ------------- | -------------- |
| All platforms       | Docker        | ✅ Recommended |
| Linux/macOS/Windows | npm global    | ✅ Supported   |
| Raspberry Pi        | Docker or npm | ✅ Supported   |

## Getting Started

### Option 1: Docker (Recommended)

```bash
# Copy environment config
cp .env.example .env

# Edit .env with your MQTT API token
nano .env

# Start Node-RED
docker-compose up -d

# Open browser
open http://localhost:1880
```

### Option 2: Local Installation

```bash
# Install Node-RED globally
npm install -g node-red

# Start Node-RED
node-red

# Open browser and import the flow
open http://localhost:1880
```

## Generate MQTT API Token

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Node-RED App")
5. Set expiration (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

## Import the Flow

1. Open Node-RED at http://localhost:1880
2. Click the hamburger menu (☰) → **Import**
3. Select **Clipboard**
4. Paste the contents of `flows/wss-streaming-flow.json`
5. Click **Import**
6. Click **Deploy**

## Configure the MQTT Node

1. Double-click the **mqtt in** node
2. Click the pencil icon next to **Server** to edit
3. Set the following:
   - **Server**: `mqtt.tesaiot.com`
   - **Port**: `8085`
   - **Connect automatically**: ✅
   - **Use TLS**: ✅
   - **Protocol**: `MQTT V3.1.1`
4. Go to **Security** tab:
   - **Username**: Your MQTT API token
   - **Password**: Your MQTT API token (same value)
5. Click **Update** then **Done**
6. Click **Deploy**

## Flow Description

The example flow includes:

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  MQTT In        │───▶│  JSON Parse     │───▶│  Debug Output   │
│  (WSS Subscribe)│    │  (Function)     │    │  (Console)      │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌─────────────────┐
                       │  Dashboard      │
                       │  (Chart/Gauge)  │
                       └─────────────────┘
```

### Nodes

| Node     | Purpose                                     |
| -------- | ------------------------------------------- |
| mqtt in  | Subscribe to `device/+/telemetry/#` via WSS |
| json     | Parse incoming JSON payload                 |
| function | Extract device ID and sensor data           |
| debug    | Output to debug sidebar                     |
| chart    | Visualize time-series data                  |
| gauge    | Show current values                         |

## Project Structure

```
nodered/
├── flows/
│   └── wss-streaming-flow.json  # Pre-built flow
├── docker-compose.yml           # Docker configuration
├── .env.example                 # Environment template
├── README.md                    # This file
└── ARCHITECTURE.md              # Implementation architecture
```

## Dashboard

If using Node-RED Dashboard (node-red-dashboard), access it at:

```
http://localhost:1880/ui
```

## Troubleshooting

| Issue             | Solution                                      |
| ----------------- | --------------------------------------------- |
| Connection failed | Verify token in MQTT node security settings   |
| No messages       | Check topic subscription matches your devices |
| TLS error         | Ensure "Use TLS" is enabled                   |
| Port issue        | Use port 8085 for WSS                         |

## Environment Variables

When using Docker, these environment variables are available:

| Variable         | Default    | Description         |
| ---------------- | ---------- | ------------------- |
| `MQTT_API_TOKEN` | (required) | Your MQTT API token |
| `NODE_RED_PORT`  | `1880`     | Node-RED HTTP port  |

## Docker Commands

```bash
# Start in background
docker-compose up -d

# View logs
docker-compose logs -f

# Stop
docker-compose down

# Rebuild after changes
docker-compose up -d --build
```

## Extending the Flow

### Add Database Storage

1. Install `node-red-node-mongodb` or `node-red-contrib-postgres`
2. Add a database node after the JSON parse
3. Configure connection and insert telemetry

### Add Alerts

1. Add a **switch** node after JSON parse
2. Configure thresholds (e.g., temperature > 30)
3. Add email/Telegram/webhook node for notifications

### Add Dashboard Charts

1. Install `node-red-dashboard`
2. Add **chart** or **gauge** nodes
3. Configure groups and tabs

## License

Apache 2.0
