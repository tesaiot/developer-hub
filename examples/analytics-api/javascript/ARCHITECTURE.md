# Analytics API JavaScript Client - Architecture

## Overview

Node.js/JavaScript client for TESAIoT Analytics API providing access to anomaly detection, connectivity status, latency statistics, and throughput metrics.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                    Node.js Application                          │
├─────────────────────────────────────────────────────────────────┤
│  index.js                                                       │
│  ├── AnalyticsClient class                                      │
│  │   ├── getAnomalies()                                         │
│  │   ├── getConnectivityStatus()                                │
│  │   ├── getLatencyStats()                                      │
│  │   ├── getThroughputStats()                                   │
│  │   └── getFleetAnomalies()                                    │
│  └── async/await pattern for API calls                          │
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

```ini
┌──────────┐     ┌───────────────┐     ┌─────────────┐
│  Start   │────►│ Load .env     │────►│ Create      │
│          │     │ API_KEY       │     │ Client      │
└──────────┘     └───────────────┘     └──────┬──────┘
                                              │
                 ┌────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────────┐
│              Query Analytics Endpoints               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐ │
│  │Anomalies│ │Latency  │ │Through- │ │Connectivity │ │
│  │         │ │Stats    │ │put      │ │Status       │ │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────┘ │
└──────────────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│              Process & Display Results               │
└──────────────────────────────────────────────────────┘
```

## Authentication

- Uses REST API Key (`tesa_ak_*` format)
- Key stored in `.env` file
- Passed via `X-API-KEY` header
- All requests use HTTPS

## Dependencies

- `axios` or `node-fetch` - HTTP client
- `dotenv` - Environment variable management

## Files

```ini
analytics-api/javascript/
├── index.js              # Main client implementation
├── package.json          # Node.js dependencies
├── .env.example          # Environment template
├── README.md             # Usage documentation
└── ARCHITECTURE.md       # This file
```

## Usage

```javascript
const client = new AnalyticsClient(apiKey);
const anomalies = await client.getAnomalies();
const latency = await client.getLatencyStats();
```
