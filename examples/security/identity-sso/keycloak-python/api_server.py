"""
TESAIoT FastAPI Server with Keycloak Authentication

Example FastAPI application with protected endpoints using Keycloak SSO.

Run:
    uvicorn api_server:app --reload

Open http://localhost:8000/docs for Swagger UI.

Author: TESAIoT Team
License: Apache 2.0
"""

from typing import Optional, List
from functools import lru_cache

from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer
from jose import jwt, JWTError
import httpx

from config import get_settings, Settings
from models import UserInfo


# ============================================================================
# FastAPI App
# ============================================================================

app = FastAPI(
    title="TESAIoT SSO Example API",
    description="Example API with Keycloak authentication",
    version="1.0.0",
)


# ============================================================================
# OAuth2 Configuration
# ============================================================================

oauth2_scheme = OAuth2PasswordBearer(
    tokenUrl="token",
    auto_error=True,
)


# JWKS cache for token verification
@lru_cache()
def get_jwks(jwks_url: str) -> dict:
    """Fetch and cache JWKS."""
    response = httpx.get(jwks_url)
    return response.json()


# ============================================================================
# Dependencies
# ============================================================================

async def get_current_user(
    token: str = Depends(oauth2_scheme),
    settings: Settings = Depends(get_settings),
) -> UserInfo:
    """
    Validate token and extract user information.

    Use as a dependency for protected endpoints.
    """
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )

    try:
        # Get JWKS for verification
        jwks = get_jwks(settings.jwks_url)

        # Decode and verify token
        payload = jwt.decode(
            token,
            jwks,
            algorithms=["RS256"],
            audience="account",
            issuer=settings.keycloak_issuer,
        )

        # Extract user info
        user = UserInfo.from_token_claims(payload, settings.keycloak_client_id)
        return user

    except JWTError as e:
        raise credentials_exception from e


def require_roles(required_roles: List[str]):
    """
    Dependency factory for role-based access control.

    Usage:
        @app.get("/admin")
        async def admin_endpoint(user = Depends(require_roles(["admin"]))):
            ...
    """
    async def role_checker(user: UserInfo = Depends(get_current_user)) -> UserInfo:
        for role in required_roles:
            if role in user.roles:
                return user

        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail=f"Required roles: {', '.join(required_roles)}",
        )

    return role_checker


# ============================================================================
# Public Endpoints
# ============================================================================

@app.get("/")
async def root():
    """Public endpoint - no authentication required."""
    return {
        "message": "TESAIoT SSO Example API",
        "docs": "/docs",
        "health": "/health",
    }


@app.get("/health")
async def health():
    """Health check endpoint."""
    return {"status": "healthy"}


@app.get("/.well-known/openid-configuration")
async def openid_config(settings: Settings = Depends(get_settings)):
    """Return OpenID Connect discovery document."""
    return {
        "issuer": settings.keycloak_issuer,
        "authorization_endpoint": settings.auth_url,
        "token_endpoint": settings.token_url,
        "userinfo_endpoint": settings.userinfo_url,
        "jwks_uri": settings.jwks_url,
        "end_session_endpoint": settings.logout_url,
    }


# ============================================================================
# Protected Endpoints
# ============================================================================

@app.get("/me")
async def get_current_user_info(user: UserInfo = Depends(get_current_user)):
    """
    Get current user information.

    Requires valid access token.
    """
    return {
        "user_id": user.sub,
        "username": user.preferred_username,
        "name": user.name,
        "email": user.email,
        "email_verified": user.email_verified,
        "organization_id": user.organization_id,
        "organization_name": user.organization_name,
        "roles": user.roles,
    }


@app.get("/protected")
async def protected_resource(user: UserInfo = Depends(get_current_user)):
    """
    Protected resource - requires authentication.

    Any authenticated user can access this endpoint.
    """
    return {
        "message": f"Hello {user.name}!",
        "access": "authenticated",
    }


@app.get("/devices")
async def list_devices(user: UserInfo = Depends(get_current_user)):
    """
    List devices - requires authentication.

    Returns mock device data for demonstration.
    """
    # In production, fetch from TESAIoT API or database
    mock_devices = [
        {
            "id": "dev-001",
            "name": "Temperature Sensor 1",
            "device_id": "temp-sensor-001",
            "status": "online",
            "organization_id": user.organization_id,
        },
        {
            "id": "dev-002",
            "name": "Humidity Sensor 1",
            "device_id": "humidity-sensor-001",
            "status": "online",
            "organization_id": user.organization_id,
        },
        {
            "id": "dev-003",
            "name": "Gateway 1",
            "device_id": "gateway-001",
            "status": "offline",
            "organization_id": user.organization_id,
        },
    ]

    return {
        "devices": mock_devices,
        "total": len(mock_devices),
    }


# ============================================================================
# Role-Protected Endpoints
# ============================================================================

@app.get("/admin/users")
async def list_users(user: UserInfo = Depends(require_roles(["admin"]))):
    """
    List users - requires admin role.

    Only users with 'admin' role can access this endpoint.
    """
    return {
        "message": "Admin access granted",
        "accessed_by": user.preferred_username,
        "users": [
            {"id": "user-1", "name": "John Doe"},
            {"id": "user-2", "name": "Jane Smith"},
        ],
    }


@app.get("/manager/reports")
async def get_reports(
    user: UserInfo = Depends(require_roles(["admin", "device-manager"]))
):
    """
    Get reports - requires admin or device-manager role.

    Users with either 'admin' or 'device-manager' role can access.
    """
    return {
        "message": "Report access granted",
        "accessed_by": user.preferred_username,
        "reports": [
            {"id": "report-1", "title": "Device Status Report"},
            {"id": "report-2", "title": "Telemetry Summary"},
        ],
    }


# ============================================================================
# Token Introspection (for debugging)
# ============================================================================

@app.get("/debug/token")
async def debug_token(
    user: UserInfo = Depends(get_current_user),
):
    """
    Debug endpoint to view token claims.

    Shows all user information extracted from the token.
    WARNING: Only enable in development environments.
    """
    return {
        "sub": user.sub,
        "preferred_username": user.preferred_username,
        "name": user.name,
        "given_name": user.given_name,
        "family_name": user.family_name,
        "email": user.email,
        "email_verified": user.email_verified,
        "organization_id": user.organization_id,
        "organization_name": user.organization_name,
        "roles": user.roles,
    }
