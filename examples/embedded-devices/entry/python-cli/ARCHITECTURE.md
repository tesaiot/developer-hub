# Python CLI Architecture

## Overview

Command-line tool for interacting with TESAIoT Platform via REST API and WebSocket.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      Python CLI Application                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────┐  │
│  │  Typer CLI  │───▶│  Settings   │───▶│  REST API Client    │  │
│  │  Framework  │    │   Loader    │    │  (requests)         │  │
│  └─────────────┘    └─────────────┘    └──────────┬──────────┘  │
│         │                                         │             │
│         │           ┌─────────────┐               │             │
│         └──────────▶│  WebSocket  │               │             │
│                     │   Client    │               │             │
│                     └──────┬──────┘               │             │
│                            │                      │             │
└────────────────────────────┼──────────────────────┼─────────────┘
                             │                      │
                             ▼                      ▼
                  ┌──────────────────┐    ┌──────────────────┐
                  │ WebSocket Server │    │   API Gateway    │
                  │  (WSS :8085)     │    │  (HTTPS :443)    │
                  └──────────────────┘    └──────────────────┘
                             │                       │
                             └───────────┬───────────┘
                                         ▼
                              ┌──────────────────────┐
                              │   TESAIoT Platform   │
                              │   - TimescaleDB      │
                              │   - MQTT Bridge      │
                              │   - REST API         │
                              └──────────────────────┘
```

## Data Flow

```
┌──────────┐    ┌─────────┐    ┌─────────────┐    ┌──────────┐    ┌──────────┐
│   User   │───▶│   CLI   │───▶│ API Gateway │───▶│ REST API │───▶│ Database │
│          │    │ Command │    │ + API Key   │    │ (FastAPI)│    │          │
└──────────┘    └─────────┘    └─────────────┘    └──────────┘    └──────────┘
     ▲               │
     │               │         ┌─────────────────────────────────────────┐
     │               └────────▶│  Display formatted table (Rich)         │
     │                         └─────────────────────────────────────────┘
     │
     └─────────────────────────────────────────────────────────────────────┘
```

## Real-time Telemetry Flow

```
┌──────────┐    ┌─────────────┐    ┌───────────────┐    ┌─────────────┐
│  Device  │───▶│ MQTT Broker │───▶│  MQTT Bridge  │───▶│  WebSocket  │
│          │    │ (EMQX)      │    │               │    │   Server    │
└──────────┘    └─────────────┘    └───────────────┘    └──────┬──────┘
                                                               │
                                                               ▼
┌──────────┐    ┌─────────────┐                        ┌─────────────┐
│   User   │◀───│  CLI Output │◀───────────────────────│  WebSocket  │
│(Terminal)│    │  (Rich)     │                        │   Client    │
└──────────┘    └─────────────┘                        └─────────────┘
```

## Key Components

| Component | Description | File |
|-----------|-------------|------|
| Typer CLI | Command-line interface | `app.py` |
| Settings Loader | Merge .env and config.yaml | `app.py:load_settings()` |
| REST Client | HTTP requests | `app.py:TESAIoTClient` |
| WebSocket Client | Real-time streaming | `app.py:_realtime_ws()` |

## Authentication

| Method | Use Case | Header |
|--------|----------|--------|
| API Key | REST API | `X-API-Key: <key>` |
| Bearer Token | WebSocket | `Authorization: Bearer <token>` |

## Configuration

```yaml
# config.yaml
api:
  base_url: "https://admin.tesaiot.com"
  timeout: 30
  verify_tls: true

realtime:
  ws_url: "wss://admin.tesaiot.com/ws/telemetry"
  subscribe_timeout: 60
```

## Dependencies

- typer - CLI framework
- requests - HTTP client
- websocket-client - WebSocket
- rich - Terminal formatting
- PyYAML - Config parsing
- python-dotenv - Environment loading
