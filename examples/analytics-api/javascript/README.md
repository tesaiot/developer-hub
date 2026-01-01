# JavaScript/Node.js Analytics API Client

A modern JavaScript/TypeScript client for the TESAIoT BDH AI Analytics API.

## Features

- ES6+ async/await support
- TypeScript type definitions
- Browser and Node.js compatible
- Comprehensive error handling
- Response caching (optional)

## Installation

```bash
npm install
```

## Quick Start

```javascript
import { AnalyticsClient } from './analytics-client.js';

const client = new AnalyticsClient({
    baseUrl: 'https://admin.tesaiot.com/api/v1/bdh-ai',
    apiToken: 'your_jwt_token'
});

// Get anomalies
const anomalies = await client.getAnomalies({
    severityFilter: ['critical', 'high'],
    limit: 10
});

// Get device clusters
const clusters = await client.getClusters({
    metricName: 'temperature',
    nClusters: 5
});

// Get AI insights
const insights = await client.getInsights({ days: 7 });

// Get connectivity status
const status = await client.getConnectivityStatus();
```

## Examples

| File | Description |
|------|-------------|
| `analytics-client.js` | Main client library |
| `example-basic.js` | Basic usage examples |
| `example-anomalies.js` | Anomaly detection examples |
| `example-patterns.js` | Pattern recognition examples |
| `example-insights.js` | AI insights examples |
| `example-connectivity.js` | Connectivity monitoring examples |
| `example-dashboard.js` | Complete dashboard example |

## Configuration

Set environment variables or pass to client constructor:

```bash
export TESAIOT_API_URL="https://admin.tesaiot.com/api/v1/bdh-ai"
export TESAIOT_API_TOKEN="your_jwt_token"
```

## Browser Usage

```html
<script type="module">
import { AnalyticsClient } from './analytics-client.js';

const client = new AnalyticsClient({
    apiToken: 'your_jwt_token'
});

const data = await client.getAnomalies();
console.log(data);
</script>
```

## TypeScript Support

TypeScript definitions are included. Import types:

```typescript
import { AnalyticsClient, Anomaly, Cluster, Insight, TimeRange } from './analytics-client';

const anomalies: Anomaly[] = await client.getAnomalies();
```

## License

Apache 2.0
