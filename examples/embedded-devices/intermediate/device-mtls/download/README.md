# mTLS Credentials Bundle

This folder should contain your mTLS credentials bundle downloaded from TESAIoT Admin UI.

## Required Files

After downloading your credentials bundle, you should have:

1. **`{device_id}-credentials.json`** - Device credentials including API key
2. **`{device_id}-mqtts-mtls-bundle-{timestamp}/`** - MQTT mTLS bundle folder containing:
   - `mqtt_client_config.h` - C header with MQTT configuration
   - `endpoints.json` - Broker endpoints
   - `README.txt` - Usage instructions
   - `telemetry/` - Telemetry data structure templates

3. **`{device_id}-https-mtls-bundle/`** - HTTPS mTLS bundle folder containing:
   - `endpoints.json` - API endpoints
   - `README.txt` - Usage instructions
   - `PRIVATE_KEY_README.txt` - Important notes about private key

## How to Get Credentials

1. Login to TESAIoT Admin UI (https://admin.tesaiot.com)
2. Navigate to Devices → Your Device
3. Click "Download Credentials Bundle"
4. Select "mTLS" authentication mode
5. Extract the ZIP file to this folder

## Security Warning

⚠️ **NEVER commit real credentials to version control!**
- Keep your credentials local only
- Add credential files to `.gitignore`
- Use `.example` suffix for placeholder files
