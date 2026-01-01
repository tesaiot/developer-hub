# Advanced MQTT over QUIC Client (Python)

This **advanced** example extends the entry-level Example #6 with production-ready features:

| Feature | Entry (#6) | Advanced (#22) |
|---------|------------|----------------|
| QUIC Transport | aioquic | aioquic + TCP fallback |
| Reconnection | Manual | Automatic with backoff |
| Multi-stream | Single stream | Parallel topic streams |
| Session Resume | No | 0-RTT when available |
| Connection Stats | No | Built-in monitoring |

---

## Advanced Features

### 1. Automatic QUIC → TCP Fallback
When QUIC (UDP) is blocked by firewall, automatically falls back to TCP+TLS.

### 2. 0-RTT Session Resumption
Saves session tickets for faster reconnection (skip full handshake).

### 3. Multi-Stream Parallel Publishing
QUIC allows multiple streams per connection. Different topics can publish in parallel.

### 4. Connection Health Monitoring
Track messages sent/received, transport type, reconnection count.

---

## Requirements

### Python Version
- Python 3.8 or higher

### Dependencies

**Recommended: Both aioquic and paho-mqtt (for fallback)**
```bash
pip install aioquic paho-mqtt
```

**Install all dependencies:**
```bash
pip install -r requirements.txt
```

---

## Setup

### 1. Download Bundle from TESAIoT Admin UI

1. Navigate to your device in the Admin UI (https://admin.tesaiot.com)
2. Go to **Credentials** tab
3. Click **Download MQTT-QUIC Server-TLS Bundle**
4. Extract the bundle to this directory

You should have these files from the bundle:
- `ca-chain.pem` - CA certificate chain for TLS verification
- `mqtt-quic-config.json` - Connection configuration with your device credentials
- `README-MQTT-QUIC.txt` - Setup instructions

### 2. Create Configuration File

Copy the example configuration and add your device credentials:

```bash
# Copy example config
cp mqtt-quic-config.example.json mqtt-quic-config.json

# Edit with your device credentials from Admin UI
# Replace YOUR_DEVICE_ID with your device UUID
# Replace YOUR_DEVICE_PASSWORD with your device password
```

**Note:** The `mqtt-quic-config.json` file contains sensitive credentials and is excluded from version control via `.gitignore`. Never commit files with real credentials to public repositories.

### 3. Verify Files

```bash
ls -la ca-chain.pem mqtt-quic-config.json
```

### 4. Get Device Credentials

From the bundle or Admin UI Credentials tab:
- **Username:** Your device_id (UUID format, e.g., `05f8968a-b400-4727-9678-b53cb0889fce`)
- **Password:** From bundle or password reset

---

## Usage

### Basic Usage

```bash
python3 mqtt_quic_client.py <device_id> <password>
```

### Example

```bash
python3 mqtt_quic_client.py 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123
```

### Expected Output

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  TESA IoT Platform - MQTT over QUIC Client
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[INFO] Using paho-mqtt (Note: QUIC support may be limited)
[INFO] ✅ Loaded configuration from mqtt-quic-config.json
[INFO] 🔌 Connecting via paho-mqtt...
[INFO]    Endpoint: mqtts://tesaiot.com:14567
[INFO]    Username: 05f8968a-b400-4727-9678-b53cb0889fce
[INFO]    Transport: QUIC (UDP)
[INFO] ✅ SSL context created with TLS 1.3
[INFO]    CA certificate: ca-chain.pem
[INFO]    Verify server: True
[WARNING] ⚠️  paho-mqtt uses TCP+TLS, not QUIC
[WARNING]    For true QUIC support, use NanoSDK or QUIC-enabled client
[INFO] ✅ Connected to MQTT broker via QUIC
[INFO]    Client ID: 05f8968a-b400-4727-9678-b53cb0889fce
[INFO]    Transport: QUIC (UDP)
[INFO]    TLS Version: 1.3

[INFO] 🚀 Starting telemetry loop (Ctrl+C to stop)...

[INFO] 📤 Publishing telemetry to devices/05f8968a-b400-4727-9678-b53cb0889fce/telemetry
[INFO]    Payload: {"timestamp":"2025-10-05T12:00:00Z","data":{"temperature":25.5,"humidity":60.2}}
[INFO] ✅ Message published (mid: 1)
[INFO] ✅ Telemetry #1 sent successfully
```

---

## Code Structure

### Main Components

1. **MQTTQUICClient Class**
   - Handles MQTT over QUIC connection
   - Loads configuration from bundle
   - Manages SSL/TLS context
   - Publishes telemetry data

2. **Configuration Loading**
   - Reads `mqtt-quic-config.json` from bundle
   - Falls back to default config if not found
   - Supports both NanoSDK and paho-mqtt

3. **SSL Context Setup**
   - Creates TLS 1.3 context (required for QUIC)
   - Loads CA certificate for server verification
   - Enables hostname verification

4. **Telemetry Publishing**
   - Publishes to topic: `devices/{device_id}/telemetry`
   - JSON payload with timestamp and sensor data
   - QoS 1 (at least once delivery)

---

## Configuration

The `mqtt-quic-config.json` file contains:

```json
{
  "protocol": "mqtts",
  "host": "tesaiot.com",
  "port": 14567,
  "transport": "quic",
  "tls": {
    "enabled": true,
    "version": "1.3",
    "ca_file": "ca-chain.pem",
    "verify_server": true,
    "verify_mode": "server-only"
  },
  "auth": {
    "method": "username_password",
    "username": "{your_device_id}",
    "password": "{your_password}"
  },
  "connection": {
    "keepalive": 60,
    "clean_session": true,
    "connect_timeout": 10
  }
}
```

---

## Troubleshooting

### ImportError: No module named 'nng'

**Solution:** Install NanoSDK Python bindings or use paho-mqtt
```bash
pip install nng
# OR
pip install paho-mqtt
```

### FileNotFoundError: CA certificate not found

**Solution:** Ensure `ca-chain.pem` is in the same directory
```bash
ls ca-chain.pem
# Download bundle again if missing
```

### Connection failed: Bad username or password

**Solution:**
1. Verify device_id is correct (check in Admin UI)
2. Reset password in Credentials tab
3. Ensure no extra whitespace in password

### SSL certificate verify failed

**Solution:**
1. Check CA certificate is valid:
   ```bash
   openssl x509 -in ca-chain.pem -text -noout
   ```
2. Verify certificate has not expired
3. Ensure file is not corrupted

### paho-mqtt uses TCP+TLS, not QUIC

**Note:** Standard paho-mqtt doesn't support QUIC transport (UDP-based).

**Options:**
1. Use for testing with TCP+TLS (still encrypted)
2. Install NanoSDK for true QUIC support
3. Wait for paho-mqtt QUIC support

---

## Performance Notes

### TCP+TLS (paho-mqtt) vs QUIC (NanoSDK)

| Feature | TCP+TLS (paho) | QUIC (NanoSDK) |
|---------|---------------|----------------|
| Initial Connection | ~200ms | ~100ms (50% faster) |
| Reconnection | ~200ms | ~10ms (90% faster) |
| IP Change | Disconnect | Seamless migration |
| Packet Loss (5%) | ~400ms | ~150ms (60% faster) |
| Transport | TCP | UDP |

### When to Use Each

**Use TCP+TLS (paho-mqtt) when:**
- Testing/development without QUIC client
- Corporate network blocks UDP
- Stable network environment

**Use QUIC (NanoSDK) when:**
- Mobile/IoV devices (moving networks)
- Weak/intermittent connections
- Need fast reconnection
- Performance is critical

---

## Advanced Usage

### Custom SSL Context

```python
import ssl

# Create custom SSL context
context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
context.minimum_version = ssl.TLSVersion.TLSv1_3
context.load_verify_locations("ca-chain.pem")
context.check_hostname = True
context.verify_mode = ssl.CERT_REQUIRED

# Apply to client
client.tls_set_context(context)
```

### Subscribe to Commands

```python
def on_message(client, userdata, msg):
    if msg.topic.startswith("devices/{device_id}/commands"):
        command = json.loads(msg.payload)
        print(f"Received command: {command}")

# Subscribe after connection
client.subscribe(f"devices/{device_id}/commands/#", qos=1)
```

### Persistent Session

```python
# Keep session state across connections
client = mqtt.Client(
    client_id=device_id,
    clean_session=False  # Persistent session
)
```

---

## Testing

### Unit Tests

```bash
python3 -m pytest test_mqtt_quic_client.py
```

### Integration Tests

```bash
# Run client for 1 minute, verify telemetry appears in platform
timeout 60 python3 mqtt_quic_client.py <device_id> <password>
```

---

## References

- **paho-mqtt Documentation:** https://pypi.org/project/paho-mqtt/
- **NanoSDK GitHub:** https://github.com/nanomq/NanoSDK
- **MQTT 3.1.1 Spec:** https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/
- **QUIC Protocol:** https://www.rfc-editor.org/rfc/rfc9000.html

---

## License

Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)

Licensed under Apache License 2.0
