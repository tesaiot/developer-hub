# Raspberry Pi → TESAIoT Platform (Server TLS)

Python MQTT client for Raspberry Pi using Server TLS authentication (NCSA Level 1).

**Security Level:** NCSA Level 1 (TLS + API Key/Password)

---

## Overview

This example demonstrates how to connect a Raspberry Pi to TESAIoT Platform using:
- **MQTTS** (MQTT over TLS 1.2+) with username/password authentication
- **HTTPS** with API Key authentication

Perfect for beginners who want to quickly prototype IoT solutions.

## Prerequisites

### Hardware
- Raspberry Pi (3/4/5 or Zero 2 W)
- Internet connection (WiFi or Ethernet)

### Software
- Raspberry Pi OS (64-bit recommended)
- Python 3.9+

### TESAIoT Platform
- Active TESAIoT account
- Device registered on TESAIoT Platform
- Server TLS credentials bundle downloaded

## Quick Start

### 1. Install Dependencies

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Python dependencies
pip3 install -r requirements.txt
```

### 2. Download Credentials from TESAIoT Platform

1. Login to **TESAIoT Admin Portal**: https://admin.tesaiot.com
2. Navigate to **Devices** → Select your device → **Credentials** tab
3. Click **Download Bundle** and select:
   - **Bundle Type**: `Server TLS` (port 8884)
   - **Include Password**: `Yes` (for MQTTS authentication)
   - **Include API Key**: `Yes` (for HTTPS mode)
4. Extract the downloaded ZIP file to `./certs/` folder

**Alternatively**, store credentials in the shared folder:
```bash
# Store in shared credentials folder
cp -r <downloaded-bundle>/* ../../shared/devices_credentials/<device-name>/
```

> **Note**: The `examples/shared/devices_credentials/` folder is for storing downloaded credentials. This folder is NOT committed to git repositories.

### 3. Configure

```bash
# Copy example config
cp .env.example .env

# Edit configuration
nano .env
```

Required settings:
```bash
DEVICE_ID=your-device-id
MQTT_HOST=mqtt.tesaiot.com
MQTT_PORT=8884
MQTT_USERNAME=your-device-id
MQTT_PASSWORD=your-password
CA_CERT_PATH=./certs/ca.pem
```

### 4. Run

```bash
# MQTTS mode (default)
python3 main.py

# HTTPS mode
python3 main.py --mode https

# Custom interval (seconds)
python3 main.py --interval 30
```

## Directory Structure

```
rpi-servertls/
├── main.py              # Main application
├── mqtt_client.py       # MQTT client wrapper
├── https_client.py      # HTTPS client wrapper
├── sensor_simulator.py  # Simulated sensor data
├── requirements.txt     # Python dependencies
├── .env.example         # Configuration template
├── certs/               # TLS certificates
│   └── ca.pem           # CA certificate
└── README.md            # This file
```

## Configuration Options

| Environment Variable | Description | Default |
|---------------------|-------------|---------|
| `DEVICE_ID` | Device identifier | Required |
| `MQTT_HOST` | MQTT broker hostname | `mqtt.tesaiot.com` |
| `MQTT_PORT` | MQTT TLS port | `8884` |
| `MQTT_USERNAME` | MQTT username (device_id) | Required |
| `MQTT_PASSWORD` | MQTT password | Required |
| `CA_CERT_PATH` | Path to CA certificate | `./certs/ca.pem` |
| `API_BASE_URL` | HTTPS API endpoint | `https://api.tesaiot.com` |
| `API_KEY` | API Key for HTTPS | Required for HTTPS mode |

## Telemetry Format

The client sends JSON telemetry in TESAIoT format:

```json
{
  "device_id": "rpi-sensor-001",
  "timestamp": "2025-12-27T21:00:00Z",
  "data": {
    "temperature": 25.5,
    "humidity": 65.2,
    "pressure": 1013.25
  }
}
```

## MQTT Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `device/{device_id}/telemetry` | Publish | Send sensor data |
| `device/{device_id}/commands` | Subscribe | Receive commands |
| `device/{device_id}/config` | Subscribe | Receive configuration |

## Hardware Sensors (Optional)

To use real sensors instead of simulated data:

```bash
# Install sensor libraries
pip3 install adafruit-circuitpython-dht
pip3 install adafruit-circuitpython-bmp280

# Edit sensor_simulator.py to read from hardware
```

## Troubleshooting

### MQTT Connection Refused (Code 5)
- Verify username matches device_id
- Check password is from Server TLS bundle with `include_password=true`
- Ensure port 8884 (not 8883 for mTLS)

### Certificate Verification Failed
- Download fresh CA certificate from TESAIoT
- Check CA_CERT_PATH points to correct file

### HTTPS 401 Unauthorized
- Download Server TLS bundle with `include_api_key=true`
- Verify API_KEY in .env file

## Security Notes

- **Always use TLS** - Never disable certificate verification
- **Protect credentials** - Keep .env and certs/ private
- **Rotate passwords** - Regenerate credentials periodically
- **NCSA Level 1** - This is baseline security; consider mTLS for production

## Next Steps

After mastering Server TLS, advance to:
- [rpi-mtls](../intermediate/rpi-mtls/) - Mutual TLS with client certificates
- [rpi-mtls-trustm](../advanced/rpi-mtls-trustm/) - Hardware security with OPTIGA Trust M

---

**NCSA Compliance:** Level 1 (Security Baseline)
**Last Updated:** 2025-12-27
