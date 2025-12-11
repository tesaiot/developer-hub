"""
AI Inference Endpoints

Provides endpoints for single and batch AI inference requests.
Integrate your ML model by implementing the AIProcessor class.

Endpoints:
- POST /inference - Single inference request
- POST /inference/batch - Batch inference request
"""

import logging
import time
from typing import Any

from fastapi import APIRouter, HTTPException

from api.models.request import InferenceRequest, BatchInferenceRequest
from api.models.response import InferenceResponse, BatchInferenceResponse
from core.processor import AIProcessor

router = APIRouter()
logger = logging.getLogger(__name__)

# Initialize processor (in production, use dependency injection)
processor = AIProcessor()


@router.post("", response_model=InferenceResponse)
async def inference(request: InferenceRequest) -> InferenceResponse:
    """
    Single inference endpoint.

    Processes a single data point through the AI model and returns
    the prediction with confidence score and metadata.

    Args:
        request: Input data for inference

    Returns:
        InferenceResponse: Prediction result with metadata

    Raises:
        HTTPException: If inference fails

    Example:
        POST /inference
        {
            "device_id": "device-001",
            "data": {
                "temperature": 25.5,
                "humidity": 60.0,
                "pressure": 1013.25
            }
        }
    """
    start_time = time.perf_counter()

    try:
        logger.info(f"Processing inference request for device: {request.device_id}")

        # Run inference
        result = processor.process(request.data)

        # Calculate processing time
        latency_ms = (time.perf_counter() - start_time) * 1000

        logger.info(
            f"Inference complete: prediction={result['prediction']}, "
            f"confidence={result['confidence']:.2f}, latency={latency_ms:.2f}ms"
        )

        return InferenceResponse(
            device_id=request.device_id,
            prediction=result["prediction"],
            confidence=result["confidence"],
            metadata={
                "model_version": result.get("model_version", "1.0.0"),
                "latency_ms": round(latency_ms, 2),
                "timestamp": result.get("timestamp"),
            },
        )

    except ValueError as e:
        logger.warning(f"Validation error: {e}")
        raise HTTPException(status_code=400, detail=str(e))

    except Exception as e:
        logger.error(f"Inference error: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Inference failed")


@router.post("/batch", response_model=BatchInferenceResponse)
async def batch_inference(request: BatchInferenceRequest) -> BatchInferenceResponse:
    """
    Batch inference endpoint.

    Processes multiple data points in a single request. More efficient
    than multiple single requests for bulk processing.

    Args:
        request: Batch of input data for inference

    Returns:
        BatchInferenceResponse: List of prediction results

    Raises:
        HTTPException: If batch inference fails

    Example:
        POST /inference/batch
        {
            "items": [
                {"device_id": "device-001", "data": {"temperature": 25.5}},
                {"device_id": "device-002", "data": {"temperature": 30.0}}
            ]
        }
    """
    start_time = time.perf_counter()

    try:
        logger.info(f"Processing batch inference: {len(request.items)} items")

        results = []
        for item in request.items:
            result = processor.process(item.data)
            results.append(
                InferenceResponse(
                    device_id=item.device_id,
                    prediction=result["prediction"],
                    confidence=result["confidence"],
                    metadata={
                        "model_version": result.get("model_version", "1.0.0"),
                        "timestamp": result.get("timestamp"),
                    },
                )
            )

        latency_ms = (time.perf_counter() - start_time) * 1000

        logger.info(
            f"Batch inference complete: {len(results)} results, "
            f"latency={latency_ms:.2f}ms"
        )

        return BatchInferenceResponse(
            results=results,
            total=len(results),
            latency_ms=round(latency_ms, 2),
        )

    except Exception as e:
        logger.error(f"Batch inference error: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Batch inference failed")
