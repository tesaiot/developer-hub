# Integrations Examples

Examples for PaaS/SaaS/AIaaS developers integrating with TESAIoT Platform.

## Target Audience

| Developer Type | Use Case | Integration Point |
|----------------|----------|-------------------|
| **PaaS Provider** | Multi-tenant IoT hosting | MQTT Broker |
| **SaaS Provider** | IoT-enabled applications | REST API + MQTT |
| **AIaaS Provider** | ML inference services | REST API + MQTT |
| **Data Platform** | Data lake/warehouse | MQTT → Database |

## Categories

```
├── mqtt-integration/       # MQTT Data Ingestion
│   ├── mqtt-to-database    # TimescaleDB/InfluxDB
│   └── mqtt-quic-c         # High-performance C client
│
├── ai-services/            # AI/ML Integration
│   ├── ai-service-template # FastAPI inference
│   └── flowise-ai-agent    # LLM + IoT
│
├── workflow-automation/    # Workflow Tools
│   ├── n8n-automation      # n8n workflows
│   └── webhook-incident    # PagerDuty/OpsGenie
│
└── enterprise/             # Enterprise Integration
    └── kafka-bridge        # MQTT → Kafka
```

## Quick Start

### MQTT Data Ingestion

1. **Subscribe to telemetry** using WSS MQTT
2. **Store in TimescaleDB** using [mqtt-to-database](./mqtt-integration/mqtt-to-database/)

### AI Integration

1. **[ai-service-template](./ai-services/ai-service-template/)** - FastAPI ML service
2. Deploy as Docker container

---

## Examples

### MQTT Integration

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [mqtt-to-database](./mqtt-integration/mqtt-to-database/) | Python | MQTT → TimescaleDB/InfluxDB | NEW |
| [mqtt-quic-c](./mqtt-integration/mqtt-quic-c/) | C | High-performance QUIC client | Ready |

### AI Services

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [ai-service-template](./ai-services/ai-service-template/) | Python, FastAPI | ML inference microservice | Ready |
| [flowise-ai-agent](./ai-services/flowise-ai-agent/) | JSON | LLM + IoT data analysis | NEW |

### Workflow Automation

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [n8n-automation](./workflow-automation/n8n-automation/) | JSON | Pre-built n8n workflows | NEW |
| [webhook-incident](./workflow-automation/webhook-incident/) | Python | Alert → PagerDuty/OpsGenie | Planned |

### Enterprise

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [kafka-bridge](./enterprise/kafka-bridge/) | Go | MQTT → Kafka bridge | Future |

---

## Key Skills

- MQTT broker integration
- Data pipeline construction
- AI/ML service development
- Event-driven automation
- Enterprise integration patterns

---

## Use Cases

### Data Pipeline
```
Device → EMQX → mqtt-to-database → TimescaleDB → Grafana
```

### AI Inference
```
Telemetry → ai-service-template → Anomaly Score → Alert
```

### Automation
```
Anomaly → n8n → Slack/Email/PagerDuty
```

---

**Category:** Integrations
**Last Updated:** 2025-12-27
