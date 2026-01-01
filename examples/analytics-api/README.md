# BDH AI Analytics API Examples

This directory contains example code for integrating with the TESAIoT BDH AI Analytics API.

## Overview

The BDH AI Analytics API provides comprehensive analytics capabilities:

- **Anomaly Detection** - Fleet-wide anomaly monitoring and visualization
- **Pattern Recognition** - K-means clustering of device behaviors
- **Insights Generation** - AI-powered recommendations and trends
- **Connectivity Metrics** - Real-time device status from EMQX

## API Base URL

```
Production: https://admin.tesaiot.com/api/v1/bdh-ai
Development: http://localhost:8000/api/v1/bdh-ai
```

## Authentication

All API requests require JWT authentication:

```
Authorization: Bearer <your_jwt_token>
```

## Examples by Language

| Language | Directory | Description |
|----------|-----------|-------------|
| Python | [python/](./python/) | Full-featured client with async support |
| JavaScript | [javascript/](./javascript/) | Node.js and browser examples |
| Rust | [rust/](./rust/) | High-performance native client |

## Quick Start

### Python
```bash
cd python
pip install -r requirements.txt
python example_basic.py
```

### JavaScript (Node.js)
```bash
cd javascript
npm install
node example_basic.js
```

### Rust
```bash
cd rust
cargo run --example basic
```

## API Endpoints

### Anomaly Detection

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/fleet/anomalies` | Get aggregated anomalies from MongoDB |
| POST | `/fleet/timeline` | Get anomaly trend timeline |
| POST | `/fleet/heatmap` | Get device×time heatmap |

> **Note:** Anomalies are automatically detected by a background worker and stored in MongoDB.

### Pattern Recognition

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/patterns/clusters` | Get device clusters (K-means) |
| POST | `/patterns/similar` | Find similar devices |

> **Note:** Pattern endpoints require `organization_id` and `metric_name` in request body.

### Insights

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/insights/generate` | Generate AI insights |
| POST | `/insights/recommendations` | Get recommendations |

### Connectivity

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/connectivity/status` | Get device status |
| POST | `/connectivity/latency` | Get latency stats |
| POST | `/connectivity/throughput` | Get throughput metrics |

> **Note:** All connectivity endpoints require `organization_id` in request body.

## Response Format

All API responses follow this structure:

```json
{
  "data": { ... },
  "metadata": {
    "timestamp": "2025-12-29T15:30:00Z",
    "processing_time_ms": 45
  }
}
```

Error responses:

```json
{
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable message",
    "details": { ... }
  }
}
```

## Rate Limits

| Endpoint Category | Limit |
|-------------------|-------|
| Anomaly APIs | 60 req/min |
| Pattern APIs | 20 req/min |
| Insights APIs | 10 req/min |
| Connectivity APIs | 120 req/min |

## Support

- Documentation: https://tesaiot.github.io/developer-hub/
- Issues: https://github.com/tesaiot/tesa-iot-platform/issues
- API Reference: [SPEC.md](../../../../TESA_Rules/ALL_PLAN/v2026.01/BDH_AI_Analytics_Menu/SPEC.md)

## License

Apache 2.0 - See [LICENSE](../../LICENSE) for details.
