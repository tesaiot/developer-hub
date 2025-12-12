# TESA IoT Device → Platform (Server-TLS) — HTTPS or MQTTS via Mongoose

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

Unified, beginner-friendly C example that can send telemetry over either:
- HTTPS Server-TLS (API key + CA)
- MQTTS Server-TLS (username/password + CA)

Uses Cesanta Mongoose single-file library, TLS backends: OpenSSL, mbedTLS, or wolfSSL.

> Licensing: all source helpers in this folder ship with the Apache 2.0 header
> (`Copyright (c) 2025 TESAIoT Platform (TESA)`), making the tutorial safe to
> reuse inside your own lab repos.

---

## Quick Start

- Download Server-TLS bundles in Admin UI → Credentials (include_password / include_api_key if needed)
- `make prepare` to sync into `./certs_credentials`
- Run for 2 minutes (every 10s):
  - HTTPS: `COMM_MODE=HTTPS API_BASE_URL=https://tesaiot.com CERTS_DIR=./certs_credentials ./device_client --period 2 --interval 10`
  - MQTTS: `COMM_MODE=MQTTS MQTT_HOST=mqtt.tesaiot.com MQTT_PORT=8884 CERTS_DIR=./certs_credentials ./device_client --period 2 --interval 10`

## Directory Structure

```text
.
├── scripts/
│   └── sync_credentials.sh
├── config.h
├── data_schema.c
├── data_schema.h
├── main.c                   # selects HTTPS or MQTTS via env COMM_MODE
├── Makefile                 # portable, fetches Mongoose on demand
├── README.md
├── README-TH.md
├── certs_credentials/       # place downloaded bundle here
└── ../common-c/             # shared Mongoose client (auto-downloaded)
```

## Credentials

- Get Device02 Server-TLS bundle from Admin UI → Credentials → "Download Server-TLS bundle".
- Then:

```bash
make prepare
```

## Build

- The Makefile auto-fetches Mongoose into `../common-c/` on first build.
- TLS backends supported: OpenSSL (default), mbedTLS, wolfSSL. Pick via `TLS_BACKEND=...`.
- Install system deps per OS:
  - Debian/Ubuntu (incl. Raspberry Pi):
    - OpenSSL: `sudo apt-get update && sudo apt-get install -y build-essential pkg-config libssl-dev`
    - mbedTLS: `sudo apt-get install -y build-essential pkg-config libmbedtls-dev`
    - wolfSSL: `sudo apt-get install -y build-essential pkg-config libwolfssl-dev`
  - macOS (Homebrew): `brew install openssl@3`
    - If not auto-detected: `export CPPFLAGS=$(brew --prefix openssl@3)/include` and `export LDFLAGS="-L$(brew --prefix openssl@3)/lib"`
- Build:
```bash
# Default: OpenSSL backend
make

# Choose TLS backend
make TLS_BACKEND=mbedtls
make TLS_BACKEND=wolfssl

# Print deps guidance for your OS
make deps
```

## Install Prerequisites

- Linux (Ubuntu/Debian):
  - OpenSSL: `sudo apt-get install -y build-essential libssl-dev`
  - mbedTLS: `sudo apt-get install -y libmbedtls-dev`
  - wolfSSL: `sudo apt-get install -y libwolfssl-dev`
- macOS (Homebrew):
  - OpenSSL: `brew install openssl@3`
  - Optionally export `CPPFLAGS`/`LDFLAGS` if not auto-detected.
- Raspberry Pi OS: same as Ubuntu/Debian instructions.
- RTOS: use this code as reference; integrate per RTOS Mongoose porting guide.

## Run — HTTPS (Server-TLS)

```bash
COMM_MODE=HTTPS \
API_BASE_URL=https://tesaiot.com \
CERTS_DIR=./certs_credentials \
./device_client
```

## Run — MQTTS (Server-TLS)

```bash
COMM_MODE=MQTTS \
MQTT_HOST=mqtt.tesaiot.com MQTT_PORT=8884 \
CERTS_DIR=./certs_credentials \
./device_client
```

## Behavior

- Builds JSON `{device_id, timestamp, data}` by reading field structure from schema file if available
  - Auto-searches at `../download/data_schema.txt` (or set via `SCHEMA_PATH`)
  - Supports types `integer/number/string/boolean` with minimum/maximum and `enum` (picks first value)
  - Falls back to sample fields if no schema file found
- HTTPS uses only `X-API-KEY: <api_key.txt>` (no Bearer token needed)
- TLS verification is enforced. Do not disable SSL. Paths are read from `CERTS_DIR/`.
- Reads `endpoints.json` if present and uses `api_base_url` or `ingest_base_url`; also reads `mqtt_host` and `mqtt_tls_port` if present.
- Device ID is read from `device_id.txt` or overridden by `DEVICE_ID` env var.

## Knowledge

- Schema-driven payload structure: The program reads schema from `../download/data_schema.txt` (or `SCHEMA_PATH`) and generates random values conforming to type/min-max/enum, enabling rapid testing with real data patterns.
- Port selection: serverTLS (user/pass) → MQTT uses 8884, HTTPS uses 443; mTLS is separate at 8883/9444 per policy. Clear path separation reduces confusion.
- Authentication: HTTPS uses only `X-API-KEY`, MQTTS uses `username=device_id` and `password` from bundle (with include_password=true), and backend validates hash in database.
- Topic ACL: Only `device/<device_id>/telemetry` is allowed for publish (subscribe uses `device/<device_id>/commands|config|firmware`).

## Troubleshooting

- HTTPS 401: API key not updated → Download new HTTPS bundle with include_api_key=true then run `./scripts/sync_credentials.sh --device-dir ../download`
- MQTTS Code 5 (Not authorized): username doesn't match device_id or password expired/not reset → Download MQTT Server-TLS bundle with include_password=true then sync again
- Port mixed with mTLS: Going to 8883 will cause TLS to close connection after CONNECT because mode doesn't match → Use 8884 for serverTLS
- ACL deny: Verify topic is `device/<device_id>/telemetry` and client_id matches `<device_id>`

## Legacy Clients (optional)

- See `./c/` for simple libmosquitto/libcurl senders (kept for comparison). The default build uses Mongoose.

## Test Results (2025-09-15) — 2-minute run

- HTTPS Server-TLS: OK. Sent successfully every 10 seconds for 2 minutes (ensure `api_key.txt` is the latest from HTTPS bundle)
- MQTTS Server-TLS (port 8884): OK. Sent successfully every 10 seconds for 2 minutes (after syncing new password from bundle)

## Notes

- To enable MQTTS in Server-TLS mode, ensure `mqtt_username.txt` and `mqtt_password.txt` are valid for this device, and that ACL permits publishing to `device/<device_id>/telemetry` on port 8884.
- TLS verification remained enabled at all times; no SSL weakening.

## Schema Tips

- Place `download/data_schema.txt` in the example folder, or set `SCHEMA_PATH` for the program to read
- If HTTPS 401, download new HTTPS Server-TLS bundle with include_api_key=true then run the sync script again:
  - `./scripts/sync_credentials.sh --device-dir ../download`
