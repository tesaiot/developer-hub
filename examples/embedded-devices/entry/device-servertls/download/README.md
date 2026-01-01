# Server-TLS Credentials Bundle

This folder should contain your Server-TLS credentials bundle downloaded from TESAIoT Admin UI.

## Required Files

After downloading your credentials bundle, you should have folders like:

```
{device_id}-servertls-mqtt-bundle-{timestamp}/
├── mqtt-credentials.txt      # MQTT username/password
├── endpoints.json            # Broker endpoints
└── README.txt                # Usage instructions

{device_id}-servertls-https-bundle-{timestamp}/
├── https-api-credentials.txt  # API key for HTTPS
├── endpoints.json             # API endpoints
└── README.txt                 # Usage instructions
```

## How to Get Credentials

1. Login to TESAIoT Admin UI (https://admin.tesaiot.com)
2. Navigate to Devices → Your Device
3. Click "Download Credentials Bundle"
4. Select "Server-TLS" authentication mode
5. Extract the ZIP file to this folder

## Security Warning

⚠️ **NEVER commit real credentials to version control!**
- Keep your credentials local only
- These files are ignored by .gitignore
- Use the example files in `certs_credentials/` as reference
