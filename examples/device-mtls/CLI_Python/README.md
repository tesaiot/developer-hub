# CLI — MQTTS mTLS Quick Sender

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

Bash helper that wraps `mosquitto_pub` to publish schema-aligned telemetry to TESAIoT over MQTT with mutual TLS.

---

## 1. Files

| File | Description |
| --- | --- |
| `publish_mqtt_sample.sh` | Bash script that synthesises telemetry, signs it with the bundle key, and publishes to the TESAIoT broker |

> The script sits next to the C example so both reuse the same credential bundle in `../certs_credentials`.

---

## 2. Prerequisites

- `mosquitto_pub` (install via `sudo apt install mosquitto-clients` or `brew install mosquitto`)
- `python3` (standard library only) for generating the JSON payload
- TESAIoT mTLS credential bundle synced into `../certs_credentials/`

---

## 3. Quick Start

1. Sync credentials: run `make prepare` from the parent directory (or copy the bundle manually).
2. Verify the Mosquitto CLI: `mosquitto_pub --help`
3. Publish telemetry with one of the recipes below.

```bash
cd tutorial/examples/device-to-platform/mTLS/CLI_Python

# Send a single payload to production broker (mqtt.tesaiot.com:8883)
./publish_mqtt_sample.sh --once

# Force TLS 1.3 during the handshake (TLS 1.2 remains the default)
./publish_mqtt_sample.sh --tls-version tlsv1.3 --once

# Keep sending every 5 seconds for 2 minutes
./publish_mqtt_sample.sh --interval 5 --period 2

# Dry-run mode to view the payload without publishing
./publish_mqtt_sample.sh --dry-run
```

---

## 4. Script Flags

| Flag | Description |
| --- | --- |
| `--certs-dir` | Directory containing the credential bundle (defaults to `../certs_credentials`) |
| `--broker-host`, `--broker-port` | Override broker host/port |
| `--device-id` | Device identifier to embed in the payload |
| `--interval`, `--period` | Publish cadence (seconds) and overall duration (minutes) |
| `--dry-run` | Print payload only, skip publishing |
| `--tls-version` | Select TLS version (`tlsv1.2` or `tlsv1.3`) |

---

## 5. Notes

- Licensed under Apache 2.0—feel free to adapt inside your lab projects while preserving the header.
- Adjust the Python snippet that builds the JSON payload to match your device schema.
- Ensure system time is accurate—TLS handshakes will fail if the clock drifts too far.
