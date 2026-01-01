"""
Application Configuration

Centralized configuration using Pydantic Settings.
All settings can be overridden via environment variables.

Environment Variables:
    SERVICE_NAME: Service identifier
    VERSION: Service version
    ENVIRONMENT: Runtime environment (development/production)
    LOG_LEVEL: Logging level (debug/info/warning/error)
    MODEL_PATH: Path to ML model file

Example .env file:
    SERVICE_NAME=my-ai-service
    VERSION=1.0.0
    ENVIRONMENT=production
    LOG_LEVEL=info
    MODEL_PATH=/models/model.pkl
"""

from functools import lru_cache
from typing import Literal

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """
    Application settings with environment variable support.

    Uses Pydantic Settings for automatic environment variable loading
    and validation. All settings have sensible defaults.
    """

    # Service identification
    service_name: str = "ai-service-template"
    version: str = "1.0.0"
    environment: Literal["development", "production"] = "development"

    # Logging
    log_level: Literal["debug", "info", "warning", "error"] = "info"

    # Model configuration
    model_path: str | None = None

    # Server settings
    host: str = "0.0.0.0"
    port: int = 8000

    # API settings
    api_prefix: str = ""
    docs_enabled: bool = True

    # Performance settings
    max_batch_size: int = 100
    request_timeout_seconds: int = 30

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        case_sensitive=False,
        extra="ignore",
    )


@lru_cache
def get_settings() -> Settings:
    """
    Get cached settings instance.

    Uses LRU cache to avoid re-reading environment variables
    on every settings access. Clear cache with:
    get_settings.cache_clear()

    Returns:
        Settings: Cached settings instance
    """
    return Settings()


# Global settings instance for convenience
settings = get_settings()
