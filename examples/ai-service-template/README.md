# TESAIoT AI Service Template

A production-ready template for building AI/ML inference services that integrate with TESAIoT Platform.

## Features

- **FastAPI Framework** - High-performance async API with automatic documentation
- **Single & Batch Inference** - Process individual or multiple data points
- **Health Checks** - Kubernetes/Docker-ready health and readiness endpoints
- **Prometheus Metrics** - Built-in metrics endpoint for monitoring
- **Docker Support** - Multi-stage Dockerfile for optimized images
- **Configurable** - Environment-based configuration with Pydantic Settings

## Quick Start

### Local Development

```bash
# Create virtual environment
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Configure environment
cp .env.example .env

# Run the service
uvicorn api.main:app --reload --port 8000
```

### Docker

```bash
# Build image
docker build -t tesaiot-ai-service .

# Run container
docker run -p 8000:8000 tesaiot-ai-service

# Or use docker-compose
docker-compose up -d
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Health check |
| `/metrics` | GET | Prometheus metrics |
| `/inference` | POST | Single inference |
| `/inference/batch` | POST | Batch inference |
| `/docs` | GET | Swagger UI documentation |

## Usage Examples

### Single Inference

```bash
curl -X POST http://localhost:8000/inference \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "device-001",
    "data": {
      "temperature": 25.5,
      "humidity": 60.0
    }
  }'
```

**Response:**
```json
{
  "device_id": "device-001",
  "prediction": "normal",
  "confidence": 0.95,
  "metadata": {
    "model_version": "1.0.0",
    "latency_ms": 12.5,
    "timestamp": "2025-12-11T10:30:00Z"
  }
}
```

### Batch Inference

```bash
curl -X POST http://localhost:8000/inference/batch \
  -H "Content-Type: application/json" \
  -d '{
    "items": [
      {"device_id": "device-001", "data": {"temperature": 25.5}},
      {"device_id": "device-002", "data": {"temperature": 40.0}}
    ]
  }'
```

## Project Structure

```
ai-service-template/
├── api/
│   ├── main.py              # FastAPI application
│   ├── routes/
│   │   ├── health.py        # Health & metrics endpoints
│   │   └── inference.py     # Inference endpoints
│   └── models/
│       ├── request.py       # Request DTOs
│       └── response.py      # Response DTOs
├── core/
│   ├── config.py            # Configuration
│   └── processor.py         # AI inference logic (customize this!)
├── tests/
│   └── ...                  # Test files
├── Dockerfile
├── docker-compose.yml
├── requirements.txt
└── README.md
```

## Integrating Your ML Model

Replace the placeholder logic in `core/processor.py`:

```python
# Example: scikit-learn model
import joblib

class AIProcessor:
    def __init__(self, model_path: str):
        self._model = joblib.load(model_path)

    def process(self, data: dict) -> dict:
        features = self._extract_features(data)
        prediction = self._model.predict([features])[0]
        confidence = self._model.predict_proba([features]).max()
        return {
            "prediction": prediction,
            "confidence": float(confidence),
        }
```

## Configuration

| Variable | Description | Default |
|----------|-------------|---------|
| `SERVICE_NAME` | Service identifier | ai-service-template |
| `ENVIRONMENT` | Runtime environment | development |
| `LOG_LEVEL` | Logging level | info |
| `MODEL_PATH` | Path to ML model | None |
| `PORT` | Server port | 8000 |

## Testing

```bash
# Install dev dependencies
pip install -r requirements-dev.txt

# Run tests
pytest --cov=. tests/

# Type checking
mypy api/ core/

# Code formatting
black api/ core/ tests/
```

## Deployment

### Docker

```bash
# Build production image
docker build -t tesaiot-ai-service:latest .

# Push to registry
docker tag tesaiot-ai-service:latest your-registry/tesaiot-ai-service:latest
docker push your-registry/tesaiot-ai-service:latest
```

### Kubernetes

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ai-service
spec:
  replicas: 3
  template:
    spec:
      containers:
        - name: ai-service
          image: tesaiot-ai-service:latest
          ports:
            - containerPort: 8000
          livenessProbe:
            httpGet:
              path: /health
              port: 8000
          resources:
            limits:
              memory: "512Mi"
              cpu: "1000m"
```

## License

Apache 2.0 - See [LICENSE](../../LICENSE)

## Attribution

Built with [TESAIoT Developer Hub](https://github.com/tesaiot/developer-hub)
Copyright 2025 TESAIoT Platform by TESA
