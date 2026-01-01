#!/usr/bin/env bash

# Copyright (c) 2025 TESAIoT Platform (TESA)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Publish a schema-aligned telemetry sample to TESAIoT over MQTTS using
# mosquitto_pub.  Script doubles as a tutorial companion so the options match
# the narrative in README.md.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CERTS_DIR=${CERTS_DIR:-"$SCRIPT_DIR/../certs_credentials"}
COUNT=${COUNT:-1}
INTERVAL=${INTERVAL:-5}
PERIOD_MIN=${PERIOD_MIN:-0}
DRY_RUN=${DRY_RUN:-0}
TLS_VERSION=${TLS_VERSION:-tlsv1.2}

log() {
  printf '[%s] %s\n' "$(date -u +'%Y-%m-%dT%H:%M:%SZ')" "$*"
}

die() {
  printf '[ERROR] %s\n' "$*" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tls-version)
      TLS_VERSION="$2"; shift 2 ;;
    --count)
      COUNT="$2"; shift 2 ;;
    --interval)
      INTERVAL="$2"; shift 2 ;;
    --period)
      PERIOD_MIN="$2"; shift 2 ;;
    --dry-run)
      DRY_RUN=1; shift 1 ;;
    -h|--help)
      cat <<USAGE
Usage: $0 [--count N] [--interval SEC] [--period MIN] [--dry-run] [--tls-version VERSION]

Environment overrides: CERTS_DIR, DEVICE_ID, MQTT_HOST, MQTT_PORT, TOPIC, QOS, MOSQUITTO_PUB, TLS_VERSION
USAGE
      exit 0 ;;
    *)
      die "Unknown argument: $1" ;;
  esac
done

command -v python3 >/dev/null 2>&1 || die "python3 is required"
MOSQUITTO_PUB=${MOSQUITTO_PUB:-mosquitto_pub}
command -v "$MOSQUITTO_PUB" >/dev/null 2>&1 || die "'$MOSQUITTO_PUB' not found"
[[ -d "$CERTS_DIR" ]] || die "CERTS_DIR '$CERTS_DIR' does not exist"

CA_FILE=${CA_FILE:-"$CERTS_DIR/ca-chain.pem"}
CLIENT_CERT=${CLIENT_CERT:-"$CERTS_DIR/client_cert.pem"}
CLIENT_KEY=${CLIENT_KEY:-"$CERTS_DIR/client_key.pem"}
DEVICE_FILE=${DEVICE_FILE:-"$CERTS_DIR/device_id.txt"}

for path in "$CA_FILE" "$CLIENT_CERT" "$CLIENT_KEY" "$DEVICE_FILE"; do
  [[ -f "$path" ]] || die "Required file '$path' is missing"
  [[ -r "$path" ]] || die "Required file '$path' is not readable"
  if [[ "$path" == *client_key.pem ]]; then
    chmod 600 "$path" 2>/dev/null || true
  fi
done

DEVICE_ID=${DEVICE_ID:-$(<"$DEVICE_FILE")}
DEVICE_ID=$(echo "$DEVICE_ID" | tr -d '\r')
[[ -n "$DEVICE_ID" ]] || die "Device ID is empty"

DEFAULT_HOST=mqtt.tesaiot.com
DEFAULT_PORT=8883
if [[ -z "${MQTT_HOST:-}" || -z "${MQTT_PORT:-}" ]]; then
  ENDPOINTS_JSON="$CERTS_DIR/endpoints.json"
  if [[ -f "$ENDPOINTS_JSON" ]]; then
    read -r parsed_host parsed_port < <(
      ENDPOINTS_JSON="$ENDPOINTS_JSON" python3 - <<'PY'
import json
import os
from pathlib import Path

path = Path(os.environ.get("ENDPOINTS_JSON", ""))
try:
    data = json.loads(path.read_text(encoding="utf-8"))
except Exception:
    print()
    print()
else:
    print(data.get("mqtt_host", ""))
    print(data.get("mqtt_mtls_port", ""))
PY
    )
    if [[ -z "${MQTT_HOST:-}" && -n "$parsed_host" ]]; then
      MQTT_HOST=$parsed_host
    fi
    if [[ -z "${MQTT_PORT:-}" && -n "$parsed_port" ]]; then
      MQTT_PORT=$parsed_port
    fi
  fi
fi
MQTT_HOST=${MQTT_HOST:-$DEFAULT_HOST}
MQTT_PORT=${MQTT_PORT:-$DEFAULT_PORT}

TOPIC=${TOPIC:-"device/${DEVICE_ID}/telemetry"}
QOS=${QOS:-1}

if ! [[ "$COUNT" =~ ^[0-9]+$ ]]; then
  die "COUNT must be an integer"
fi
if ! [[ "$INTERVAL" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  die "INTERVAL must be numeric"
fi
if ! [[ "$PERIOD_MIN" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  die "PERIOD must be numeric (minutes)"
fi
if ! [[ "$TLS_VERSION" =~ ^tlsv1\.(2|3)$ ]]; then
  die "TLS_VERSION must be tlsv1.2 or tlsv1.3"
fi

TOTAL_COUNT=$COUNT
if [[ "$PERIOD_MIN" != "0" && "$PERIOD_MIN" != "0.0" ]]; then
  TOTAL_COUNT=$(INTERVAL="$INTERVAL" PERIOD_MIN="$PERIOD_MIN" python3 - <<'PY'
import math
import os
interval = float(os.environ.get("INTERVAL", "5"))
period_min = float(os.environ.get("PERIOD_MIN", "0"))
if interval <= 0:
    interval = 5.0
if period_min <= 0:
    total = 1
else:
    total = max(1, int(math.floor(period_min * 60.0 / interval)))
print(total)
PY
  )
fi

if ! [[ "$TOTAL_COUNT" =~ ^[0-9]+$ ]]; then
  die "Resolved publish count is invalid"
fi
if (( TOTAL_COUNT < 1 )); then
  TOTAL_COUNT=1
fi

cat <<'PY' >"$SCRIPT_DIR/.gen_payload.py"
from __future__ import annotations
import datetime as dt
import json
import os
import random

device_id = os.environ["DEVICE_ID"].strip()
now = dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat()

data = {
    "spo2": random.randint(94, 99),
    "spo2_unit": "%",
    "temperature": round(random.uniform(36.2, 37.5), 2),
    "temperature_unit": "°C",
    "heart_rate": random.randint(65, 95),
    "heart_rate_bpm": random.randint(65, 95),
    "perfusion_index": round(random.uniform(2.5, 6.5), 2),
    "motion": random.choice([True, False]),
    "signal_quality": random.randint(85, 100),
    "rhythm": random.choice(["sinus", "afib", "svt", "vt", "unknown"]),
    "rr_interval_ms": round(random.uniform(600.0, 950.0), 1),
    "qt_interval_ms": round(random.uniform(320.0, 420.0), 1),
    "site": random.choice(["temporal", "oral", "core"]),
}

payload = {
    "device_id": device_id,
    "timestamp": now,
    "data": data,
}
print(json.dumps(payload, separators=(",", ":")))
PY

PAYLOAD_GENERATOR="$SCRIPT_DIR/.gen_payload.py"
trap 'rm -f "$PAYLOAD_GENERATOR"' EXIT

log "Using device_id=$DEVICE_ID host=$MQTT_HOST port=$MQTT_PORT topic=$TOPIC tls=$TLS_VERSION"
log "Credentials: cert=$CLIENT_CERT key=$CLIENT_KEY ca=$CA_FILE"

for ((i=1; i<=TOTAL_COUNT; ++i)); do
  PAYLOAD=$(DEVICE_ID="$DEVICE_ID" python3 "$PAYLOAD_GENERATOR")
  log "Publishing sample $i/$TOTAL_COUNT"
  if (( DRY_RUN )); then
    echo "$PAYLOAD"
  else
    "$MOSQUITTO_PUB" \
      --cafile "$CA_FILE" \
      --cert "$CLIENT_CERT" \
      --key "$CLIENT_KEY" \
      --host "$MQTT_HOST" \
      --port "$MQTT_PORT" \
      --id "$DEVICE_ID" \
      --tls-version "$TLS_VERSION" \
      --protocol-version mqttv311 \
      --qos "$QOS" \
      --topic "$TOPIC" \
      --message "$PAYLOAD"
  fi

  if (( i < TOTAL_COUNT )); then
    sleep "$INTERVAL"
  fi
done

log "Done."
