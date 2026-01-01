# TESAIoT WSS MQTT Third-Party Integration Examples

This directory contains example applications demonstrating how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) protocol.

## Overview

TESAIoT Platform supports WebSocket Secure (WSS) connections to the MQTT broker, enabling third-party applications to subscribe to device telemetry data in real-time.

### WSS Connection Details

| Property | Value |
|----------|-------|
| Protocol | WSS (WebSocket Secure) |
| Host | `mqtt.tesaiot.com` |
| Port | `8085` |
| Path | `/mqtt` |
| Full URL | `wss://mqtt.tesaiot.com:8085/mqtt` |

### Authentication

Third-party applications authenticate using **MQTT API Tokens** generated from the TESAIoT Admin UI.

**Token Format:** `tesa_mqtt_<org_prefix>_<random_32_chars>`

**Usage:**
- Username: `<token>`
- Password: `<token>` (same value)

## Available Examples

### [Node.js Example](./nodejs/)

JavaScript/TypeScript integration using the `mqtt` package.

```bash
cd nodejs
npm install
cp .env.example .env
# Edit .env with your token
npm start
```

### [Python Example](./python/)

Python integration using the `paho-mqtt` library.

```bash
cd python
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
cp .env.example .env
# Edit .env with your token
python main.py
```

## Getting Started

### Step 1: Generate MQTT API Token

1. Log in to TESAIoT Admin UI (https://admin.tesaiot.com)
2. Navigate to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label for identification
5. Set expiration period (default: 90 days)
6. Click **Generate**
7. **Copy the token immediately** - it will only be shown once!

### Step 2: Configure Your Application

Create a `.env` file with your token:

```env
MQTT_API_TOKEN=tesa_mqtt_yourorg_xxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### Step 3: Run and Test

Run the example and verify you receive telemetry from your devices.

## Topic Structure

| Topic Pattern | Description | Access |
|---------------|-------------|--------|
| `device/+/telemetry/#` | All telemetry from all org devices | Subscribe only |
| `device/<id>/telemetry/#` | All telemetry from specific device | Subscribe only |
| `device/<id>/telemetry/temperature` | Specific sensor type | Subscribe only |
| `device/<id>/commands/*` | Device commands | **Not allowed** |

**Note:** API tokens can only **subscribe** to telemetry topics. Publishing is not permitted.

## Security Considerations

1. **Token Security**
   - Never commit tokens to version control
   - Use environment variables in production
   - Rotate tokens before expiration

2. **Connection Security**
   - Always use WSS (not WS) in production
   - Verify server certificate (don't disable TLS verification)
   - Use unique client IDs per connection

3. **Access Control**
   - Tokens are scoped to organization level
   - Can only access devices in your organization
   - Subscribe-only (no publish permission)

## Troubleshooting

### Cannot Connect

1. Verify token is valid and not expired
2. Check network connectivity to `mqtt.tesaiot.com:8085`
3. Ensure firewall allows outbound WSS connections

### No Messages Received

1. Verify devices are publishing telemetry
2. Check subscription topic pattern matches device topics
3. Confirm devices belong to your organization

### Authentication Failed

1. Token may be expired - generate new token
2. Token may be revoked - check Admin UI
3. Verify username and password are both set to the token value

## Additional Resources

- [SPEC.md](../../TESA_Rules/ALL_PLAN/v2025.12/WSS-based_MQTT_API_Token/SPEC.md) - Feature specification
- [TESAIoT Platform Documentation](https://docs.tesaiot.com)
- [MQTT.js Documentation](https://github.com/mqttjs/MQTT.js)
- [Paho MQTT Python](https://eclipse.dev/paho/index.php?page=clients/python/index.php)

## License

MIT - See LICENSE file for details.
