"""
TESAIoT Third-Party AI Service Template

FastAPI-based microservice template for integrating custom AI/ML models
with TESAIoT Platform. Use this as a starting point for building your
own AI inference services.

Features:
- Health check endpoint for container orchestration
- Prometheus-compatible metrics endpoint
- Single and batch inference endpoints
- Configuration via environment variables
- Structured logging with request tracing

Usage:
    uvicorn api.main:app --host 0.0.0.0 --port 8000 --reload

Environment Variables:
    SERVICE_NAME: Service identifier (default: ai-service)
    LOG_LEVEL: Logging level (default: info)
    MODEL_PATH: Path to ML model file (optional)

Author: TESAIoT Platform Team
License: Apache 2.0
"""

import logging
import time
from contextlib import asynccontextmanager

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse

from api.routes import health, inference
from core.config import settings

# Configure logging
logging.basicConfig(
    level=getattr(logging, settings.log_level.upper()),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    Application lifespan handler.

    Runs startup and shutdown logic for the FastAPI application.
    Use this for initializing ML models, database connections, etc.
    """
    # Startup
    logger.info(f"Starting {settings.service_name} v{settings.version}")
    logger.info(f"Environment: {settings.environment}")

    # TODO: Load your ML model here
    # from core.processor import AIProcessor
    # app.state.processor = AIProcessor()

    yield

    # Shutdown
    logger.info(f"Shutting down {settings.service_name}")


# Create FastAPI application
app = FastAPI(
    title=settings.service_name,
    description="TESAIoT Third-Party AI Service Template",
    version=settings.version,
    docs_url="/docs",
    redoc_url="/redoc",
    lifespan=lifespan,
)

# Configure CORS
# In production, replace "*" with specific allowed origins
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["GET", "POST"],
    allow_headers=["*"],
)


@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    """
    Middleware to add processing time header to all responses.

    Useful for monitoring and debugging latency issues.
    """
    start_time = time.perf_counter()
    response = await call_next(request)
    process_time = (time.perf_counter() - start_time) * 1000
    response.headers["X-Process-Time-Ms"] = f"{process_time:.2f}"
    return response


@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """
    Global exception handler for unhandled errors.

    Logs the error and returns a structured error response.
    Never expose internal error details in production!
    """
    logger.error(f"Unhandled error: {exc}", exc_info=True)
    return JSONResponse(
        status_code=500,
        content={
            "error": "Internal server error",
            "detail": str(exc) if settings.environment == "development" else None,
        },
    )


# Include routers
app.include_router(health.router, tags=["Health"])
app.include_router(inference.router, prefix="/inference", tags=["Inference"])


@app.get("/", include_in_schema=False)
async def root():
    """Redirect root to health endpoint."""
    return {"message": f"Welcome to {settings.service_name}", "docs": "/docs"}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        "api.main:app",
        host="0.0.0.0",
        port=8000,
        reload=settings.environment == "development",
    )
