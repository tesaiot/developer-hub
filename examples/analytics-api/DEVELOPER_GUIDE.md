# TESAIoT Analytics API Developer Guide

> Comprehensive guide for integrating with the TESAIoT BDH AI Analytics API

## Overview

The TESAIoT Analytics API provides four powerful analytics domains for IoT device management:

1. **Anomaly Detection** - Real-time anomaly monitoring across your device fleet
2. **Pattern Recognition** - K-means clustering of device behavior patterns
3. **AI Insights** - AI-powered recommendations and trend analysis
4. **Connectivity Metrics** - MQTT-based device connectivity monitoring

### Background Anomaly Detection

The system includes a **background anomaly detection worker** that automatically:
- Scans TimescaleDB telemetry every **5 minutes**
- Uses **S-H-ESD algorithm** (Seasonal Hybrid Extreme Studentized Deviate)
- Stores detected anomalies in **MongoDB** for fast queries
- Supports configurable lookback window (default: 24 hours)
- Assigns severity levels: critical, high, medium, low

**Configuration (Environment Variables):**
```bash
BACKGROUND_WORKER_ENABLED=true          # Enable/disable worker
ANOMALY_SCAN_INTERVAL_SECS=300          # Scan every 5 minutes
ANOMALY_LOOKBACK_MINUTES=1440           # 24-hour lookback
ANOMALY_MIN_DATA_POINTS=10              # Min points for detection
ANOMALY_METRICS=temperature,humidity,pressure,heart_rate,spo2,glucose
```

## Quick Start

### Authentication

All API requests require a JWT token in the Authorization header:

```http
Authorization: Bearer <your_jwt_token>
```

Get your API token from the TESAIoT Admin Portal under **Settings > API Tokens**.

### Base URL

```
https://admin.tesaiot.com/api/v1/bdh-ai
```

### First API Call

```bash
curl -X POST https://admin.tesaiot.com/api/v1/bdh-ai/analytics/anomalies \
  -H "Authorization: Bearer $TESAIOT_API_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "time_range": {
      "start": "2025-12-22T00:00:00Z",
      "end": "2025-12-29T23:59:59Z"
    },
    "severity_filter": ["critical", "high"],
    "limit": 10
  }'
```

---

## API Reference

### Anomaly Detection

#### GET Anomalies

```http
POST /analytics/anomalies
```

Retrieve aggregated anomalies across your device fleet.

**Request Body:**
```json
{
  "time_range": {
    "start": "2025-12-22T00:00:00Z",
    "end": "2025-12-29T23:59:59Z"
  },
  "severity_filter": ["critical", "high", "medium", "low"],
  "device_ids": ["device-001", "device-002"],
  "limit": 100,
  "offset": 0
}
```

**Response:**
```json
{
  "anomalies": [
    {
      "id": "anom_abc123",
      "device_id": "device-001",
      "device_name": "Temperature Sensor A",
      "metric": "temperature",
      "value": 85.5,
      "severity": "high",
      "score": 0.92,
      "timestamp": "2025-12-29T10:30:00Z",
      "acknowledged": false,
      "resolved": false
    }
  ],
  "summary": {
    "total": 150,
    "by_severity": {
      "critical": 5,
      "high": 25,
      "medium": 80,
      "low": 40
    },
    "by_metric": {
      "temperature": 100,
      "humidity": 30,
      "pressure": 20
    }
  }
}
```

#### GET Anomaly Timeline

```http
POST /analytics/anomalies/timeline
```

Get anomaly trends over time for visualization.

**Request Body:**
```json
{
  "days": 30,
  "group_by": "severity"
}
```

**Response:**
```json
{
  "timeline": [
    {
      "date": "2025-12-01",
      "counts": {
        "critical": 2,
        "high": 10,
        "medium": 25,
        "low": 15
      }
    }
  ],
  "trend": {
    "direction": "decreasing",
    "percent_change": -15.5
  }
}
```

#### GET Anomaly Heatmap

```http
POST /analytics/anomalies/heatmap
```

Get device × time heatmap data for visualization.

**Request Body:**
```json
{
  "time_range": {
    "start": "2025-12-22T00:00:00Z",
    "end": "2025-12-29T23:59:59Z"
  },
  "resolution": "hour",
  "max_devices": 50
}
```

---

### Pattern Recognition

#### GET Clusters

```http
POST /patterns/clusters
```

K-means clustering of device behavior patterns.

**Request Body:**
```json
{
  "metric_name": "temperature",
  "n_clusters": 5,
  "time_range": {
    "start": "2025-12-22T00:00:00Z",
    "end": "2025-12-29T23:59:59Z"
  },
  "include_outliers": true
}
```

**Response:**
```json
{
  "clusters": [
    {
      "cluster_id": 0,
      "cluster_name": "Normal Operating Range",
      "device_count": 45,
      "characteristics": {
        "mean": 24.5,
        "std_dev": 2.1,
        "trend_slope": 0.001,
        "periodicity": 0.95,
        "entropy": 0.35
      },
      "devices": ["device-001", "device-002"]
    }
  ],
  "silhouette_score": 0.72,
  "outliers": [
    {
      "device_id": "device-050",
      "outlier_score": 0.85,
      "reason": "Abnormal variance pattern"
    }
  ]
}
```

#### Find Similar Devices

```http
POST /patterns/similar
```

Find devices with similar behavior patterns.

**Request Body:**
```json
{
  "device_id": "device-001",
  "metric_name": "temperature",
  "top_k": 10
}
```

---

### AI Insights

#### Generate Insights

```http
POST /insights
```

Generate AI-powered insights and recommendations.

**Request Body:**
```json
{
  "analysis_period_days": 7,
  "insight_types": ["anomaly_pattern", "trend", "correlation", "recommendation"],
  "min_confidence": 0.7
}
```

**Response:**
```json
{
  "fleet_summary": {
    "total_devices": 100,
    "active_devices": 95,
    "anomaly_rate": 0.05,
    "health_score": 87.5
  },
  "insights": [
    {
      "id": "ins_xyz789",
      "type": "trend",
      "severity": "warning",
      "title": "Temperature Sensors Trending Higher",
      "description": "5% increase in average temperature readings detected",
      "confidence": 0.85,
      "actionable": true,
      "recommended_actions": [
        "Check HVAC system calibration",
        "Review sensor placement"
      ]
    }
  ]
}
```

#### Get Recommendations

```http
GET /insights/recommendations?priority=high&limit=10
```

---

### Connectivity Metrics

> **Note:** Connectivity endpoints use POST method for organization context.

#### Device Status

```http
POST /connectivity/status
```

Get real-time device connectivity status.

**Request Body:**
```json
{
  "organization_id": "your-org-id"
}
```

**Response:**
```json
{
  "summary": {
    "total": 100,
    "online": 95,
    "offline": 5
  },
  "devices": [
    {
      "device_id": "device-001",
      "device_name": "Temperature Sensor A",
      "status": "online",
      "last_seen": "2025-12-29T12:00:00Z",
      "uptime_percent": 99.5
    }
  ]
}
```

#### Latency Statistics

```http
POST /connectivity/latency
```

**Request Body:**
```json
{
  "organization_id": "your-org-id",
  "hours": 24
}
```

**Response:**
```json
{
  "summary": {
    "avg_latency_ms": 45.2,
    "median_latency_ms": 38.0,
    "p95_latency_ms": 120.5,
    "p99_latency_ms": 250.0
  }
}
```

#### Throughput Statistics

```http
POST /connectivity/throughput
```

**Request Body:**
```json
{
  "organization_id": "your-org-id",
  "hours": 24
}
```

**Response:**
```json
{
  "summary": {
    "total_messages": 1250000,
    "avg_per_hour": 52000,
    "peak_per_hour": 85000,
    "peak_time": "2025-12-29T14:00:00Z"
  }
}
```

---

## Client Libraries

### Python

```bash
pip install httpx pydantic
```

```python
from analytics_client import AnalyticsClient

async with AnalyticsClient(api_token="your_token") as client:
    # Get anomalies
    anomalies = await client.get_anomalies(
        severity_filter=["critical", "high"],
        limit=10
    )

    # Get clusters
    clusters = await client.get_clusters(
        metric_name="temperature",
        n_clusters=5
    )

    # Get insights
    insights = await client.get_insights(days=7)
```

### JavaScript/Node.js

```javascript
import { AnalyticsClient } from './analytics-client.js';

const client = new AnalyticsClient({
    apiToken: 'your_token'
});

// Get anomalies
const anomalies = await client.getAnomalies({
    severityFilter: ['critical', 'high'],
    limit: 10
});

// Get clusters
const clusters = await client.getClusters({
    metricName: 'temperature',
    nClusters: 5
});
```

### Rust

```rust
use tesaiot_analytics::{AnalyticsClient, TimeRange};

let client = AnalyticsClient::from_env()?;

// Get anomalies
let anomalies = client.get_anomalies(
    None,
    Some(vec!["critical", "high"]),
    None,
    10,
    0
).await?;

// Get clusters
let clusters = client.get_clusters(
    "temperature",
    5,
    None,
    true
).await?;
```

---

## Building a Dashboard

### Step 1: Collect Data

```python
async def collect_dashboard_data(client):
    # Collect all data in parallel
    anomalies, clusters, insights, connectivity = await asyncio.gather(
        client.get_anomalies(limit=100),
        client.get_clusters(metric_name="temperature", n_clusters=5),
        client.get_insights(days=7),
        client.get_connectivity_status()
    )

    return {
        "anomalies": anomalies,
        "clusters": clusters,
        "insights": insights,
        "connectivity": connectivity
    }
```

### Step 2: Calculate Health Score

```python
def calculate_health_score(data):
    # Anomaly score (lower is better)
    anomaly_rate = data["anomalies"]["summary"]["total"] / 100
    anomaly_score = max(0, 100 - anomaly_rate * 1000)

    # Connectivity score
    conn = data["connectivity"]["summary"]
    connectivity_score = (conn["online"] / conn["total"]) * 100

    # Weighted overall
    overall = anomaly_score * 0.5 + connectivity_score * 0.5

    return {
        "overall": overall,
        "status": "EXCELLENT" if overall >= 90 else "GOOD" if overall >= 70 else "FAIR"
    }
```

### Step 3: Generate Alerts

```python
def generate_alerts(data):
    alerts = []

    # Critical anomalies
    critical = data["anomalies"]["summary"]["by_severity"].get("critical", 0)
    if critical > 0:
        alerts.append({
            "level": "critical",
            "title": f"{critical} Critical Anomalies",
            "action": "Investigate immediately"
        })

    # Offline devices
    offline = data["connectivity"]["summary"]["offline"]
    if offline > 0:
        alerts.append({
            "level": "warning",
            "title": f"{offline} Devices Offline",
            "action": "Check network connectivity"
        })

    return alerts
```

---

## Best Practices

### Rate Limiting

The API enforces the following rate limits:

| Endpoint Category | Limit |
|------------------|-------|
| Anomaly queries | 100 req/min |
| Pattern analysis | 30 req/min |
| Insight generation | 10 req/min |
| Connectivity status | 120 req/min |

### Caching

- Cache anomaly summaries for 5 minutes
- Cache cluster results for 15 minutes (computationally expensive)
- Do not cache connectivity status (real-time data)

### Error Handling

```python
try:
    result = await client.get_anomalies()
except httpx.HTTPStatusError as e:
    if e.response.status_code == 401:
        # Token expired, refresh
        pass
    elif e.response.status_code == 429:
        # Rate limited, back off
        await asyncio.sleep(60)
    else:
        # Log and alert
        logger.error(f"API error: {e}")
```

### Time Range Optimization

- Use the smallest time range necessary
- For dashboards, use 7-day default
- For trend analysis, use 30 days
- Avoid queries spanning more than 90 days

---

## Support

- **Documentation:** https://tesaiot.github.io/developer-hub/
- **GitHub Issues:** https://github.com/tesaiot/analytics-client/issues
- **API Status:** https://status.tesaiot.com

## License

Apache 2.0 - See LICENSE file for details.
