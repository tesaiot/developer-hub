"""
Request Models

Pydantic models for validating incoming API requests.
These models ensure type safety and provide automatic documentation.
"""

from typing import Any, Optional

from pydantic import BaseModel, Field, field_validator


class InferenceRequest(BaseModel):
    """
    Single inference request model.

    Attributes:
        device_id: Unique identifier for the IoT device
        data: Sensor data dictionary for inference
        correlation_id: Optional request tracking ID

    Example:
        {
            "device_id": "device-001",
            "data": {
                "temperature": 25.5,
                "humidity": 60.0,
                "pressure": 1013.25
            },
            "correlation_id": "req-12345"
        }
    """

    device_id: str = Field(
        ...,
        min_length=1,
        max_length=128,
        description="Unique device identifier",
        examples=["device-001", "sensor-temp-01"],
    )

    data: dict[str, Any] = Field(
        ...,
        description="Sensor data for inference",
        examples=[{"temperature": 25.5, "humidity": 60.0}],
    )

    correlation_id: Optional[str] = Field(
        default=None,
        max_length=64,
        description="Optional request tracking ID",
    )

    @field_validator("data")
    @classmethod
    def validate_data(cls, v: dict) -> dict:
        """Validate that data is not empty."""
        if not v:
            raise ValueError("Data dictionary cannot be empty")
        return v

    model_config = {
        "json_schema_extra": {
            "examples": [
                {
                    "device_id": "device-001",
                    "data": {
                        "temperature": 25.5,
                        "humidity": 60.0,
                        "pressure": 1013.25,
                    },
                    "correlation_id": "req-12345",
                }
            ]
        }
    }


class BatchInferenceRequest(BaseModel):
    """
    Batch inference request model.

    Attributes:
        items: List of inference requests to process

    Example:
        {
            "items": [
                {"device_id": "device-001", "data": {"temperature": 25.5}},
                {"device_id": "device-002", "data": {"temperature": 30.0}}
            ]
        }
    """

    items: list[InferenceRequest] = Field(
        ...,
        min_length=1,
        max_length=100,
        description="List of inference requests (max 100)",
    )

    @field_validator("items")
    @classmethod
    def validate_items(cls, v: list) -> list:
        """Validate batch size."""
        if len(v) > 100:
            raise ValueError("Batch size cannot exceed 100 items")
        return v

    model_config = {
        "json_schema_extra": {
            "examples": [
                {
                    "items": [
                        {"device_id": "device-001", "data": {"temperature": 25.5}},
                        {"device_id": "device-002", "data": {"temperature": 30.0}},
                    ]
                }
            ]
        }
    }
