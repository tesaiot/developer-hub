# Raspberry Pi Server TLS Architecture

## Overview

Python-based MQTT client for Raspberry Pi using Server TLS (one-way TLS) with API key authentication.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      Raspberry Pi Device                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐                     │
│  │    main.py      │───▶│  mqtt_client.py │                     │
│  │  (Entry Point)  │    │  (Paho MQTT)    │                     │
│  └────────┬────────┘    └────────┬────────┘                     │
│           │                      │                              │
│           ▼                      │                              │
│  ┌─────────────────┐             │                              │
│  │ sensor_simulator│             │                              │
│  │   (Test Data)   │─────────────┤                              │
│  └─────────────────┘             │                              │
│                                  │                              │
│  ┌─────────────────┐             │                              │
│  │ https_client.py │             │                              │
│  │  (REST API)     │─────┐       │                              │
│  └─────────────────┘     │       │                              │
│                          │       │                              │
└──────────────────────────┼───────┼──────────────────────────────┘
                           │       │
                           │       │  MQTTS + API Key
                           │       │  (Port 8883)
                           │       ▼
┌──────────────────────────┼───────────────────────────────────────┐
│                          │       TESAIoT Platform                │
│                          │                                       │
│  ┌─────────────────┐     │     ┌─────────────────┐               │
│  │   REST API      │◀────┘     │   EMQX Broker   │               │
│  │  (HTTPS :443)   │           │  (MQTTS :8883)  │               │
│  └────────┬────────┘           └────────┬────────┘               │
│           │                             │                        │
│           └──────────────┬──────────────┘                        │
│                          ▼                                       │
│                 ┌─────────────────┐                              │
│                 │   TimescaleDB   │                              │
│                 │  (Time-series)  │                              │
│                 └─────────────────┘                              │
└──────────────────────────────────────────────────────────────────┘
```

## Data Flow

```
Sensor Data Generation:
┌────────────┐    ┌────────────┐    ┌────────────┐    ┌────────────┐
│  Sensor    │───▶│   JSON     │───▶│   MQTT     │───▶│   Broker   │
│ Simulator  │    │  Payload   │    │  Publish   │    │  (EMQX)    │
└────────────┘    └────────────┘    └────────────┘    └────────────┘

Platform Storage:
┌────────────┐    ┌────────────┐    ┌────────────┐
│   Broker   │───▶│   Bridge   │───▶│ TimescaleDB│
│   (EMQX)   │    │  (Python)  │    │            │
└────────────┘    └────────────┘    └────────────┘
```

## TLS Connection

```
┌──────────────────┐          ┌──────────────────┐
│   Raspberry Pi   │          │   EMQX Broker    │
│                  │          │                  │
│  ┌────────────┐  │   TLS    │  ┌────────────┐  │
│  │ MQTT Client│──┼──────────┼─▶│TLS Endpoint│  │
│  └────────────┘  │ Handshake│  └────────────┘  │
│        │         │          │        │         │
│        ▼         │          │        ▼         │
│  ┌────────────┐  │          │  ┌────────────┐  │
│  │ CA Cert    │  │ Verify   │  │ Server Cert│  │
│  │ (ca.crt)   │──┼─────────▶│  │            │  │
│  └────────────┘  │ Server   │  └────────────┘  │
└──────────────────┘          └──────────────────┘

Authentication:
  Username: device_id
  Password: api_key
```

## Key Components

| Component | Description | File |
|-----------|-------------|------|
| Main Entry | Application orchestrator | `main.py` |
| MQTT Client | Paho MQTT with TLS | `mqtt_client.py` |
| HTTPS Client | REST API integration | `https_client.py` |
| Sensor Simulator | Test data generator | `sensor_simulator.py` |

## MQTT Topics

| Topic Pattern | Direction | Description |
|---------------|-----------|-------------|
| `device/{id}/telemetry/{type}` | Publish | Sensor data |
| `device/{id}/commands/#` | Subscribe | Commands |
| `device/{id}/status` | Publish | Device status |

## Configuration

```bash
# .env file
TESAIOT_DEVICE_ID=rpi-sensor-001
TESAIOT_API_KEY=tesa_mqtt_org_key_xxx
TESAIOT_MQTT_HOST=mqtt.tesaiot.com
TESAIOT_MQTT_PORT=8883
```

## NCSA Compliance

| Level | Requirement | Implementation |
|-------|-------------|----------------|
| Level 1 | Encrypted transport | TLS 1.2+ (MQTTS) |
| Level 1 | Authentication | API Key validation |
| Level 2 | Server verification | CA certificate |
