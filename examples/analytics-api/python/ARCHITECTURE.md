# Analytics API Python Client - Architecture

## Overview

Python client for TESAIoT Analytics API providing access to anomaly detection, connectivity status, latency statistics, and throughput metrics.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                      Python Application                         │
├─────────────────────────────────────────────────────────────────┤
│  analytics_client.py                                            │
│  ├── AnalyticsClient class                                      │
│  │   ├── get_anomalies()                                        │
│  │   ├── get_connectivity_status()                              │
│  │   ├── get_latency_stats()                                    │
│  │   ├── get_throughput_stats()                                 │
│  │   └── get_fleet_anomalies()                                  │
│  └── Example scripts                                            │
│      ├── example_latency.py                                     │
│      ├── example_throughput.py                                  │
│      └── example_fleet_anomalies.py                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS (TLS 1.2+)
                              │ X-API-KEY header
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform                             │
├─────────────────────────────────────────────────────────────────┤
│  /api/v1/bdh-ai/                                                │
│  ├── anomalies              → Anomaly detection results         │
│  ├── connectivity/status    → Device online/offline status      │
│  ├── connectivity/latency   → Network latency statistics        │
│  ├── connectivity/throughput→ Message throughput metrics        │
│  └── analytics/anomalies    → Fleet-wide anomaly overview       │
└─────────────────────────────────────────────────────────────────┘
```

## Workflow

1. **Initialize Client**: Create `AnalyticsClient` with API base URL and API key
2. **Query Endpoints**: Call methods to retrieve analytics data
3. **Process Response**: Handle JSON responses with error checking
4. **Display/Store**: Output data to console or store for further analysis

## Authentication

```ini
Authorization Flow:
┌────────────┐    X-API-KEY header    ┌─────────────────┐
│   Client   │ ─────────────────────► │  TESAIoT API    │
│            │                        │ (validates key) │
│            │ ◄───────────────────── │                 │
└────────────┘    JSON response       └─────────────────┘
```

- Uses REST API Key (`tesa_ak_*` format)
- Key passed in `X-API-KEY` header
- TLS encryption for all requests

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/bdh-ai/anomalies` | GET | Get anomaly detection results |
| `/api/v1/bdh-ai/connectivity/status` | GET | Get device connectivity status |
| `/api/v1/bdh-ai/connectivity/latency` | GET | Get network latency statistics |
| `/api/v1/bdh-ai/connectivity/throughput` | GET | Get message throughput metrics |
| `/api/v1/bdh-ai/analytics/anomalies` | GET | Get fleet-wide anomaly overview |

## Dependencies

- `requests` - HTTP client library
- `python-dotenv` - Environment variable management

## Files

```ini
analytics-api/python/
├── analytics_client.py        # Main client class
├── example_latency.py         # Latency statistics example
├── example_throughput.py      # Throughput metrics example
├── example_fleet_anomalies.py # Fleet anomalies example
├── requirements.txt           # Python dependencies
├── .env.example              # Environment template
├── README.md                 # Usage documentation
└── ARCHITECTURE.md           # This file
```
