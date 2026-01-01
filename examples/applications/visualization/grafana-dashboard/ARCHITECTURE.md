# Grafana Dashboard Architecture

## Overview

Pre-configured Grafana dashboards for visualizing TESAIoT telemetry stored in TimescaleDB.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Grafana                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Dashboard View                        │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐      │   │
│  │  │Time     │  │Gauge    │  │Table    │  │Alert    │      │   │
│  │  │Series   │  │Panel    │  │Panel    │  │Panel    │      │   │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                               │                                 │
│                               ▼                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Data Source                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  PostgreSQL (TimescaleDB)                           │ │   │
│  │  │  URL: timescaledb:5432                              │ │   │
│  │  │  Database: tesaiot                                  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │     TimescaleDB      │
                    │  ┌────────────────┐  │
                    │  │ telemetry_data │  │
                    │  │ (hypertable)   │  │
                    │  └────────────────┘  │
                    └──────────────────────┘
```

## Data Flow

```
┌──────────┐    ┌─────────────┐    ┌───────────────┐    ┌─────────────┐
│  Device  │───▶│ EMQX Broker │───▶│  MQTT Bridge  │───▶│ TimescaleDB │
│  (MQTTS) │    │             │    │               │    │             │
└──────────┘    └─────────────┘    └───────────────┘    └──────┬──────┘
                                                               │
                                                               │
                                   ┌───────────────────────────┘
                                   │
                                   ▼
┌──────────┐    ┌─────────────────────────────────────┐
│  User    │◀───│              Grafana                │
│ (Browser)│    │  ┌────────────────────────────────┐ │
│          │    │  │ SQL Query (time_bucket)        │ │
│          │    │  └────────────────────────────────┘ │
└──────────┘    └─────────────────────────────────────┘
```

## Directory Structure

```
grafana-dashboard/
├── dashboards/
│   ├── device-overview.json     # Fleet overview
│   └── telemetry-details.json   # Single device
├── datasources/
│   └── timescaledb.yaml         # Data source config
└── README.md
```

## Dashboard: Device Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Device Overview Dashboard                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │  Total Devices  │  │  Online Now     │  │  Offline        │  │
│  │      156        │  │      142        │  │       14        │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                  Telemetry Over Time                      │  │
│  │  ▲                                                        │  │
│  │  │    ╭──╮    ╭──╮                                        │  │
│  │  │   ╱    ╲  ╱    ╲  ╱                                    │  │
│  │  │  ╱      ╲╱      ╲╱                                     │  │
│  │  └────────────────────────────────────────────────────▶   │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Device        │ Temperature │ Humidity │ Last Seen       │  │
│  │────────────────┼─────────────┼──────────┼─────────────────│  │
│  │  sensor-001    │    25.5°C   │   60%    │ 2 min ago       │  │
│  │  sensor-002    │    24.8°C   │   55%    │ 1 min ago       │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## SQL Queries

```sql
-- Device count by status
SELECT
  status,
  COUNT(*) as count
FROM devices
GROUP BY status;

-- Telemetry time series (using TimescaleDB time_bucket)
SELECT
  time_bucket('5 minutes', timestamp) AS time,
  device_id,
  AVG((data->>'temperature')::float) AS temperature
FROM telemetry_data
WHERE timestamp > NOW() - INTERVAL '24 hours'
GROUP BY time, device_id
ORDER BY time;
```

## Importing Dashboards

```bash
# Using Grafana API
curl -X POST \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $GRAFANA_API_KEY" \
  -d @dashboards/device-overview.json \
  http://localhost:3000/api/dashboards/db

# Using Grafana UI
1. Go to Dashboards > Import
2. Upload JSON file or paste content
3. Select TimescaleDB data source
4. Click Import
```

## Data Source Config

```yaml
# datasources/timescaledb.yaml
apiVersion: 1
datasources:
  - name: TESAIoT TimescaleDB
    type: postgres
    url: timescaledb:5432
    database: tesaiot
    user: grafana_reader
    jsonData:
      sslmode: require
      timescaledb: true
```

## Alert Rules

| Alert | Condition | Action |
|-------|-----------|--------|
| High Temperature | temp > 35°C | Slack notification |
| Device Offline | No data 10 min | Email alert |
| Low Battery | battery < 20% | Dashboard warning |
