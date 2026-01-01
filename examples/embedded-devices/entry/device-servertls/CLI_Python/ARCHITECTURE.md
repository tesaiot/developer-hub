# HTTPS Server-TLS CLI Python - Architecture

## Overview

Python CLI tool for sending telemetry to TESAIoT Platform over HTTPS using Server-TLS authentication. Uses the standard library (no external dependencies) for HTTP requests with X-API-KEY header authentication and CA certificate verification.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      Python CLI Tool                            │
├─────────────────────────────────────────────────────────────────┤
│  post_https_sample.py                                           │
│  ├── parse_args()              → CLI argument parsing           │
│  ├── load_text()               → Read credential files          │
│  ├── resolve_base_url()        → Get ingest URL from bundle     │
│  ├── generate_payload()        → Medical sensor telemetry       │
│  ├── build_ssl_context()       → TLS with CA verification       │
│  └── post_payload()            → HTTPS POST to /api/v1/telemetry│
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS (TLS 1.2+) Port 443
                              │ X-API-KEY Header Authentication
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform                              │
├─────────────────────────────────────────────────────────────────┤
│  Ingest Endpoint: https://tesaiot.com/api/v1/telemetry          │
│  ├── TLS Server Certificate (verified by ca-chain.pem)         │
│  ├── X-API-KEY validation (from api_key.txt)                    │
│  └── Device ID extraction (from device_id.txt)                  │
└─────────────────────────────────────────────────────────────────┘
```

## Authentication Flow

```
┌────────────────────────────────────────────────────────────────┐
│                    Server-TLS Authentication                    │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐                      ┌─────────────────────┐  │
│  │ CLI Client  │                      │   TESAIoT Server    │  │
│  └──────┬──────┘                      └──────────┬──────────┘  │
│         │                                        │              │
│         │  1. HTTPS Request (TLS 1.2+)          │              │
│         │───────────────────────────────────────►│              │
│         │                                        │              │
│         │  2. Server Certificate                 │              │
│         │◄───────────────────────────────────────│              │
│         │                                        │              │
│         │  3. Verify against ca-chain.pem       │              │
│         │  (Client validates server identity)    │              │
│         │                                        │              │
│         │  4. POST /api/v1/telemetry             │              │
│         │     Header: X-API-KEY: <api_key>       │              │
│         │     Body: JSON telemetry payload       │              │
│         │───────────────────────────────────────►│              │
│         │                                        │              │
│         │  5. HTTP 200 OK                        │              │
│         │◄───────────────────────────────────────│              │
│                                                                 │
│  Note: No client certificate - API key authentication only     │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

## Credential Bundle Structure

```
certs_credentials/
├── device_id.txt       # Device UUID (e.g., 05f8968a-b400-4727-9678-b53cb0889fce)
├── api_key.txt         # REST API Key (tesa_ak_xxx...)
├── ca-chain.pem        # CA certificate chain for server verification
└── endpoints.json      # Platform endpoints configuration
    {
      "ingest_base_url": "https://tesaiot.com",
      "api_base_url": "https://tesaiot.com"
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

## Request Flow

```
┌──────────┐     ┌───────────────┐     ┌─────────────────┐
│   CLI    │────►│ Load Bundle   │────►│ Build SSL       │
│  args    │     │ credentials   │     │ Context         │
└──────────┘     └───────────────┘     └───────┬─────────┘
                                               │
                 ┌─────────────────────────────┘
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Generate Telemetry                            │
│  generate_payload(device_id)                                    │
│  ├── Random medical sensor values (SpO2, temp, HR, etc.)       │
│  ├── ISO 8601 timestamp (UTC)                                   │
│  └── JSON serialization                                         │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    HTTP POST Request                             │
│  urllib.request.Request(                                        │
│      url="https://tesaiot.com/api/v1/telemetry",               │
│      data=json_payload,                                         │
│      headers={                                                  │
│          "Content-Type": "application/json",                    │
│          "X-API-KEY": "<api_key_from_bundle>"                   │
│      }                                                          │
│  )                                                              │
└─────────────────────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Response Handling                             │
│  ├── HTTP 200 OK → Success, print response                     │
│  ├── HTTP 4xx → Client error (bad auth, invalid payload)       │
│  └── HTTP 5xx → Server error (retry later)                     │
└─────────────────────────────────────────────────────────────────┘
```

## CLI Options

| Flag | Description | Default |
|------|-------------|---------|
| `--certs-dir` | Path to credential bundle | `../certs_credentials` |
| `--base-url` | Override ingest base URL | From `endpoints.json` |
| `--endpoint` | API endpoint path | `/api/v1/telemetry` |
| `--device-id` | Override device ID | From `device_id.txt` |
| `--timeout` | HTTP timeout (seconds) | 10 |
| `--interval` | Seconds between sends | 5 |
| `--count` | Number of payloads to send | 1 |
| `--period` | Duration in minutes | 0 (disabled) |
| `--dry-run` | Print payload without sending | false |

## SSL Context Configuration

```python
# Server-TLS verification (no client certificate)
ctx = ssl.create_default_context(purpose=ssl.Purpose.SERVER_AUTH)
ctx.check_hostname = True
ctx.load_verify_locations(cafile="ca-chain.pem")
```

## Files

```
CLI_Python/
├── post_https_sample.py   # Main CLI tool
├── README.md              # English/Thai documentation
├── README-TH.md           # Thai-only documentation
└── ARCHITECTURE.md        # This file

../certs_credentials/
├── device_id.txt          # Device UUID
├── api_key.txt            # REST API key
├── ca-chain.pem           # CA certificate chain
└── endpoints.json         # Platform endpoints
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | ✅ Supported | Python 3.8+ |
| Linux (Ubuntu/Debian) | ✅ Supported | Python 3.8+ |
| Raspberry Pi | ✅ Supported | Python 3.8+ |
| Windows | ✅ Supported | Python 3.8+ |

## No External Dependencies

```python
# Uses only Python standard library
import argparse
import datetime
import json
import ssl
import urllib.request
import urllib.parse
import urllib.error
```

## Error Handling

| Error | Meaning | Solution |
|-------|---------|----------|
| `HTTP 401` | Invalid API key | Check api_key.txt |
| `HTTP 403` | Unauthorized | Verify device belongs to org |
| `SSL: CERTIFICATE_VERIFY_FAILED` | CA cert issue | Verify ca-chain.pem |
| `FileNotFoundError` | Missing credential | Download bundle from platform |
| `TIMEOUT` | Network issue | Check connectivity |

## Security Notes

- API key is passed via `X-API-KEY` header (not URL parameter)
- TLS server certificate verified against trusted CA chain
- Credentials stored in separate files with restricted permissions (0600)
- No client certificate required (Server-TLS mode)
