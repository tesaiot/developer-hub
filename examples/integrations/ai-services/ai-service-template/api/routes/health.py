"""
Health Check and Metrics Endpoints

Provides endpoints for container orchestration (Kubernetes, Docker Swarm)
and monitoring systems (Prometheus, Grafana).

Endpoints:
- GET /health - Liveness and readiness check
- GET /metrics - Prometheus-compatible metrics
"""

import time
from datetime import datetime

from fastapi import APIRouter

from core.config import settings

router = APIRouter()

# Track service start time for uptime calculation
SERVICE_START_TIME = time.time()


@router.get("/health")
async def health_check():
    """
    Health check endpoint for container orchestration.

    Returns service status, version, and uptime. Use this endpoint for:
    - Kubernetes liveness/readiness probes
    - Docker health checks
    - Load balancer health checks

    Returns:
        dict: Health status with version and uptime
    """
    uptime_seconds = int(time.time() - SERVICE_START_TIME)

    return {
        "status": "healthy",
        "service": settings.service_name,
        "version": settings.version,
        "environment": settings.environment,
        "uptime_seconds": uptime_seconds,
        "timestamp": datetime.utcnow().isoformat(),
    }


@router.get("/metrics")
async def metrics():
    """
    Prometheus-compatible metrics endpoint.

    Returns metrics in Prometheus text format. Extend this endpoint
    to expose custom metrics for your AI service:
    - Inference count
    - Inference latency histogram
    - Model accuracy metrics
    - Queue depth (if applicable)

    Returns:
        str: Prometheus text format metrics
    """
    uptime = int(time.time() - SERVICE_START_TIME)

    # Basic metrics - extend with your own
    metrics_text = f"""# HELP ai_service_info Service information
# TYPE ai_service_info gauge
ai_service_info{{version="{settings.version}",service="{settings.service_name}"}} 1

# HELP ai_service_uptime_seconds Service uptime in seconds
# TYPE ai_service_uptime_seconds counter
ai_service_uptime_seconds {uptime}

# HELP ai_service_requests_total Total number of requests
# TYPE ai_service_requests_total counter
# TODO: Implement request counting
ai_service_requests_total 0

# HELP ai_service_inference_latency_seconds Inference latency in seconds
# TYPE ai_service_inference_latency_seconds histogram
# TODO: Implement latency histogram
"""

    return metrics_text
