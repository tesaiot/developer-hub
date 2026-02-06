"""
TESAIoT SSO Models

Pydantic models for authentication data.
"""

from datetime import datetime
from typing import Optional, List
from pydantic import BaseModel, Field


class TokenResponse(BaseModel):
    """OAuth token response from Keycloak."""

    access_token: str
    refresh_token: Optional[str] = None
    id_token: Optional[str] = None
    token_type: str = "Bearer"
    expires_in: int = 300
    refresh_expires_in: Optional[int] = None
    scope: str = "openid profile email"

    # Computed fields (set after creation)
    access_token_expires_at: Optional[datetime] = None
    refresh_token_expires_at: Optional[datetime] = None

    def model_post_init(self, __context) -> None:
        """Set expiration timestamps after initialization."""
        now = datetime.utcnow()
        if self.expires_in:
            self.access_token_expires_at = datetime.fromtimestamp(
                now.timestamp() + self.expires_in
            )
        if self.refresh_expires_in:
            self.refresh_token_expires_at = datetime.fromtimestamp(
                now.timestamp() + self.refresh_expires_in
            )

    def is_access_token_expired(self) -> bool:
        """Check if access token is expired."""
        if not self.access_token_expires_at:
            return True
        # Consider expired 30 seconds before actual expiry
        return datetime.utcnow().timestamp() >= (
            self.access_token_expires_at.timestamp() - 30
        )

    def is_refresh_token_expired(self) -> bool:
        """Check if refresh token is expired."""
        if not self.refresh_token_expires_at:
            return True
        return datetime.utcnow().timestamp() >= self.refresh_token_expires_at.timestamp()


class DeviceCodeResponse(BaseModel):
    """Device code flow response."""

    device_code: str
    user_code: str
    verification_uri: str
    verification_uri_complete: Optional[str] = None
    expires_in: int = 600
    interval: int = 5


class UserInfo(BaseModel):
    """User information from ID token or userinfo endpoint."""

    sub: str  # Subject (user ID)
    preferred_username: Optional[str] = None
    name: Optional[str] = None
    given_name: Optional[str] = None
    family_name: Optional[str] = None
    email: Optional[str] = None
    email_verified: bool = False
    organization_id: Optional[str] = None
    organization_name: Optional[str] = None
    roles: List[str] = Field(default_factory=list)

    @classmethod
    def from_token_claims(cls, claims: dict, client_id: str = "") -> "UserInfo":
        """Create UserInfo from JWT token claims."""
        # Extract roles from realm and resource access
        roles = []
        if "realm_access" in claims:
            roles.extend(claims["realm_access"].get("roles", []))
        if "resource_access" in claims and client_id:
            client_roles = claims["resource_access"].get(client_id, {})
            roles.extend(client_roles.get("roles", []))

        return cls(
            sub=claims.get("sub", ""),
            preferred_username=claims.get("preferred_username"),
            name=claims.get("name"),
            given_name=claims.get("given_name"),
            family_name=claims.get("family_name"),
            email=claims.get("email"),
            email_verified=claims.get("email_verified", False),
            organization_id=claims.get("organization_id"),
            organization_name=claims.get("organization_name"),
            roles=list(set(roles)),  # Remove duplicates
        )


class Device(BaseModel):
    """TESAIoT Device model."""

    id: str
    name: str
    device_id: str
    status: str = "unknown"
    last_seen: Optional[datetime] = None
    organization_id: Optional[str] = None
    metadata: Optional[dict] = None


class ApiError(BaseModel):
    """API error response."""

    status: int
    message: str
    details: Optional[dict] = None
