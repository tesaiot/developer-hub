# MQTT over QUIC - Quick Start Guide

This guide shows how to test MQTT connectivity using credentials downloaded from the TESA IoT Platform.

## Prerequisites

- Python 3.8+
- Downloaded credentials from Admin UI (Credentials Tab → Download MQTT-QUIC Bundle)

## Quick Start (3 Steps)

### Step 1: Download Credentials

1. Login to Admin UI: https://admin.tesaiot.com
2. Go to **Devices** → Select your device
3. Click **Credentials** tab
4. Click **Download MQTT-QUIC Bundle**
5. Extract to `credential_certifcates/` directory

Your directory structure should look like:
```
mqtt_quic-connectivity/
├── credential_certifcates/
│   ├── ca-chain.pem
│   ├── mqtt-quic-config.json
│   ├── endpoints.json
│   └── README-MQTT-QUIC.txt
├── setup_env.py
└── test_mqtt_quic.py
```

### Step 2: Setup Environment

Run the setup script to create `.env` file from credentials:

```bash
python3 setup_env.py
```

This will:
- Read `mqtt-quic-config.json`
- Extract credentials (username/password)
- Create `.env` file with all settings
- Create `.env.example` (safe to commit)

**Output:**
```
✅ .env file created successfully
📋 Configuration Summary:
   Host: mqtt.tesaiot.com
   Port: 14567 (UDP)
   Transport: QUIC
   Username: 3cb4589c-007a-4bac-866c-d97bf73cef50
   Password: ****************
```

### Step 3: Run Test

```bash
# Create virtual environment (first time only)
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install paho-mqtt python-dotenv

# Run test
python test_mqtt_quic.py
```

**Expected Output:**
```
============================================================
TESA IoT Platform - MQTT over QUIC Test
============================================================

✅ Connected to mqtt.tesaiot.com:14567
   Transport: QUIC
   Device ID: 3cb4589c-007a-4bac-866c-d97bf73cef50
📥 Subscribed to: devices/{device_id}/command

📊 Message 1/5:
   Temperature: 20.0°C
   Humidity: 50%
   Pressure: 1013.0 hPa
📤 Message published (total: 1)

...

✅ Test PASSED
```

## What's Being Tested

1. **Connection**: Connect to MQTT broker with Server-TLS authentication
2. **Authentication**: Username/password verification
3. **TLS**: Verify server certificate using CA chain
4. **Subscribe**: Subscribe to command topic
5. **Publish**: Send 5 telemetry messages
6. **Disconnect**: Graceful disconnection

## Important Notes

### QUIC Transport Limitation

⚠️ **Standard `paho-mqtt` doesn't support QUIC transport!**

The test script uses **MQTTS on TCP port 8884** as fallback to verify credentials work correctly.

For **true QUIC support** (UDP port 14567), you need:
- **NanoSDK** (C/C++) - Recommended
- **QUIC-enabled MQTT client** (custom build)

See [python/README.md](python/README.md) and [c_cpp/README.md](c_cpp/README.md) for language-specific examples.

### Transport Comparison

| Feature | MQTTS (TCP) | MQTT-QUIC (UDP) |
|---------|-------------|-----------------|
| Port | 8884 | 14567 |
| Initial Connection | 2-RTT | **1-RTT** ⚡ |
| Reconnection | 2-RTT | **0-RTT** ⚡ |
| Head-of-line blocking | Yes | **No** ✅ |
| Connection migration | No | **Yes** ✅ |
| Firewall compatibility | High | Medium |

## Configuration Files

### `.env` (Auto-generated)

```bash
# MQTT Broker Settings
MQTT_HOST=mqtt.tesaiot.com
MQTT_PORT=14567
MQTT_TRANSPORT=quic

# Authentication (Server-TLS mode)
MQTT_USERNAME=YOUR_DEVICE_ID
MQTT_PASSWORD=YOUR_DEVICE_PASSWORD

# TLS/SSL Settings
MQTT_TLS_ENABLED=true
MQTT_TLS_VERSION=1.3
MQTT_CA_CERT=./credential_certifcates/ca-chain.pem

# Topics
MQTT_TOPIC_TELEMETRY=devices/{device_id}/telemetry
MQTT_TOPIC_COMMAND=devices/{device_id}/command
MQTT_TOPIC_STATUS=devices/{device_id}/status
```

⚠️ **DO NOT COMMIT `.env` to git** (contains password!)

### `.gitignore` Recommended

```gitignore
# Environment files
.env
venv/

# Credentials (sensitive)
credential_certifcates/*.pem
credential_certifcates/*.json
credential_certifcates/*.txt

# Keep only README
!credential_certifcates/README.md
```

## Troubleshooting

### Connection Timeout

```
❌ Connection timeout
```

**Solutions:**
1. Check firewall allows **UDP port 14567** (QUIC) or **TCP port 8884** (MQTTS)
2. Verify `MQTT_HOST` resolves: `ping mqtt.tesaiot.com`
3. Check credentials are correct in `.env`

### Certificate Error

```
❌ SSL: CERTIFICATE_VERIFY_FAILED
```

**Solutions:**
1. Verify `ca-chain.pem` exists in `credential_certifcates/`
2. Check `MQTT_CA_CERT` path in `.env` is correct
3. Re-download bundle from UI

### Authentication Failed

```
❌ Connection failed with code 4 (Bad username or password)
```

**Solutions:**
1. Reset password in UI → Credentials tab → Reset Password
2. Download new bundle
3. Re-run `setup_env.py`

### Missing Dependencies

```
❌ paho-mqtt not installed
```

**Solution:**
```bash
source venv/bin/activate
pip install paho-mqtt python-dotenv
```

## Next Steps

1. **View telemetry in UI**: Admin UI → Devices → Telemetry tab
2. **Send commands**: Admin UI → Devices → Commands tab
3. **Monitor logs**: Check device connection status
4. **Production deployment**: Embed `ca-chain.pem` in firmware, implement reconnection logic

## Security Best Practices

1. ✅ **Store password securely** - Use encrypted storage on device
2. ✅ **Rotate credentials** - Reset password periodically
3. ✅ **Use TLS 1.3** - Mandatory in QUIC
4. ✅ **Verify server certificate** - Always validate CA chain
5. ✅ **Monitor audit logs** - Check for suspicious activity

## Support

- **Documentation**: https://docs.tesaiot.com/mqtt-quic
- **Issues**: https://github.com/tesaiot/platform/issues
- **Email**: support@tesaiot.com

---

**Generated**: 2025-10-06
**Version**: 1.0.0
