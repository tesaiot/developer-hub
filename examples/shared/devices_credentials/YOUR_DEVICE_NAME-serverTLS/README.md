# Device Credentials Bundle (Server-TLS)

This folder should contain your device-specific credential bundles.

## Expected Structure

After downloading from TESAIoT Admin UI, you should have folders like:

```
{device_id}-servertls-mqtt-bundle-{timestamp}/
├── mqtt-credentials.txt      # MQTT username/password
├── mqtt_client_config.h      # C header with embedded credentials
├── endpoints.json            # Broker endpoints
├── README.txt                # Usage instructions
└── telemetry/
    ├── data_telemetry.c      # Telemetry data structure template
    └── data_telemetry.h      # Header file

{device_id}-servertls-https-bundle-{timestamp}/
├── https-api-credentials.txt  # API key for HTTPS
├── endpoints.json             # API endpoints
├── README.txt                 # Usage instructions
└── telemetry/
    ├── data_telemetry.c
    └── data_telemetry.h

{device_id}-mqtt-quic-server-tls-bundle-{timestamp}/
├── mqtt-quic-config.json      # MQTT-QUIC configuration
├── endpoints.json             # QUIC endpoints
└── README-MQTT-QUIC.txt       # QUIC-specific instructions
```

## How to Get Credentials

1. Login to TESAIoT Admin UI (https://admin.tesaiot.com)
2. Navigate to Devices → Your Device
3. Click "Download Credentials Bundle"
4. Select authentication mode (Server-TLS, mTLS, or MQTT-QUIC)
5. Extract to a folder named: `{your-device-name}-serverTLS/`

## Security Warning

⚠️ **NEVER commit real credentials to version control!**
