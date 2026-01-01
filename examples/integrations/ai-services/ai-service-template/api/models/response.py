"""
Response Models

Pydantic models for API response serialization.
These models ensure consistent response structure across all endpoints.
"""

from typing import Any, Optional

from pydantic import BaseModel, Field


class InferenceResponse(BaseModel):
    """
    Single inference response model.

    Attributes:
        device_id: Device that was processed
        prediction: Model prediction (e.g., "normal", "anomaly")
        confidence: Prediction confidence score (0.0 - 1.0)
        metadata: Additional information about the inference

    Example:
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
    """

    device_id: str = Field(
        ...,
        description="Device identifier that was processed",
    )

    prediction: str = Field(
        ...,
        description="Model prediction result",
        examples=["normal", "anomaly", "warning"],
    )

    confidence: float = Field(
        ...,
        ge=0.0,
        le=1.0,
        description="Prediction confidence score (0.0 - 1.0)",
    )

    metadata: Optional[dict[str, Any]] = Field(
        default=None,
        description="Additional inference metadata",
    )

    model_config = {
        "json_schema_extra": {
            "examples": [
                {
                    "device_id": "device-001",
                    "prediction": "normal",
                    "confidence": 0.95,
                    "metadata": {
                        "model_version": "1.0.0",
                        "latency_ms": 12.5,
                        "timestamp": "2025-12-11T10:30:00Z",
                    },
                }
            ]
        }
    }


class BatchInferenceResponse(BaseModel):
    """
    Batch inference response model.

    Attributes:
        results: List of individual inference results
        total: Total number of processed items
        latency_ms: Total processing time in milliseconds

    Example:
        {
            "results": [...],
            "total": 10,
            "latency_ms": 125.5
        }
    """

    results: list[InferenceResponse] = Field(
        ...,
        description="List of inference results",
    )

    total: int = Field(
        ...,
        ge=0,
        description="Total number of processed items",
    )

    latency_ms: float = Field(
        ...,
        ge=0,
        description="Total processing time in milliseconds",
    )

    model_config = {
        "json_schema_extra": {
            "examples": [
                {
                    "results": [
                        {
                            "device_id": "device-001",
                            "prediction": "normal",
                            "confidence": 0.95,
                        },
                        {
                            "device_id": "device-002",
                            "prediction": "anomaly",
                            "confidence": 0.87,
                        },
                    ],
                    "total": 2,
                    "latency_ms": 25.5,
                }
            ]
        }
    }


class ErrorResponse(BaseModel):
    """
    Error response model for API errors.

    Attributes:
        error: Error type/code
        detail: Human-readable error description
        correlation_id: Request tracking ID (if provided)

    Example:
        {
            "error": "validation_error",
            "detail": "Data dictionary cannot be empty",
            "correlation_id": "req-12345"
        }
    """

    error: str = Field(
        ...,
        description="Error type or code",
    )

    detail: Optional[str] = Field(
        default=None,
        description="Human-readable error description",
    )

    correlation_id: Optional[str] = Field(
        default=None,
        description="Request tracking ID",
    )
