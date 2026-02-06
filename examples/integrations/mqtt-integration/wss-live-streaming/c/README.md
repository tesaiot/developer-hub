# TESAIoT WSS Live Streaming - C Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using C with libmosquitto.

## Prerequisites

| Item           | Version | Description                                |
| -------------- | ------- | ------------------------------------------ |
| GCC/Clang      | C11+    | C compiler                                 |
| libmosquitto   | 2.0+    | Eclipse Mosquitto client library           |
| OpenSSL        | 1.1+    | TLS support                                |
| libwebsockets  | 4.0+    | WebSocket support (bundled with mosquitto) |
| MQTT API Token | -       | Generate from TESAIoT Admin UI             |

## Platform Support

| Platform              | Status                  |
| --------------------- | ----------------------- |
| Linux (Ubuntu/Debian) | ✅ Fully supported      |
| Raspberry Pi (ARM64)  | ✅ Fully supported      |
| macOS (Homebrew)      | ✅ Fully supported      |
| Windows               | ⚠️ Requires MinGW/MSYS2 |

## Getting Started

### 1. Install Dependencies

**Linux (Ubuntu/Debian):**

```bash
sudo apt update
sudo apt install -y build-essential libmosquitto-dev libssl-dev
```

**Raspberry Pi:**

```bash
sudo apt update
sudo apt install -y build-essential libmosquitto-dev libssl-dev
```

**macOS (Homebrew):**

```bash
brew install mosquitto openssl
# Add to path if needed:
export CPATH=$(brew --prefix mosquitto)/include
export LIBRARY_PATH=$(brew --prefix mosquitto)/lib
```

### 2. Generate MQTT API Token

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My C App")
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

### 4. Build and Run

```bash
# Build
make

# Run
./wss_mqtt_client
```

Or with direct environment variables:

```bash
MQTT_API_TOKEN=tesa_mqtt_yourorg_xxxx ./wss_mqtt_client
```

You should see:

```
┌─────────────────────────────────────────────────┐
│  TESAIoT WSS Live Streaming - C Client          │
└─────────────────────────────────────────────────┘

Connecting to TESAIoT MQTT Broker via WSS...
  Host: mqtt.tesaiot.com
  Port: 8085
  Client ID: tesaiot-c-1738678800
  Token: tesa_mqtt_yourorg_...xxxx

✅ Connected to TESAIoT MQTT Broker!
📡 Subscribed to: device/+/telemetry/#

Waiting for telemetry messages...
──────────────────────────────────────────────────
```

## Project Structure

```
c/
├── wss_mqtt_client.c  # Main application
├── Makefile           # Build configuration
├── .env.example       # Environment variable template
├── README.md          # This file
└── ARCHITECTURE.md    # Implementation architecture
```

## Configuration Options

| Environment Variable | Default                | Description         |
| -------------------- | ---------------------- | ------------------- |
| `MQTT_API_TOKEN`     | (required)             | Your MQTT API token |
| `MQTT_HOST`          | `mqtt.tesaiot.com`     | MQTT broker host    |
| `MQTT_PORT`          | `8085`                 | WSS port            |
| `MQTT_TOPIC`         | `device/+/telemetry/#` | Topic to subscribe  |

## Build Options

```bash
# Debug build
make DEBUG=1

# Clean build
make clean && make

# Install to system
sudo make install
```

## Extending the Example

### Custom Message Handler

```c
// In wss_mqtt_client.c, modify the on_message callback:
void on_message(struct mosquitto *mosq, void *userdata,
                const struct mosquitto_message *msg) {
    // Parse device ID from topic
    char *device_id = parse_device_id(msg->topic);

    // Parse JSON payload (use cJSON or similar)
    cJSON *json = cJSON_Parse(msg->payload);

    // Store in database
    store_telemetry(device_id, json);

    // Check thresholds
    double value = cJSON_GetNumberValue(
        cJSON_GetObjectItem(json, "value")
    );
    if (value > THRESHOLD) {
        send_alert(device_id, value);
    }

    cJSON_Delete(json);
}
```

## Troubleshooting

| Issue                                | Solution                           |
| ------------------------------------ | ---------------------------------- |
| `MQTT_API_TOKEN is required`         | Set token in `.env` or environment |
| `mosquitto.h not found`              | Install `libmosquitto-dev`         |
| `undefined reference to mosquitto_*` | Link with `-lmosquitto`            |
| `SSL handshake failed`               | Update CA certificates             |
| `Connection refused`                 | Check firewall and network         |

## Dependencies

| Library       | Purpose                            |
| ------------- | ---------------------------------- |
| libmosquitto  | MQTT client with WebSocket support |
| OpenSSL       | TLS/SSL support                    |
| libwebsockets | WebSocket transport (bundled)      |

## Memory Management

This example follows these memory management practices:

- All allocated memory is freed on exit
- Signal handlers ensure clean shutdown
- mosquitto_destroy() cleans up client resources

## License

Apache 2.0
