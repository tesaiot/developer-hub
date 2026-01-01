# Shared Device Credentials

This folder stores device credentials downloaded from the TESAIoT Platform.

## Important Security Notice

**This folder is NOT committed to git repositories.**

- All `.pem`, `.txt`, and credential files are excluded via `.gitignore`
- Do NOT share credentials publicly
- Keep credentials secure and rotate them regularly

## Folder Structure

Organize credentials by device name:

```
devices_credentials/
├── README.md                    # This file
├── .gitkeep                     # Keeps folder in git (empty)
├── BDH-Device01-serverTLS/      # Device credentials folder
│   ├── *-servertls-mqtt-bundle-*/
│   │   ├── ca-chain.pem
│   │   ├── mqtt-credentials.txt
│   │   └── endpoints.json
│   └── *-servertls-https-bundle-*/
│       ├── ca-chain.pem
│       ├── https-api-credentials.txt
│       └── endpoints.json
├── PSoC-Edge01-mTLS/            # mTLS device credentials
│   ├── *-mqtts-mtls-bundle-*/
│   │   ├── ca-chain.pem
│   │   ├── <device-id>.pem      # Client certificate
│   │   └── endpoints.json
│   └── *-https-mtls-bundle-*/
│       ├── ca-chain.pem
│       └── <device-id>-certificate.pem
└── ...
```

## How to Download Credentials

1. Login to **TESAIoT Admin Portal**: https://admin.tesaiot.com
2. Navigate to **Devices** > Select your device
3. Go to the **Credentials** tab
4. Choose bundle type:
   - **Server-TLS MQTT Bundle** - For MQTT with username/password auth
   - **Server-TLS HTTPS Bundle** - For HTTPS with API key auth
   - **mTLS MQTT Bundle** - For MQTT with client certificate auth
   - **mTLS HTTPS Bundle** - For HTTPS with client certificate auth
   - **MQTT-QUIC Bundle** - For MQTT over QUIC protocol
5. Click **Download** and extract to this folder

## Bundle Types

| Bundle Type | Contents | Use Case |
|-------------|----------|----------|
| Server-TLS MQTT | CA chain, credentials | MQTT port 8884 |
| Server-TLS HTTPS | CA chain, API key | HTTPS port 443 |
| mTLS MQTT | CA chain, client cert | MQTT port 8883 |
| mTLS HTTPS | CA chain, client cert | HTTPS port 9444 |
| MQTT-QUIC | CA chain, config | QUIC port 14567 |

## Usage in Examples

Each example can reference credentials from this folder:

```bash
# From rpi-servertls example
CERTS_DIR=../../shared/devices_credentials/BDH-Device01-serverTLS/...

# Or copy to example's local certs folder
cp ../../shared/devices_credentials/BDH-Device01-serverTLS/*/* ./certs_credentials/
```

## Security Best Practices

1. **Never commit credentials** - Verify `.gitignore` is working
2. **Use dedicated test devices** - Don't use production device credentials
3. **Rotate credentials regularly** - Regenerate from Admin Portal
4. **Secure your machine** - These are device secrets
5. **Delete after testing** - Remove credentials when done

## License

Apache 2.0 - See parent directory LICENSE file
