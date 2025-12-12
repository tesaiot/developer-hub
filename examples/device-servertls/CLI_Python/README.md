# CLI — HTTPS Server-TLS Sample

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

Standalone Python helper that posts telemetry to TESAIoT over HTTPS using the Server-TLS bundle (CA + API key).

---

## 1. Files

| File | Description |
| --- | --- |
| `post_https_sample.py` | Python 3 script that loads the bundle, generates schema-aligned telemetry, and POSTs it to `/api/v1/telemetry` |

---

## 2. Quick Start

1. Sync credentials: from the parent folder run `make prepare` or copy the Server-TLS bundle into `../certs_credentials/`.
2. Ensure Python ≥ 3.8 is available (`python3 --version`).
3. Run one of the recipes below.

```bash
cd tutorial/examples/device-to-platform/serverTLS/CLI_Python

# Single telemetry document to production (https://tesaiot.com)
./post_https_sample.py

# Override base URL (e.g., staging) and preview payload only
./post_https_sample.py --base-url https://staging.tesaiot.com --dry-run

# Send every 5 seconds for two minutes
./post_https_sample.py --period 2 --interval 5
```

---

## 3. Flags & Environment Variables

| Flag / env | Description |
| --- | --- |
| `--certs-dir`, `CERTS_DIR` | Directory containing the credential bundle |
| `--base-url`, `BASE_URL` | Override ingest base URL (falls back to `endpoints.json`) |
| `--endpoint`, `ENDPOINT` | API endpoint path to POST to |
| `--period`, `--interval` | Duration in minutes and send interval in seconds |
| `--dry-run` | Print payload and exit without sending |

---

## 4. Additional Notes

- Licensed under Apache 2.0—copy and adapt as long as the header is preserved.
- `resolve_base_url` inspects `endpoints.json` when `--base-url` is omitted, mirroring device onboarding.
- TLS handshake errors usually mean the system clock is wrong or the CA file lacks `0600` permissions.
