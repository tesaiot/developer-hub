# Device Credentials Bundle (mTLS)

This folder should contain your device-specific mTLS credential bundles.

## Expected Structure

After downloading from TESAIoT Admin UI:

```
{device_id}-mqtts-mtls-bundle-{timestamp}/
├── mqtt_client_config.h      # C header with client cert embedded
├── endpoints.json            # Broker endpoints
├── README.txt                # Usage instructions
└── telemetry/
    ├── data_telemetry.c
    └── data_telemetry.h

{device_id}-https-mtls-bundle/
├── endpoints.json            # API endpoints
├── README.txt                # Usage instructions
└── PRIVATE_KEY_README.txt    # Important private key notes
```

## mTLS vs Server-TLS

- **mTLS**: Client presents certificate to server (mutual authentication)
- **Server-TLS**: Only server presents certificate (password authentication)

## Security Warning

⚠️ **NEVER commit real credentials or certificates to version control!**
