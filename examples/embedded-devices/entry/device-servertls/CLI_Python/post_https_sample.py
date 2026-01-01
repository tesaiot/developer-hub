#!/usr/bin/env python3
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

"""Send a sample telemetry document to TESAIoT over HTTPS (server-TLS).

Tutorial tone: mimic the steps from README.md so beginners can run the script,
inspect the payload, and map flags back to the onboarding checklist.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import math
import os
import random
import ssl
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

DEFAULT_ENDPOINT = "/api/v1/telemetry"


def load_text(path: Path) -> str:
    """Read UTF-8 text from *path* and fail fast with a friendly message."""
    try:
        return path.read_text(encoding="utf-8").strip()
    except FileNotFoundError as exc:
        raise SystemExit(f"Required file not found: {path}") from exc


def resolve_base_url(certs_dir: Path, explicit: str | None) -> str:
    """Resolve ingest base URL by inspecting `endpoints.json` if no override is provided."""
    if explicit:
        return explicit.rstrip("/")

    endpoints_path = certs_dir / "endpoints.json"
    if endpoints_path.exists():
        try:
            endpoints = json.loads(endpoints_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            endpoints = {}
        for key in ("ingest_base_url", "api_base_url"):
            value = endpoints.get(key)
            if isinstance(value, str) and value:
                return value.rstrip("/")
    return "https://tesaiot.com"


def generate_payload(device_id: str) -> dict:
    """Build a telemetry payload that matches the tutorial's medical sensor schema."""
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
    return {
        "device_id": device_id,
        "timestamp": now,
        "data": data,
    }


def build_ssl_context(ca_path: Path) -> ssl.SSLContext:
    """Create an SSL context that trusts the TESAIoT CA bundle shipped with the tutorial."""
    ctx = ssl.create_default_context(purpose=ssl.Purpose.SERVER_AUTH)
    ctx.check_hostname = True
    try:
        ctx.load_verify_locations(cafile=str(ca_path))
    except FileNotFoundError as exc:
        raise SystemExit(f"CA bundle not found: {ca_path}") from exc
    return ctx


def post_payload(url: str, payload: dict, api_key: str, ca_path: Path, timeout: int, dry_run: bool) -> None:
    """Submit the telemetry payload to TESAIoT or print it when --dry-run is enabled."""
    encoded = json.dumps(payload).encode("utf-8")
    if dry_run:
        print("[DRY-RUN] Would POST", url)
        print(json.dumps(payload, indent=2))
        return

    context = build_ssl_context(ca_path)
    headers = {
        "Content-Type": "application/json",
        "Content-Length": str(len(encoded)),
        "X-API-KEY": api_key,
    }
    request = urllib.request.Request(url=url, data=encoded, headers=headers, method="POST")
    try:
        with urllib.request.urlopen(request, context=context, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            print(f"[OK] HTTP {response.status} {response.reason}")
            if body:
                print(body)
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"[ERROR] HTTP {exc.code} {exc.reason}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise SystemExit(f"[ERROR] Request failed: {exc}") from exc


def parse_args() -> argparse.Namespace:
    """Collect CLI arguments so the script mirrors the walkthrough in the README."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--certs-dir",
        default=str(Path(__file__).resolve().parent.parent / "certs_credentials"),
        help="Path to the Server-TLS credential bundle (default: ../certs_credentials)",
    )
    parser.add_argument(
        "--base-url",
        help="Override ingest base URL (default: derive from endpoints.json or https://tesaiot.com)",
    )
    parser.add_argument(
        "--endpoint",
        default=DEFAULT_ENDPOINT,
        help=f"Relative API endpoint (default: {DEFAULT_ENDPOINT})",
    )
    parser.add_argument(
        "--device-id",
        help="Override device ID (default: read from device_id.txt)",
    )
    parser.add_argument("--timeout", type=int, default=10, help="HTTPS timeout in seconds (default: 10)")
    parser.add_argument("--interval", type=float, default=5.0, help="Seconds between sends when looping (default: 5)")
    parser.add_argument("--count", type=int, default=1, help="Number of payloads to send (default: 1)")
    parser.add_argument("--period", type=float, default=0.0, help="Total duration in minutes (overrides count when >0)")
    parser.add_argument("--dry-run", action="store_true", help="Print the payload and URL without sending the request")
    return parser.parse_args()


def main() -> None:
    """Entry point used by the tutorial to send one or many HTTPS requests."""
    args = parse_args()
    certs_dir = Path(args.certs_dir).expanduser().resolve()
    if not certs_dir.exists():
        raise SystemExit(f"Credential directory not found: {certs_dir}")

    device_id_path = certs_dir / "device_id.txt"
    api_key_path = certs_dir / "api_key.txt"
    ca_path = certs_dir / "ca-chain.pem"

    device_id = args.device_id or load_text(device_id_path)
    api_key = load_text(api_key_path)

    base_url = resolve_base_url(certs_dir, args.base_url)
    endpoint = args.endpoint.lstrip("/")
    url = urllib.parse.urljoin(base_url.rstrip("/") + "/", endpoint)

    count = args.count
    interval = max(args.interval, 0.1)
    if args.period > 0 and args.count == 1:
        iterations = int(math.floor((args.period * 60.0) / interval))
        count = max(1, iterations)
    if count < 1:
        count = 1

    for idx in range(1, count + 1):
        payload = generate_payload(device_id)
        print(f"[INFO] Sending payload {idx}/{count}")
        post_payload(
            url=url,
            payload=payload,
            api_key=api_key,
            ca_path=ca_path,
            timeout=args.timeout,
            dry_run=args.dry_run,
        )
        if idx < count:
            time.sleep(interval)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit("Interrupted by user.")
