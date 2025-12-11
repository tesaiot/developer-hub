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

"""Typer-based CLI that exercises the TESAIoT API Gateway via APISIX using one API key.

Highlights
- Auto-loads `.env` and `config.yaml` so the CLI can run without extra wiring.
- Provides commands for listing devices, fetching detail, telemetry, and aggregated stats.
- Comments emphasise the Why/What/How for developers who are new to the platform.
"""
from __future__ import annotations

import json
import os
import re
import ssl
import threading
import time
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from functools import lru_cache
from pathlib import Path
from typing import Any, List, Optional
from urllib.parse import urlparse, urlunparse

import requests
import typer
import yaml
from dotenv import load_dotenv
from rich import box
from rich.console import Console
from rich.table import Table
from websocket import WebSocketApp

APP_DIR = Path(__file__).resolve().parent
console = Console()

# Main Typer CLI plus device/stats subcommands so each topic has focused help text.
cli = typer.Typer(help="Sample CLI for interacting with the TESAIoT API Gateway")
devices_cli = typer.Typer(help="Device-related commands")
stats_cli = typer.Typer(help="Summary statistics commands")
cli.add_typer(devices_cli, name="devices")
cli.add_typer(stats_cli, name="stats")


class TESAIoTClientError(Exception):
    """Raised when TESAIoT API returns an error response."""


@dataclass
class Settings:
    base_url: str
    timeout: int
    verify_tls: bool
    api_key: str
    default_schema: str
    realtime_ws_url: str
    realtime_subscribe_timeout: int
    organization_id: Optional[str] = None
    user_email: Optional[str] = None


def _derive_realtime_ws_url(http_base_url: str) -> str:
    """Convert the REST base URL into the platform's telemetry WebSocket URL."""
    parsed = urlparse(http_base_url)
    scheme = "wss" if parsed.scheme == "https" else "ws"
    netloc = parsed.netloc or parsed.path.split("/")[0]
    if not netloc:
        raise TESAIoTClientError(
            "Unable to derive realtime WebSocket URL. Set realtime.ws_url in config.yaml or TESAIOT_REALTIME_WS_URL."
        )
    return urlunparse((scheme, netloc, "/ws/telemetry", "", "", ""))


@lru_cache(maxsize=1)
def load_settings() -> Settings:
    """Central loader that merges `.env` overrides with `config.yaml`, cached for reuse."""
    env_candidates = [APP_DIR / ".env", APP_DIR / ".env.local"]
    for env_path in env_candidates:
        if env_path.exists():
            load_dotenv(env_path)

    api_key = os.getenv("TESAIOT_API_KEY")
    if not api_key:
        raise TESAIoTClientError(
            "TESAIOT_API_KEY not found. Copy .env.example to .env and set the organisation API key."
        )

    organization_id = os.getenv("TESAIOT_ORGANIZATION_ID") or None
    user_email = os.getenv("TESAIOT_USER_EMAIL") or None

    config_path = APP_DIR / "config.yaml"
    if not config_path.exists():
        raise TESAIoTClientError("config.yaml is missing in the sample directory")

    with config_path.open("r", encoding="utf-8") as cfg_file:
        config = yaml.safe_load(cfg_file) or {}

    api_gateway_cfg = config.get("api_gateway", {})
    base_url = (api_gateway_cfg.get("base_url") or "https://admin.tesaiot.com/api/v1").rstrip("/")
    timeout = int(api_gateway_cfg.get("timeout_seconds", 15))
    verify_tls = bool(api_gateway_cfg.get("verify_tls", True))
    default_schema = api_gateway_cfg.get("default_schema", "industrial_device")

    realtime_cfg = config.get("realtime", {})
    realtime_ws_url = (
        os.getenv("TESAIOT_REALTIME_WS_URL")
        or realtime_cfg.get("ws_url")
        or _derive_realtime_ws_url(base_url)
    )
    realtime_subscribe_timeout = int(realtime_cfg.get("subscribe_ack_timeout_seconds", 5))

    if realtime_subscribe_timeout <= 0:
        raise TESAIoTClientError("realtime.subscribe_ack_timeout_seconds must be positive")

    return Settings(
        base_url=base_url,
        timeout=timeout,
        verify_tls=verify_tls,
        api_key=api_key,
        default_schema=default_schema,
        realtime_ws_url=realtime_ws_url,
        realtime_subscribe_timeout=realtime_subscribe_timeout,
        organization_id=organization_id,
        user_email=user_email,
    )


class TESAIoTClient:
    """Thin wrapper around requests.Session tuned for the tutorial use-cases."""

    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._session = requests.Session()
        self._session.headers.update(
            {
                "X-API-KEY": settings.api_key,
                "Accept": "application/json",
                "User-Agent": "TESAIoT-Tutorial-Client/2025.09",
            }
        )
        if settings.organization_id:
            # Preserve multi-tenant context if backend expects explicit org hints.
            self._session.headers["X-Organization-Id"] = settings.organization_id
        if settings.user_email:
            self._session.headers["X-User-Email"] = settings.user_email

    def _request(self, method: str, path: str, **kwargs: Any) -> Any:
        """Central request helper that builds URLs, enforces TLS, and raises friendly errors."""
        url = self._build_url(path)
        try:
            response = self._session.request(
                method=method,
                url=url,
                timeout=self._settings.timeout,
                verify=self._settings.verify_tls,
                **kwargs,
            )
            response.raise_for_status()
        except requests.exceptions.HTTPError as err:
            detail = _extract_error_detail(err.response)
            raise TESAIoTClientError(f"Request to {url} failed: {detail}") from err
        except requests.exceptions.RequestException as err:
            raise TESAIoTClientError(f"Unable to reach {url}: {err}") from err

        if not response.content:
            return None
        try:
            return response.json()
        except ValueError:
            return response.text

    def _build_url(self, path: str) -> str:
        if not path.startswith("/"):
            path = f"/{path}"
        return f"{self._settings.base_url}{path}"

    def list_devices(self) -> Any:
        """Return the raw payload from `/devices`, letting commands format output."""
        return self._request("GET", "/devices")

    def get_device(self, device_id: str) -> Any:
        return self._request("GET", f"/devices/{device_id}")

    def get_device_schema(self, schema_type: str) -> Any:
        return self._request("GET", f"/devices/schemas/{schema_type}")

    def get_device_telemetry(
        self,
        device_id: str,
        limit: int,
        *,
        start_time: Optional[str] = None,
        end_time: Optional[str] = None,
    ) -> Any:
        params = {"limit": max(1, min(limit, 1000))}
        if start_time:
            params["start_time"] = start_time
        if end_time:
            params["end_time"] = end_time
        return self._request("GET", f"/devices/{device_id}/telemetry", params=params)

    def get_device_stats(self) -> Any:
        return self._request("GET", "/devices/stats")

    def get_dashboard_stats(self) -> Any:
        # Dashboard stats include adoption metrics similar to AWS/Azure IoT best-practice dashboards.
        return self._request("GET", "/dashboard/stats")


def get_client() -> TESAIoTClient:
    return TESAIoTClient(load_settings())


@devices_cli.command("list")
def list_devices(limit: int = typer.Option(20, help="Maximum number of devices to display")) -> None:
    """List devices with the most relevant fields for quick inspection."""
    client = get_client()
    data = client.list_devices()
    if not data:
        console.print("[yellow]No devices found[/yellow]")
        return

    rows = data if isinstance(data, list) else data.get("devices", [])
    if not isinstance(rows, list):
        console.print("[red]Unexpected response shape[/red]")
        console.print_json(data=data)
        return

    table = Table(title="Device Inventory", box=box.MINIMAL_DOUBLE_HEAD)
    table.add_column("#", justify="right")
    table.add_column("Device ID", overflow="fold")
    table.add_column("Name", overflow="fold")
    table.add_column("Status")
    table.add_column("Type")
    table.add_column("Last Telemetry")

    for idx, device in enumerate(rows[:limit], start=1):
        table.add_row(
            str(idx),
            str(device.get("device_id") or device.get("id") or device.get("_id") or "-"),
            str(device.get("name") or device.get("label") or "-"),
            str(device.get("status") or "unknown"),
            str(device.get("type") or device.get("category") or "-"),
            str(device.get("telemetry_summary", {}).get("last_update") or "-"),
        )

    console.print(table)
    console.print(f"Displayed {min(limit, len(rows))} of {len(rows)} entries")


@devices_cli.command("show")
def show_device(device_id: str = typer.Argument(..., help="Device ID to inspect")) -> None:
    """Dump the JSON profile for a specific device (Thai prompt kept in Typer help)."""
    client = get_client()
    device = client.get_device(device_id)
    console.print_json(data=device)


@devices_cli.command("schema")
def show_schema(
    schema_type: Optional[str] = typer.Option(
        None,
        help="Schema type such as industrial_device or medical_device (defaults from config.yaml)",
    ),
) -> None:
    """Fetch schema definitions so firmware engineers can align payloads."""
    settings = load_settings()
    schema_name = schema_type or settings.default_schema
    client = TESAIoTClient(settings)
    schema = client.get_device_schema(schema_name)
    console.print_json(data=schema)


@devices_cli.command("telemetry")
def show_telemetry(
    device_id: str = typer.Argument(..., help="Device ID"),
    limit: int = typer.Option(20, help="Number of raw telemetry records to display"),
    start: Optional[str] = typer.Option(
        None,
        "--start",
        help="Inclusive ISO 8601 timestamp to start from (e.g. 2025-09-20T18:30:00Z)",
    ),
    end: Optional[str] = typer.Option(
        None,
        "--end",
        help="Inclusive ISO 8601 timestamp to stop at",
    ),
    since: Optional[str] = typer.Option(
        None,
        "--since",
        help="Relative lookback such as 15m, 2h, 1d (takes priority over --start/--end)",
    ),
) -> None:
    """Render recent raw telemetry records as a table for quick inspection."""
    client = get_client()
    if since and (start or end):
        raise TESAIoTClientError("Use either --since or --start/--end, not both")

    start_iso, end_iso = _resolve_time_bounds(start, end, since)

    payload = client.get_device_telemetry(
        device_id,
        limit,
        start_time=start_iso,
        end_time=end_iso,
    )
    telemetry_records = []
    if isinstance(payload, dict):
        telemetry_records = payload.get("telemetry") or payload.get("data") or payload.get("records") or []
    if not telemetry_records:
        console.print("[yellow]No telemetry available[/yellow]")
        return

    window_parts = []
    if start_iso:
        window_parts.append(f"start: {start_iso}")
    if end_iso:
        window_parts.append(f"end: {end_iso}")
    if not window_parts and since:
        window_parts.append(f"since: last {since}")

    table_title = f"Telemetry for {device_id}"
    if window_parts:
        table_title = f"{table_title} ({', '.join(window_parts)})"

    table = Table(title=table_title, box=box.SIMPLE_HEAVY)
    table.add_column("Timestamp", overflow="fold")
    table.add_column("Values", overflow="fold")

    for entry in telemetry_records:
        timestamp = entry.get("timestamp") or entry.get("time") or "-"
        data_blob = entry.get("data") or entry.get("values") or entry
        pretty_data = json.dumps(data_blob, ensure_ascii=False)
        table.add_row(str(timestamp), pretty_data)

    console.print(table)


@devices_cli.command("stream")
def stream_telemetry(
    device_ids: List[str] = typer.Argument(
        ..., help="One or more device IDs to subscribe to for live telemetry"
    ),
    duration: Optional[int] = typer.Option(
        None,
        "--duration",
        help="Automatically disconnect after N seconds (default: wait for Ctrl+C)",
    ),
    max_events: Optional[int] = typer.Option(
        None,
        "--max-events",
        help="Stop after receiving this many telemetry messages",
    ),
    ws_url: Optional[str] = typer.Option(
        None,
        "--ws-url",
        help="Override the realtime WebSocket endpoint (defaults to config.yaml / env)",
    ),
    json_output: bool = typer.Option(
        False,
        "--json-output",
        help="Print each event as raw JSON instead of a formatted summary",
        show_default=False,
    ),
) -> None:
    """Stream live telemetry using the platform's WebSocket gateway."""
    if not device_ids:
        raise TESAIoTClientError("Provide at least one device ID to subscribe to")

    settings = load_settings()
    target_ws_url = (ws_url or settings.realtime_ws_url).strip()

    headers = [f"X-API-Key: {settings.api_key}"]
    if settings.organization_id:
        headers.append(f"X-Organization-Id: {settings.organization_id}")
    if settings.user_email:
        headers.append(f"X-User-Email: {settings.user_email}")

    subscription_ack = threading.Event()
    stop_event = threading.Event()
    message_counter = 0

    def _handle_subscribed(payload: dict[str, Any]) -> None:
        subscribed_ids = payload.get("deviceIds") or device_ids
        console.print(
            "[green]✓ Subscribed[/green] to " + ", ".join(str(device) for device in subscribed_ids)
        )
        subscription_ack.set()

    def _handle_message(payload: dict[str, Any]) -> None:
        nonlocal message_counter
        msg_type = payload.get("type")

        if msg_type == "subscribed":
            _handle_subscribed(payload)
            return
        if msg_type == "ping":
            console.print("[dim]• keep-alive ping[/dim]")
            return

        message_counter += 1
        if json_output:
            console.print_json(data=payload)
        else:
            device = payload.get("deviceId") or payload.get("device_id") or "?"
            timestamp = payload.get("timestamp") or payload.get("time") or "?"
            values = payload.get("data") or payload
            console.print(
                f"[cyan]{timestamp}[/cyan] [magenta]{device}[/magenta] {json.dumps(values, ensure_ascii=False)}"
            )

        if max_events and message_counter >= max_events:
            stop_event.set()

    def _on_open(ws_app: WebSocketApp) -> None:
        console.print(f"[green]Connected[/green] to {target_ws_url}. Requesting subscription...")
        subscribe_payload = json.dumps({"type": "subscribe", "deviceIds": device_ids})
        ws_app.send(subscribe_payload)

    def _on_message(_: WebSocketApp, message: str) -> None:
        try:
            payload = json.loads(message)
        except json.JSONDecodeError:
            console.print(f"[red]Received non-JSON frame:[/red] {message}")
            return

        _handle_message(payload)

    def _on_error(_: WebSocketApp, error: Any) -> None:
        console.print(f"[red]WebSocket error:[/red] {error}")
        subscription_ack.set()
        stop_event.set()

    def _on_close(_: WebSocketApp, status_code: int, msg: str) -> None:
        status = status_code if status_code is not None else "?"
        if msg:
            console.print(f"[yellow]Connection closed[/yellow] (code={status}, reason={msg})")
        else:
            console.print(f"[yellow]Connection closed[/yellow] (code={status})")
        subscription_ack.set()
        stop_event.set()

    ws_app = WebSocketApp(
        target_ws_url,
        header=headers,
        on_open=_on_open,
        on_message=_on_message,
        on_error=_on_error,
        on_close=_on_close,
    )

    run_kwargs: dict[str, Any] = {"ping_interval": 25, "ping_timeout": 10}
    if settings.verify_tls is False:
        run_kwargs["sslopt"] = {"cert_reqs": ssl.CERT_NONE}

    worker = threading.Thread(target=ws_app.run_forever, kwargs=run_kwargs, daemon=True)
    worker.start()

    if not subscription_ack.wait(timeout=settings.realtime_subscribe_timeout):
        stop_event.set()
        ws_app.close()
        worker.join(timeout=5)
        raise TESAIoTClientError(
            "Subscription acknowledgement not received in time. Check device IDs and API key scopes."
        )

    if duration:
        console.print(
            f"Streaming live telemetry for approximately {duration} seconds. Press Ctrl+C to stop early."
        )
        try:
            stop_event.wait(timeout=duration)
        except KeyboardInterrupt:
            console.print("Stopping stream...")
            stop_event.set()
    else:
        console.print("Streaming live telemetry. Press Ctrl+C to stop.")
        try:
            while not stop_event.is_set():
                time.sleep(0.5)
        except KeyboardInterrupt:
            console.print("Stopping stream...")
            stop_event.set()

    ws_app.close()
    worker.join(timeout=5)

    console.print(f"Received {message_counter} telemetry message(s)")


@stats_cli.command("summary")
def summary_stats() -> None:
    """Summarise device fleet posture (totals, auth mix, throughput)."""
    client = get_client()
    device_stats = client.get_device_stats()
    dashboard_stats = {}
    try:
        dashboard_stats = client.get_dashboard_stats()
    except TESAIoTClientError as err:
        console.print(f"[yellow]Warning:[/yellow] unable to load dashboard stats: {err}")

    table = Table(title="Device Posture", box=box.MINIMAL_DOUBLE_HEAD)
    table.add_column("Metric")
    table.add_column("Value", justify="right")

    if isinstance(device_stats, dict):
        table.add_row("Total devices", str(device_stats.get("total_devices", "-")))
        table.add_row("Active devices", str(device_stats.get("active_devices", "-")))
        table.add_row("Inactive devices", str(device_stats.get("inactive_devices", "-")))
        table.add_row("Pending devices", str(device_stats.get("pending_devices", "-")))

        device_types = device_stats.get("device_types", {})
        if device_types:
            table.add_row("Device types", json.dumps(device_types, ensure_ascii=False))
        auth_modes = device_stats.get("auth_modes", {})
        if auth_modes:
            table.add_row("Auth modes", json.dumps(auth_modes, ensure_ascii=False))

    if isinstance(dashboard_stats, dict):
        total_devices_all = dashboard_stats.get("totalDevices") or dashboard_stats.get("total_devices")
        if total_devices_all is not None:
            table.add_row("Dashboard total", str(total_devices_all))
        active_rate = dashboard_stats.get("active_rate")
        if isinstance(active_rate, (int, float)):
            table.add_row("Active rate", f"{active_rate:.2%}")
        secure_devices = dashboard_stats.get("secure_devices")
        if secure_devices is not None:
            table.add_row("Secure devices", str(secure_devices))
        secure_rate = dashboard_stats.get("secure_rate")
        if isinstance(secure_rate, (int, float)):
            table.add_row("Secure rate", f"{secure_rate:.2%}")
        ingest_rate = dashboard_stats.get("messages_per_minute") or dashboard_stats.get("throughput", {}).get("avg")
        if ingest_rate is not None:
            table.add_row("Telemetry avg msg/min", str(ingest_rate))
        api_requests = dashboard_stats.get("api_requests_per_min") or dashboard_stats.get("apiRequestsPerMin")
        if api_requests is not None:
            table.add_row("API req/min", str(api_requests))

    console.print(table)


def _parse_iso8601(value: str) -> datetime:
    """Parse ISO 8601 timestamps while accepting a trailing Z."""
    candidate = value.strip()
    candidate = candidate.replace("Z", "+00:00")
    try:
        parsed = datetime.fromisoformat(candidate)
    except ValueError as exc:
        raise TESAIoTClientError(
            f"Invalid ISO 8601 timestamp '{value}'. Use formats like 2025-09-20T18:30:00Z"
        ) from exc

    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)


def _parse_relative_offset(expr: str) -> timedelta:
    """Parse shorthand durations such as 15m, 2h, 1d."""
    match = re.fullmatch(r"(?P<value>\d+)(?P<unit>[smhd])", expr.strip().lower())
    if not match:
        raise TESAIoTClientError(
            "Invalid relative duration. Use patterns like 15m, 2h, 1d to represent minutes, hours, or days."
        )

    value = int(match.group("value"))
    unit = match.group("unit")
    if unit == "s":
        return timedelta(seconds=value)
    if unit == "m":
        return timedelta(minutes=value)
    if unit == "h":
        return timedelta(hours=value)
    return timedelta(days=value)


def _to_iso(dt: datetime) -> str:
    """Serialise a datetime to a UTC Zulu string."""
    return dt.astimezone(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def _resolve_time_bounds(
    start: Optional[str],
    end: Optional[str],
    since: Optional[str],
) -> tuple[Optional[str], Optional[str]]:
    """Normalise user input into ISO8601 strings the API understands."""
    if since:
        delta = _parse_relative_offset(since)
        now = datetime.now(timezone.utc)
        start_dt = now - abs(delta)
        return _to_iso(start_dt), _to_iso(now)

    start_iso = _to_iso(_parse_iso8601(start)) if start else None
    end_iso = _to_iso(_parse_iso8601(end)) if end else None

    if start_iso and end_iso and start_iso > end_iso:
        raise TESAIoTClientError("Start time must be earlier than end time")

    return start_iso, end_iso


def _extract_error_detail(response: Optional[requests.Response]) -> str:
    if not response:
        return "Unknown error"
    try:
        data = response.json()
        if isinstance(data, dict):
            return data.get("error") or data.get("message") or json.dumps(data, ensure_ascii=False)
    except ValueError:
        return response.text
    return response.text


def main() -> None:
    try:
        cli()
    except TESAIoTClientError as err:
        console.print(f"[red]Error:[/red] {err}")


if __name__ == "__main__":
    main()
