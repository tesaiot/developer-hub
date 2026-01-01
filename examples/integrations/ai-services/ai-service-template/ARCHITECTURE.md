# AI Service Template Architecture

## Overview

FastAPI-based microservice template for deploying AI/ML inference as a service with TESAIoT Platform integration.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    AI Service Container                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                     FastAPI App                          │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  /health   │  /predict   │  /batch   │  /models     │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────┬────────────────────────────────┘   │
│                            │                                    │
│              ┌─────────────┴──────────────┐                     │
│              │                            │                     │
│              ▼                            ▼                     │
│  ┌─────────────────────┐       ┌─────────────────────┐          │
│  │   Inference Engine  │       │   Model Manager     │          │
│  │   (sklearn/pytorch) │       │   (load/reload)     │          │
│  └─────────────────────┘       └─────────────────────┘          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
              │                              │
              │  MQTT (Subscribe)            │  MQTT (Publish)
              ▼                              ▼
       ┌─────────────┐              ┌─────────────┐
       │ telemetry/* │              │ inference/* │
       └─────────────┘              └─────────────┘
              │                              ▲
              └──────────────────────────────┘
                        EMQX Broker
```

## Directory Structure

```
ai-service-template/
├── app/
│   ├── main.py              # FastAPI application
│   ├── config.py            # Configuration
│   ├── models/
│   │   ├── base.py          # Model interface
│   │   └── anomaly.py       # Anomaly detection
│   ├── routes/
│   │   ├── health.py        # Health check
│   │   ├── predict.py       # Inference endpoint
│   │   └── models.py        # Model management
│   └── mqtt/
│       ├── client.py        # MQTT client
│       └── handlers.py      # Message handlers
├── models/                   # Saved model files
├── Dockerfile
├── docker-compose.yml
└── requirements.txt
```

## Data Flow

```
                    Real-time Inference Pipeline

┌──────────┐    ┌─────────────┐    ┌───────────────┐    ┌─────────────┐
│  Device  │───▶│ EMQX Broker │───▶│  AI Service   │───▶│ EMQX Broker │
│          │    │ telemetry/* │    │  (inference)  │    │ inference/* │
└──────────┘    └─────────────┘    └───────────────┘    └─────────────┘
                                           │
                                           ▼
                                   ┌───────────────┐
                                   │ Model Output  │
                                   │ - prediction  │
                                   │ - confidence  │
                                   │ - anomaly     │
                                   └───────────────┘
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Service health check |
| `/predict` | POST | Single inference |
| `/batch` | POST | Batch inference |
| `/models` | GET | List available models |
| `/models/{name}` | PUT | Update model |

## Inference Flow

```
Request:
┌────────────────────────────────────────────┐
│ POST /predict                              │
│ {                                          │
│   "device_id": "sensor-001",               │
│   "features": [25.5, 60.2, 1013.2]         │
│ }                                          │
└────────────────────────────────────────────┘
                    │
                    ▼
            ┌──────────────┐
            │ Preprocess   │
            │ (normalize)  │
            └──────┬───────┘
                   │
                   ▼
            ┌──────────────┐
            │ Model.predict│
            │ (inference)  │
            └──────┬───────┘
                   │
                   ▼
            ┌──────────────┐
            │ Postprocess  │
            │ (format)     │
            └──────┬───────┘
                   │
                   ▼
Response:
┌────────────────────────────────────────────┐
│ {                                          │
│   "prediction": "normal",                  │
│   "confidence": 0.95,                      │
│   "model_version": "1.2.0"                 │
│ }                                          │
└────────────────────────────────────────────┘
```

## MQTT Integration

```python
# mqtt/client.py
from paho.mqtt import client as mqtt

def on_telemetry(client, userdata, msg):
    data = json.loads(msg.payload)
    features = extract_features(data)

    result = model.predict(features)

    client.publish(
        f"inference/{data['device_id']}/anomaly",
        json.dumps(result)
    )
```

## Docker Deployment

```yaml
# docker-compose.yml
services:
  ai-service:
    build: .
    ports:
      - "8000:8000"
    environment:
      - MQTT_HOST=mqtt.tesaiot.com
      - MQTT_PORT=8883
      - API_KEY=${API_KEY}
    volumes:
      - ./models:/app/models
```

## Running

```bash
# Development
uvicorn app.main:app --reload --port 8000

# Docker
docker-compose up -d

# Test
curl http://localhost:8000/health
curl -X POST http://localhost:8000/predict \
  -H "Content-Type: application/json" \
  -d '{"device_id": "test", "features": [25.5, 60]}'
```

## Dependencies

- fastapi - Web framework
- uvicorn - ASGI server
- paho-mqtt - MQTT client
- scikit-learn - ML models
- numpy - Numerical computing
- python-dotenv - Configuration
