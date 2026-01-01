# Grafana Dashboards for TESAIoT

Pre-built Grafana dashboards for visualizing IoT telemetry data.

---

## Overview

These dashboards provide real-time visualization of:
- Device telemetry (temperature, humidity, pressure)
- AI inference results (anomaly detection, predictions)
- System health metrics
- Device fleet overview

## Prerequisites

### Grafana Installation

```bash
# Docker (recommended)
docker run -d --name grafana \
  -p 3000:3000 \
  grafana/grafana

# apt (Ubuntu/Debian)
sudo apt-get install -y grafana
sudo systemctl start grafana-server
```

Default login: `admin` / `admin`

### Data Source Configuration

These dashboards work with:
- **TimescaleDB** (recommended for time-series)
- **PostgreSQL** (via telemetry API)
- **InfluxDB** (optional)

## Available Dashboards

| Dashboard | Description | Data Source |
|-----------|-------------|-------------|
| [device-overview](./dashboards/device-overview.json) | Fleet status and health | TimescaleDB |
| [telemetry-details](./dashboards/telemetry-details.json) | Per-device metrics | TimescaleDB |
| [ai-inference](./dashboards/ai-inference.json) | Anomaly detection | TimescaleDB |
| [system-health](./dashboards/system-health.json) | Platform metrics | Prometheus |

## Quick Start

### 1. Add Data Source

1. Open Grafana at `http://localhost:3000`
2. Navigate to **Configuration** → **Data Sources**
3. Add **PostgreSQL** or **TimescaleDB**:
   - Host: `your-timescaledb-host:5432`
   - Database: `tesaiot`
   - User: `grafana_reader`
   - SSL Mode: `require`

### 2. Import Dashboard

1. Navigate to **Dashboards** → **Import**
2. Upload JSON from `./dashboards/`
3. Select your data source
4. Click **Import**

### 3. Configure Variables

Each dashboard uses template variables:
- `$device_id` - Filter by device
- `$time_range` - Time window
- `$organization_id` - Multi-tenant filter

## Dashboard Details

### Device Overview

Bird's-eye view of your IoT fleet:
- Total devices by status (online/offline)
- Latest telemetry per device
- Geographic distribution map
- Alert summary

**Panels:**
- Device Status Pie Chart
- Latest Readings Table
- Device Map (requires Geomap plugin)
- Active Alerts List

### Telemetry Details

Deep-dive into single device metrics:
- Real-time sensor values
- Historical trends
- Rate of change
- Min/Max/Average stats

**Panels:**
- Temperature Time Series
- Humidity Gauge
- Multi-Metric Graph
- Statistical Summary

### AI Inference

Anomaly detection visualization:
- Anomaly scores over time
- Prediction confidence
- Feature importance
- Model performance metrics

**Panels:**
- Anomaly Score Timeline
- Confidence Gauge
- Prediction Accuracy
- Alert Threshold Markers

### System Health

Platform monitoring:
- API response times
- MQTT message rates
- Database performance
- Container health

**Panels:**
- Request Latency P99
- Messages/Second
- Query Duration
- Container Status

## Data Source Setup

### TimescaleDB Connection

```yaml
# Grafana data source config
name: TESAIoT TimescaleDB
type: postgres
url: timescaledb.local:5432
database: tesaiot
user: grafana_reader
secureJsonData:
  password: your-password
jsonData:
  sslmode: require
  maxOpenConns: 10
  maxIdleConns: 5
  connMaxLifetime: 14400
  postgresVersion: 1400
  timescaledb: true
```

### Read-Only User Setup

```sql
-- Create Grafana read-only user
CREATE USER grafana_reader WITH PASSWORD 'secure-password';
GRANT CONNECT ON DATABASE tesaiot TO grafana_reader;
GRANT USAGE ON SCHEMA public TO grafana_reader;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO grafana_reader;
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT SELECT ON TABLES TO grafana_reader;
```

## Docker Compose

Full stack with Grafana, TimescaleDB, and provisioning:

```yaml
version: '3.8'
services:
  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=changeme
      - GF_INSTALL_PLUGINS=grafana-worldmap-panel
    volumes:
      - grafana_data:/var/lib/grafana
      - ./provisioning:/etc/grafana/provisioning
      - ./dashboards:/var/lib/grafana/dashboards

  timescaledb:
    image: timescale/timescaledb:latest-pg15
    ports:
      - "5432:5432"
    environment:
      - POSTGRES_PASSWORD=changeme
    volumes:
      - timescale_data:/var/lib/postgresql/data

volumes:
  grafana_data:
  timescale_data:
```

## Customization

### Adding Custom Panels

1. Edit dashboard JSON
2. Add panel configuration
3. Re-import dashboard

### Alert Rules

Configure alerting for anomaly thresholds:

```yaml
# Example alert rule
- alert: HighTemperature
  expr: temperature > 40
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "High temperature on {{ $labels.device_id }}"
```

## Troubleshooting

### No Data Displayed
- Verify data source connection
- Check time range selector
- Confirm device_id variable

### Slow Dashboard Load
- Add time indexes to queries
- Reduce data points with aggregation
- Use Grafana caching

### Permission Denied
- Check PostgreSQL user grants
- Verify SSL configuration

---

**Category:** Visualization
**Last Updated:** 2025-12-27
