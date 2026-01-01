# Platform-to-Application Sample (Python)

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

---

## 1. Overview

This sample shows how to talk to the TESAIoT Platform via the APISIX API Gateway using a single API key. You will learn how to list devices, fetch schema definitions, inspect telemetry windows (`--start`/`--since`) and stream live telemetry via the WebSocket gateway, plus pull basic KPIs with just a few commands.

**Suitable for:** Beginner to Intermediate Python developers, DevOps engineers, or Solution Architects who need to quickly test the API gateway.


## 2. Project Layout

```
platform-to-application/
├── .env.example              # Environment template (do not commit real .env)
├── .gitignore                # Blocks .env, __pycache__, .venv
├── README.md                 # This guide (English)
├── README-TH.md              # Thai version
├── app.py                    # Typer CLI that calls REST API via requests
├── config.yaml               # Defaults: base_url, timeout, schema type
├── requirements.txt          # Dependencies for virtualenv
├── run.sh                    # Bootstrap + run helper script (Linux/macOS)
└── __pycache__/ .venv/ ...   # Ignored automatically
```


## 3. Requirements

| Item | Description |
| ------ | ------- |
| Python | Version 3.9+ (3.11 recommended) |
| Pip / venv | Ensure `pip` + `venv` are available |
| OS | Linux, macOS, or Windows (PowerShell/WSL) |
| API Key | Valid APISIX API key with required scopes |
| Internet | Internet access (to reach TESAIoT endpoints) |

> For Windows PowerShell, replace `export` with `setx` or use a `.env` file


## 4. Getting Credentials from TESAIoT Platform

Before using this example, you need to obtain credentials from the TESAIoT Platform:

### 4.1 API Key

1. Login to **TESAIoT Admin Portal**: https://admin.tesaiot.com
2. Navigate to **Settings** > **API Keys**
3. Click **Create API Key** and select required scopes:
   - `devices:read`
   - `telemetry:read`
   - `organizations:read`
   - `dashboard:read`
   - `analytics:read`
4. Copy the generated API Key (starts with `tesa_ak_`)

### 4.2 Organization ID (Optional)

1. Navigate to **Settings** > **Organization**
2. Copy your **Organization ID** (e.g., `bdh-corporation`)

---

## 5. Environment Setup

### 5.1 Quick Start (Linux/macOS)

1. Navigate to the example folder:
   ```bash
   cd tutorial/examples/application-to-platform
   ```
2. Create virtualenv and install dependencies:
   ```bash
   ./run.sh bootstrap
   ```
   If the script warns about missing `python3-venv`, install it with `sudo apt install python3-venv`
3. Copy the example environment file and add your API Key:
   ```bash
   cp .env.example .env
   # Edit .env and set TESAIOT_API_KEY=tesa_ak_xxx
   ```
4. Run a command:
   ```bash
   ./run.sh run devices list
   ```

> **Windows Note**: Use PowerShell `python -m venv .venv ; .\.venv\Scripts\Activate.ps1` then run `pip install -r requirements.txt` and use `python app.py ...`


## 5. Environment Variables

The `.env` file supports the following values:

| Key | Description |
| --- | ------------------- |
| `TESAIOT_API_KEY` (required) | API key provided by APISIX |
| `TESAIOT_ORGANIZATION_ID` (optional) | Optional organization override |
| `TESAIOT_USER_EMAIL` (optional) | Optional user context header |
| `TESAIOT_REALTIME_WS_URL` (optional) | WebSocket endpoint for live telemetry (defaults to `wss://admin.tesaiot.com/ws/telemetry`) |

Default values from `config.yaml`:
- `api_gateway.base_url` → `https://admin.tesaiot.com/api/v1/external`
- `verify_tls` → `true`
- `default_schema` → `industrial_device`


## 6. Available Commands

All commands work through the Typer CLI (Python). Use the format `./run.sh run <command>` or `python app.py <command>` when virtualenv is activated:

| Command | What it does |
| ------- | ------------ |
| `devices list --limit 20` | Lists devices (id, name, status, type) |
| `devices show <device_id>` | Shows device profile JSON |
| `devices telemetry <device_id> --since 15m` | Fetches telemetry slices using `--since`, `--start`, and `--end` filters |
| `devices schema --schema-type industrial_device` | Retrieves schema definition + JSON schema |
| `devices stream <device_id> --duration 60` | Streams live telemetry via the WebSocket gateway |
| `stats summary` | Combines `/devices/stats` + `/dashboard/stats` |

Example usage:

```bash
# Linux/macOS
./run.sh run devices list --limit 5
./run.sh run devices telemetry 1ba776fc-1250-4a21-bc7f-fd538cdda083 --since 30m
./run.sh run devices telemetry 1ba776fc-1250-4a21-bc7f-fd538cdda083 \
  --start 2025-09-20T00:00:00Z --end 2025-09-20T01:00:00Z --limit 100
./run.sh run devices stream 1ba776fc-1250-4a21-bc7f-fd538cdda083 --duration 60
./run.sh run stats summary
```

Sample output:

```
Device Inventory
#  Device ID                        Name              Status  Type
1  1ba776fc-1250-4a21-…             Device01-mTLS-CSR  active  sensor
2  e919a2f9-ad08-40be-…             Device02-ServerTLS active  gateway
```

```
Telemetry for 1ba776fc-1250-4a21-bc7f-fd538cdda083 (start: 2025-09-20T00:00:00Z, end: 2025-09-20T01:00:00Z)
Timestamp                Values
2025-09-20T00:42:14...   {"data_heart_rate": 79, ...}
...
```

```
Streaming live telemetry. Press Ctrl+C to stop.
2025-09-20T09:31:44Z 1ba776fc-1250-4a21-bc7f-fd538cdda083 {"temperature": 24.3, "humidity": 58}
• keep-alive ping
2025-09-20T09:31:49Z 1ba776fc-1250-4a21-bc7f-fd538cdda083 {"temperature": 24.1, "humidity": 57}
Received 2 telemetry message(s)
```

```
Device Posture
Metric                Value
Total devices         2
Active devices        2
Secure rate           100.00%
Telemetry avg msg/min 0.0
```


## 7. Required Scopes

- The CLI calls endpoints: `/devices`, `/devices/<id>`, `/devices/<id>/telemetry`, `/devices/schemas/<type>`, `/devices/stats`, `/dashboard/stats`, and WebSocket `/ws/telemetry`
- API Key must have at least these scopes: `devices:read`, `telemetry:read`, `organizations:read`, `dashboard:read`, `analytics:read`
- On recent systems, the APISIX setup script will add these scopes automatically. If using an older key with only `devices:read`, contact your administrator for an update.

Verify with `curl` (replace `<key>`):
```bash
curl -i https://admin.tesaiot.com/api/v1/external/health \
  -H "X-API-Key: <key>"
```
Expected: `200` with status `{"status":"healthy"}`


## 8. Extending the Client

| Feature | How to extend |
| -------- | ------------- |
| Add CLI command | Add new Typer commands in `app.py` |
| Add parameters | Add Typer options and pass them into `_request` |
| Integrate with existing systems | Import `TESAIoTClient` into your app |
| Logging | Configure `console` / add logging module |


## 9. Security Checklist

- Do not commit `.env` or real API Keys (`.gitignore` blocks this)
- Rotate API Key when not in use (APISIX has revoke/rotate endpoint)
- Always test new keys on non-production first
- Verify `.venv`, `__pycache__` are not committed (already ignored)
- Log API Key usage for audit purposes


## 10. Troubleshooting

| Issue | Fix |
| ------ | --- |
| `401 Authentication required` | Confirm API key & required scopes |
| `404 Device not found` | Make sure device belongs to your org |
| `requests.exceptions.SSLError` | Keep HTTPS; set `verify_tls: false` temporarily only if you understand the risk |
| `ModuleNotFoundError` | Install dependencies inside virtualenv |
| `.venv` creation fails | `sudo apt install python3-venv` |


## 11. Field-Test Snapshot (2025-09-18)

Using API Key `Infineon_TESAIoT` (with full scope set)

```
TESAIOT_API_KEY=tesa_ak_<redacted> \
  PYTHONPATH=. python3 app.py devices list --limit 5
```
- Found 2 devices: `Device01-mTLS-CSR`, `Device02-ServerTLS`

```
TESAIOT_API_KEY=... python3 app.py devices telemetry 1ba776fc-... --limit 5
```
- Retrieved Vital-signs data with timestamps

```
TESAIOT_API_KEY=... python3 app.py stats summary
```
- Summary: Total=2, Active=2, Secure=100%, Telemetry avg=0 msg/min (in 15-minute window)


## 12. Next Steps

- Add CLI command to send telemetry (POST `/telemetry`) for end-to-end ingest testing
- Integrate with CI/CD (e.g., GitHub Actions) to verify API key is still valid before deploy
- Create unit tests for `TESAIoTClient` by mocking `requests.Session`
- Add integration tests for `devices stream` command (e.g., mock WebSocket server)


## 13. Getting Support

- Incident reporting: Same channel as TESAIoT Platform (Ops/Support channel)
- Infineon contact: eric.seow@infineon.com
- Platform owner: sriborrirux@gmail.com

> When opening a ticket, attach results from `stats summary` + `devices telemetry` for permission and data verification

---

Happy building on TESAIoT!
