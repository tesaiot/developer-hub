# TESAIoT WSS Live Streaming - Rust Example

This example demonstrates how to connect to the TESAIoT MQTT broker via WebSocket Secure (WSS) and subscribe to device telemetry data using Rust.

## Prerequisites

| Item           | Version | Description                    |
| -------------- | ------- | ------------------------------ |
| Rust           | 1.70+   | Rust toolchain                 |
| Cargo          | 1.70+   | Package manager                |
| MQTT API Token | -       | Generate from TESAIoT Admin UI |

## Platform Support

| Platform                    | Status             |
| --------------------------- | ------------------ |
| Linux (x86_64)              | ✅ Fully supported |
| macOS (Intel/Apple Silicon) | ✅ Fully supported |
| Windows (x86_64)            | ✅ Fully supported |
| Raspberry Pi (ARM64)        | ✅ Fully supported |

## Getting Started

### 1. Install Rust (if not installed)

**All Platforms:**

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env
```

**Windows:**
Download and run [rustup-init.exe](https://rustup.rs/)

### 2. Generate MQTT API Token

1. Log in to [TESAIoT Admin UI](https://admin.tesaiot.com)
2. Go to **Organization Settings** > **MQTT API Tokens**
3. Click **Generate New Token**
4. Enter a label (e.g., "My Rust App")
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

### 4. Build and Run

```bash
# Build release binary
cargo build --release

# Run
cargo run --release
```

You should see:

```
┌─────────────────────────────────────────────────┐
│  TESAIoT WSS Live Streaming - Rust Client       │
└─────────────────────────────────────────────────┘

Connecting to TESAIoT MQTT Broker via WSS...
  Broker: wss://mqtt.tesaiot.com:8085/mqtt
  Client ID: tesaiot-rust-1738678800
  Token: tesa_mqtt_yourorg_...xxxx

✅ Connected to TESAIoT MQTT Broker!
📡 Subscribed to: device/+/telemetry/#

Waiting for telemetry messages...
──────────────────────────────────────────────────
```

When devices publish telemetry:

```
[2026-02-04T10:30:45.123Z] device/abc123/telemetry/temperature
  Device: abc123
  Sensor: temperature
  Data: {"value": 25.5, "unit": "°C"}
```

## Project Structure

```
rust/
├── Cargo.toml         # Dependencies and project config
├── src/
│   └── main.rs        # Main application
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

```rust
// In main.rs, modify the process_message function:
fn process_message(device_id: &str, sensor_type: &str, data: &Value) {
    // Store in database
    database.insert(TelemetryRecord {
        device_id: device_id.to_string(),
        sensor: sensor_type.to_string(),
        data: data.clone(),
        timestamp: Utc::now(),
    });

    // Check thresholds
    if let Some(value) = data.get("value").and_then(|v| v.as_f64()) {
        if value > THRESHOLD {
            send_alert(device_id, value);
        }
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
| `TLS handshake failed`       | Update system CA certificates       |
| `No messages received`       | Confirm devices are publishing      |

## Dependencies

| Crate        | Version | Purpose                     |
| ------------ | ------- | --------------------------- |
| `rumqttc`    | 0.24    | Async MQTT client           |
| `tokio`      | 1.0     | Async runtime               |
| `dotenv`     | 0.15    | Environment variable loader |
| `serde_json` | 1.0     | JSON parsing                |
| `chrono`     | 0.4     | Timestamp handling          |

## Building for Release

```bash
# Optimized release build
cargo build --release

# Binary location
./target/release/tesaiot-wss-streaming
```

## Cross-Compilation

```bash
# For Raspberry Pi (ARM64)
rustup target add aarch64-unknown-linux-gnu
cargo build --release --target aarch64-unknown-linux-gnu
```

## License

Apache 2.0
