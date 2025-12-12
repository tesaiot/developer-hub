# TESA IoT Device → Platform (mTLS) — HTTPS or MQTTS via Mongoose

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

Unified, beginner-friendly C example that can send telemetry over either:
- HTTPS mTLS (client certificate + private key)
- MQTTS mTLS (client certificate + private key)

Uses Cesanta Mongoose single-file library, TLS backends: OpenSSL, mbedTLS, or wolfSSL.

> Licensing: every C/Python/shell source in this tutorial carries the Apache 2.0
> header (`Copyright (c) 2025 TESAIoT Platform (TESA)`), so feel free to adapt it
> for your lab projects.

---

## Quick Start

- Download mTLS bundles in Admin UI → Credentials
- `make prepare` to sync credentials into `./certs_credentials`
- Run for 2 minutes (every 10s):
  - HTTPS: `COMM_MODE=HTTPS API_BASE_URL=https://tesaiot.com:9444 CERTS_DIR=./certs_credentials ./device_client --period 2 --interval 10`
  - MQTTS: `COMM_MODE=MQTTS MQTT_HOST=mqtt.tesaiot.com MQTT_PORT=8883 CERTS_DIR=./certs_credentials ./device_client --period 2 --interval 10`

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

- Get Device01 mTLS bundle from Admin UI → Credentials → "Download mTLS bundle".
- Then:

```bash
make prepare
```

## Build

- The Makefile auto-fetches Mongoose into `../common-c/` on first build.
- TLS backends supported: OpenSSL (default), mbedTLS, wolfSSL. Pick with `TLS_BACKEND=...`.
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

## Run — HTTPS (mTLS)

```bash
COMM_MODE=HTTPS \
API_BASE_URL=https://tesaiot.com:9444 \
CERTS_DIR=./certs_credentials \
./device_client
```

## Run — MQTTS (mTLS)

```bash
COMM_MODE=MQTTS \
MQTT_HOST=mqtt.tesaiot.com MQTT_PORT=8883 \
CERTS_DIR=./certs_credentials \
./device_client
```

## Behavior

- Builds JSON `{device_id, timestamp, data}` by reading field structure from schema file if available
  - Auto-searches at `../download/data_schema.txt` (or set via `SCHEMA_PATH`)
  - Supports types `integer/number/string/boolean` with minimum/maximum and `enum` (picks first value)
  - Falls back to sample fields if no schema file found
- TLS verification is enforced. Do not disable SSL. Paths are read from `CERTS_DIR/`.
- Reads `endpoints.json` if present and uses `api_base_url` or `ingest_base_url`.
- Device ID is read from `device_id.txt` or overridden by `DEVICE_ID` env var.

## Knowledge

- Schema-driven payload structure: The program reads schema from `../download/data_schema.txt` (or `SCHEMA_PATH`) and generates random values that conform to each field's specification (range/enum/type) to make testing reflect real data patterns.
- Port matching: mTLS → HTTPS uses 9444, MQTTS uses 8883 per platform design, reducing SAN/ALPN confusion and security policy issues.
- Trust model: TLS verification uses system trust to trust `tesaiot.com`. Avoid placing unrelated CAs that could interfere (avoid unrelated CA certificates).
- CSR device: Per security principles, bundles generated from CSR will not include private key in the ZIP. You must place the device-generated key at `certs_credentials/client_key.pem` before use.

## Troubleshooting

- TLS Handshake succeeds but send fails (HTTPS 4xx): Check `X-API-KEY` for serverTLS mode (mTLS doesn't use API key), check system time/timestamp format per schema.
- mTLS shows error about missing private key: Run `./scripts/sync_csr_key.sh <DEVICE_ID>` to copy from `./csr/`
- SAN/domain mismatch: Verify you're using `https://tesaiot.com:9444` for mTLS ingest (not port 443)
- Data doesn't match schema: Set `SCHEMA_PATH=/path/to/data_schema.txt` to point to your schema file

## Advanced Topics

- Environment variables:
  - `COMM_MODE` (HTTPS|MQTTS), `API_BASE_URL`, `MQTT_HOST`, `MQTT_PORT`, `CERTS_DIR`, `SCHEMA_PATH`, `TLS_BACKEND`
  - Tuning: `--period` (minutes), `--interval` (seconds)

- Cross-platform notes:
  - Linux / Raspberry Pi: install dev packages (see Build section). ARM/aarch64 works out-of-the-box
  - macOS: Homebrew OpenSSL is auto-detected; export CPPFLAGS/LDFLAGS if needed

## Legacy Clients (optional)

- See `./c/` for simple libmosquitto/libcurl senders (kept for comparison). The default build uses Mongoose.

## CLI Flags

- `--mode HTTPS|MQTTS`: force transport, overrides `COMM_MODE` env.
- `--host <host>` and `--port <n>`: override MQTT host/port (MQTTS) or base host/port (HTTPS when combined with `API_BASE_URL`).
- `--period <minutes>`: total send duration in minutes.
- `--interval <seconds>`: send interval in seconds.

## Test Results (2025-09-15) — 2-minute run

- HTTPS mTLS: OK. Published telemetry every 10s for 2 minutes.
  - Command: `COMM_MODE=HTTPS API_BASE_URL=https://tesaiot.com:9444 CERTS_DIR=./certs_credentials ./device_client --period 2 --interval 10`
  - Notes: client cert/key were used; server TLS verification via system trust (public CA) succeeded.
- MQTTS mTLS (port 8883): OK. Published every 10s for 2 minutes.
  - Command: `COMM_MODE=MQTTS CERTS_DIR=./certs_credentials ./device_client --host mqtt.tesaiot.com --port 8883 --period 2 --interval 10`
  - Notes: TLS handshake and PUBACK received (QoS1) each cycle.

## Recommendations

- For HTTPS mTLS: no further action needed; current config works with system trust for tesaiot.com.
- For MQTTS mTLS: verify that the device's client certificate is authorized on the broker and that the broker maps client cert to permissions for topic `device/<device_id>/telemetry`.

## Schema Tips

- Schema file follows the example format in `download/data_schema.txt` (simple JSON Schema)
- For CSR devices, sync private key to `certs_credentials/client_key.pem` before using mTLS
  - Helper script: `./scripts/sync_csr_key.sh <DEVICE_ID>`
