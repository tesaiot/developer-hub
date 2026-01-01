# TESAIoT BDH AI Analytics API - Python Client

A full-featured Python client for the TESAIoT BDH AI Analytics API.

## Features

- Async/await support with `httpx`
- Type hints and dataclasses
- Comprehensive error handling
- Support for all Analytics API endpoints
- Both async and sync client options

## Prerequisites

| Item | Description |
|------|-------------|
| Python | 3.9+ (3.11 recommended) |
| pip | Package manager |
| API Token | JWT token or API Key from TESAIoT Platform |

## Installation

**macOS:**
```bash
pip install -r requirements.txt
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update && sudo apt install -y python3-pip python3-venv
pip install -r requirements.txt
```

**Raspberry Pi:**
```bash
sudo apt update && sudo apt install -y python3-pip python3-venv
pip install -r requirements.txt
```

**Windows (PowerShell):**
```powershell
pip install -r requirements.txt
```

## Getting API Token from TESAIoT Platform

1. Login to **TESAIoT Admin Portal**: https://admin.tesaiot.com
2. Navigate to **Settings** > **API Keys**
3. Click **Create API Key** with required scopes:
   - `analytics:read`
   - `devices:read`
4. Copy the generated API Key (starts with `tesa_ak_`)

**Alternative: Use JWT Token**
- Copy your session JWT token from browser DevTools
- Token format: `eyJhbGciOiJIUzI1NiIs...`

## Configuration

### Option 1: Environment Variables

```bash
# Copy example config
cp .env_example .env

# Edit with your credentials
nano .env
```

Set your token in `.env`:
```env
TESAIOT_API_URL=https://admin.tesaiot.com/api/v1/bdh-ai
TESAIOT_API_TOKEN=tesa_ak_your_api_key_here
```

### Option 2: Pass to Client

```python
from analytics_client import AnalyticsClient

client = AnalyticsClient(
    base_url="https://admin.tesaiot.com/api/v1/bdh-ai",
    api_token="tesa_ak_your_api_key_here"
)
```

## Quick Start

```python
import asyncio
from analytics_client import AnalyticsClient

async def main():
    async with AnalyticsClient() as client:
        # Get anomalies
        anomalies = await client.get_anomalies(limit=10)
        print(f"Found {len(anomalies.get('data', []))} anomalies")

        # Get connectivity status
        status = await client.get_connectivity_status()
        print(f"Online devices: {status['summary']['online_count']}")

        # Get latency statistics
        latency = await client.get_latency_stats(hours=24)
        print(f"Avg latency: {latency['summary']['overall_avg_ms']}ms")

asyncio.run(main())
```

## Available Examples

| File | Description | API Endpoint |
|------|-------------|--------------|
| `example_basic.py` | Basic usage examples | Multiple |
| `example_anomalies.py` | Anomaly detection | `/analytics/anomalies` |
| `example_patterns.py` | Pattern recognition | `/patterns/clusters` |
| `example_insights.py` | AI insights | `/insights` |
| `example_connectivity.py` | Device connectivity | `/connectivity/status` |
| `example_latency.py` | Network latency | `/connectivity/latency` |
| `example_throughput.py` | Message throughput | `/connectivity/throughput` |
| `example_fleet_anomalies.py` | Fleet anomalies | `/analytics/anomalies` |
| `example_dashboard.py` | Complete dashboard | Multiple |

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/analytics/anomalies` | GET | Get detected anomalies |
| `/connectivity/status` | GET | Device connectivity status |
| `/connectivity/latency` | GET | Network latency statistics |
| `/connectivity/throughput` | GET | Message throughput stats |
| `/patterns/clusters` | POST | K-means clustering |
| `/patterns/outliers` | POST | Outlier detection |
| `/insights` | POST | AI-generated insights |

## Synchronous Client

For non-async code, use `AnalyticsClientSync`:

```python
from analytics_client import AnalyticsClientSync

client = AnalyticsClientSync()
anomalies = client.get_anomalies(limit=10)
client.close()
```

## Troubleshooting

### 401 Unauthorized
- Check your API token is valid
- Ensure token has required scopes

### 404 Not Found
- Verify the endpoint path is correct
- Check API base URL

### Connection Error
- Ensure internet connectivity
- Verify TLS/SSL is working

## Security Notes

- **Never commit `.env` file** - it contains your secret token
- **Use `.env_example`** as template (no real credentials)
- **Rotate tokens regularly**

## License

Apache 2.0
