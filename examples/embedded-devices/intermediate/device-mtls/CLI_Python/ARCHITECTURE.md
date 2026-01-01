# MQTTS mTLS CLI (Bash/Python) - Architecture

## Overview

Bash CLI tool for publishing telemetry to TESAIoT Platform over MQTTS using Mutual TLS (mTLS) authentication. Uses `mosquitto_pub` for MQTT publishing with client certificate authentication and Python for JSON payload generation.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                      CLI Tool (Bash + Python)                   │
├─────────────────────────────────────────────────────────────────┤
│  publish_mqtt_sample.sh                                         │
│  ├── Credential Loading     → Read bundle files                 │
│  ├── Endpoint Resolution    → Parse endpoints.json              │
│  ├── Python Payload Gen     → JSON telemetry generation         │
│  └── mosquitto_pub          → MQTTS publish with mTLS           │
│                                                                 │
│  .gen_payload.py (temporary)                                    │
│  └── Medical sensor data generator (inline Python)              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ MQTTS (TLS 1.2/1.3) Port 8883
                              │ Client Certificate Authentication
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform (EMQX)                      │
├─────────────────────────────────────────────────────────────────┤
│  MQTT Broker: mqtt.tesaiot.com:8883                             │
│  ├── TLS Server Certificate (verified by ca-chain.pem)          │
│  ├── Client Certificate Validation (client_cert.pem)            │
│  └── Topic: device/{device_id}/telemetry                        │
└─────────────────────────────────────────────────────────────────┘
```

## mTLS Authentication Flow

```ini
┌────────────────────────────────────────────────────────────────┐
│                    Mutual TLS Authentication                   │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────────┐                      ┌─────────────────────┐  │
│  │ CLI Client  │                      │   TESAIoT Broker    │  │
│  └──────┬──────┘                      └──────────┬──────────┘  │
│         │                                        │             │
│         │  1. TLS ClientHello                    │             │
│         │───────────────────────────────────────►│             │
│         │                                        │             │
│         │  2. ServerHello + Server Certificate   │             │
│         │     + CertificateRequest               │             │
│         │◄───────────────────────────────────────│             │
│         │                                        │             │
│         │  3. Verify Server Cert (ca-chain.pem)  │             │
│         │                                        │             │
│         │  4. Client Certificate + Key           │             │
│         │     (client_cert.pem + client_key.pem) │             │
│         │───────────────────────────────────────►│             │
│         │                                        │             │
│         │  5. Verify Client Cert (Platform CA)   │             │
│         │◄───────────────────────────────────────│             │
│         │                                        │             │
│         │  6. TLS Handshake Complete             │             │
│         │     MQTT PUBLISH (QoS 1)               │             │
│         │───────────────────────────────────────►│             │
│                                                                │
│  Note: BOTH client and server authenticate with certificates   │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Server-TLS vs mTLS Comparison

```ini
┌────────────────────────────────────────────────────────────────┐
│                    Authentication Comparison                   │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  Server-TLS (HTTPS API)        │  mTLS (MQTTS)                 │
│  ───────────────────────────── │ ────────────────────────────  │
│  Server presents certificate   │  Server presents certificate  │
│  Client verifies server        │  Client verifies server       │
│  Client uses API KEY (header)  │  Client presents certificate  │
│  No client certificate         │  Server verifies client       │
│  Protocol: HTTPS               │  Protocol: MQTTS              │
│  Port: 443                     │  Port: 8883                   │
│  Use case: REST API access     │  Use case: Real-time MQTT     │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Credential Bundle Structure

```ini
certs_credentials/
├── device_id.txt         # Device UUID
├── client_cert.pem       # Client certificate (for mTLS)
├── client_key.pem        # Client private key (chmod 600)
├── ca-chain.pem          # CA certificate chain
└── endpoints.json        # Platform endpoints
    {
      "mqtt_host": "mqtt.tesaiot.com",
      "mqtt_mtls_port": 8883
    }
```

## Telemetry Payload Format

```json
{
  "device_id": "05f8968a-b400-4727-9678-b53cb0889fce",
  "timestamp": "2025-12-09T10:30:45+00:00",
  "data": {
    "spo2": 97,
    "spo2_unit": "%",
    "temperature": 36.85,
    "temperature_unit": "°C",
    "heart_rate": 78,
    "heart_rate_bpm": 78,
    "perfusion_index": 4.23,
    "motion": false,
    "signal_quality": 95,
    "rhythm": "sinus",
    "rr_interval_ms": 780.5,
    "qt_interval_ms": 375.2,
    "site": "temporal"
  }
}
```

## Script Execution Flow

```ini
┌──────────┐     ┌───────────────┐     ┌─────────────────┐
│   CLI    │────►│ Validate      │────►│ Load Bundle     │
│  args    │     │ Dependencies  │     │ Credentials     │
└──────────┘     │ mosquitto_pub │     └───────┬─────────┘
                 │ python3       │             │
                 └───────────────┘             │
                                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Parse endpoints.json                         │
│  ├── mqtt_host: mqtt.tesaiot.com                                │
│  └── mqtt_mtls_port: 8883                                       │
└─────────────────────────────────────────────────────────────────┘
                                               │
                                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Generate Payload (Python)                    │
│  DEVICE_ID=<uuid> python3 .gen_payload.py                       │
│  Output: {"device_id":"...","timestamp":"...","data":{...}}     │
└─────────────────────────────────────────────────────────────────┘
                                               │
                                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    mosquitto_pub Command                        │
│  mosquitto_pub \                                                │
│    --cafile ca-chain.pem \          # Server cert verification  │
│    --cert client_cert.pem \         # Client certificate        │
│    --key client_key.pem \           # Client private key        │
│    --host mqtt.tesaiot.com \        # Broker hostname           │
│    --port 8883 \                    # mTLS port                 │
│    --id <device_id> \               # MQTT client ID            │
│    --tls-version tlsv1.2 \          # TLS version               │
│    --qos 1 \                        # At-least-once delivery    │
│    --topic device/<id>/telemetry \  # Publish topic             │
│    --message '{"...":"..."}'        # JSON payload              │
└─────────────────────────────────────────────────────────────────┘
```

## CLI Options

| Flag | Description | Default |
|------|-------------|---------|
| `--certs-dir` | Path to credential bundle | `../certs_credentials` |
| `--broker-host` | Override MQTT broker host | From `endpoints.json` |
| `--broker-port` | Override MQTT broker port | From `endpoints.json` |
| `--device-id` | Override device ID | From `device_id.txt` |
| `--tls-version` | TLS version (`tlsv1.2` or `tlsv1.3`) | `tlsv1.2` |
| `--count` | Number of payloads to send | 1 |
| `--interval` | Seconds between sends | 5 |
| `--period` | Duration in minutes | 0 (disabled) |
| `--dry-run` | Print payload without sending | false |

## TLS Configuration

```bash
# mosquitto_pub mTLS options
mosquitto_pub \
    --cafile "$CA_FILE" \        # Trust store for server verification
    --cert "$CLIENT_CERT" \      # Client certificate (X.509)
    --key "$CLIENT_KEY" \        # Client private key (must be 600)
    --tls-version "$TLS_VERSION" # tlsv1.2 or tlsv1.3
```

## Files

```ini
CLI_Python/
├── publish_mqtt_sample.sh   # Main CLI tool (Bash)
├── README.md                # English/Thai documentation
├── README-TH.md             # Thai-only documentation
└── ARCHITECTURE.md          # This file

../certs_credentials/
├── device_id.txt            # Device UUID
├── client_cert.pem          # Client certificate (mTLS)
├── client_key.pem           # Client private key (chmod 600)
├── ca-chain.pem             # CA certificate chain
└── endpoints.json           # Broker endpoints
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | ✅ Supported | `brew install mosquitto` |
| Linux (Ubuntu/Debian) | ✅ Supported | `apt install mosquitto-clients` |
| Raspberry Pi | ✅ Supported | `apt install mosquitto-clients` |
| Windows | ⚠️ Partial | Use WSL or Git Bash |

## Dependencies

```bash
# Required commands
mosquitto_pub  # MQTT CLI client
python3        # Payload generation (standard library only)
```

## Error Handling

| Error | Meaning | Solution |
|-------|---------|----------|
| `Connection refused` | Broker unreachable | Check host/port |
| `Certificate verify failed` | CA cert issue | Verify ca-chain.pem |
| `Bad username or password` | mTLS handshake failed | Check client cert/key |
| `File not readable` | Permission issue | `chmod 600 client_key.pem` |
| `mosquitto_pub not found` | Missing dependency | Install mosquitto-clients |

## Security Notes

- Client private key requires `chmod 600` permissions
- Both client and server must present valid certificates
- Certificate chain must be complete (root + intermediate)
- TLS 1.3 recommended for enhanced security
- Device ID embedded in certificate CN (Common Name)
