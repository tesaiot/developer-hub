"""
TESAIoT SSO Configuration

Manages configuration from environment variables.
"""

from functools import lru_cache
from typing import Optional
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """Application settings loaded from environment variables."""

    # Keycloak Configuration
    keycloak_url: str = "https://auth.tesaiot.org"
    keycloak_realm: str = "tesa-iot-platform"
    keycloak_client_id: str = "tesa-iot-web"
    keycloak_client_secret: Optional[str] = None

    # TESAIoT API
    tesaiot_api_url: str = "https://admin.tesaiot.com/api/v1"

    # Token Storage
    token_storage: str = "file"  # file, keyring, memory
    token_file_path: str = "~/.tesaiot/tokens.json"

    # Derived properties
    @property
    def keycloak_issuer(self) -> str:
        """Get the token issuer URL."""
        return f"{self.keycloak_url}/realms/{self.keycloak_realm}"

    @property
    def auth_url(self) -> str:
        """Get the authorization endpoint."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/auth"

    @property
    def token_url(self) -> str:
        """Get the token endpoint."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/token"

    @property
    def userinfo_url(self) -> str:
        """Get the userinfo endpoint."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/userinfo"

    @property
    def logout_url(self) -> str:
        """Get the logout endpoint."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/logout"

    @property
    def device_auth_url(self) -> str:
        """Get the device authorization endpoint."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/auth/device"

    @property
    def jwks_url(self) -> str:
        """Get the JWKS endpoint for token verification."""
        return f"{self.keycloak_issuer}/protocol/openid-connect/certs"

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        case_sensitive=False,
    )


@lru_cache()
def get_settings() -> Settings:
    """Get cached settings instance."""
    return Settings()
