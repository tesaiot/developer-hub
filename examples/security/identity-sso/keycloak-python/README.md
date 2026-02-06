# TESAIoT Keycloak Python SSO Example

Professional Python application demonstrating Single Sign-On integration with TESAIoT Platform using Keycloak.

## Features

- **Multiple Auth Flows** - Authorization Code, Client Credentials, Device Code
- **CLI Tool** - Interactive command-line authentication
- **API Client** - Authenticated requests to TESAIoT Core API
- **Token Management** - Automatic refresh and secure storage
- **FastAPI Integration** - Protected API endpoints example
- **Async Support** - Full async/await support with httpx

## Quick Start

### Prerequisites

- Python 3.10+
- pip or poetry
- TESAIoT Platform account with OAuth client configured

### Installation

```bash
# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Copy environment configuration
cp .env.example .env

# Edit .env with your Keycloak client settings
```

### CLI Authentication

```bash
# Interactive login (opens browser)
python sso_client.py login

# Login with device code (for headless environments)
python sso_client.py login --device-code

# View current token info
python sso_client.py token-info

# Make authenticated API call
python sso_client.py api /devices

# Logout
python sso_client.py logout
```

### FastAPI Server

```bash
# Run the API server with protected endpoints
uvicorn api_server:app --reload

# Open http://localhost:8000/docs for Swagger UI
```

## Project Structure

```
keycloak-python/
├── sso_client.py           # CLI SSO client
├── api_server.py           # FastAPI with Keycloak auth
├── keycloak_auth.py        # Keycloak authentication library
├── token_storage.py        # Secure token storage
├── models.py               # Pydantic models
├── config.py               # Configuration management
├── requirements.txt        # Python dependencies
├── .env.example            # Environment template
└── README.md
```

## Configuration

### Environment Variables

| Variable                 | Description                              | Example                            |
| ------------------------ | ---------------------------------------- | ---------------------------------- |
| `KEYCLOAK_URL`           | Keycloak server URL                      | `https://auth.tesaiot.org`         |
| `KEYCLOAK_REALM`         | Keycloak realm name                      | `tesa-iot-platform`                |
| `KEYCLOAK_CLIENT_ID`     | OAuth client ID                          | `tesa-iot-web`                     |
| `KEYCLOAK_CLIENT_SECRET` | Client secret (for confidential clients) | `your-secret`                      |
| `TESAIOT_API_URL`        | TESAIoT API URL                          | `https://admin.tesaiot.com/api/v1` |

## Usage Examples

### Authorization Code Flow (Interactive)

```python
from keycloak_auth import KeycloakAuth

auth = KeycloakAuth()

# Start interactive login
tokens = await auth.login_interactive()

# Use access token for API calls
headers = {"Authorization": f"Bearer {tokens.access_token}"}
```

### Client Credentials Flow (Service-to-Service)

```python
from keycloak_auth import KeycloakAuth

auth = KeycloakAuth(
    client_id="my-service",
    client_secret="service-secret"
)

# Get service token
tokens = await auth.login_client_credentials()
```

### Device Code Flow (Headless)

```python
from keycloak_auth import KeycloakAuth

auth = KeycloakAuth()

# Start device code flow
device_code = await auth.start_device_code_flow()
print(f"Go to: {device_code.verification_uri}")
print(f"Enter code: {device_code.user_code}")

# Poll for completion
tokens = await auth.poll_device_code(device_code)
```

### Protected FastAPI Endpoint

```python
from fastapi import FastAPI, Depends
from keycloak_auth import get_current_user, require_roles

app = FastAPI()

@app.get("/protected")
async def protected_route(user = Depends(get_current_user)):
    return {"message": f"Hello {user.name}"}

@app.get("/admin")
async def admin_route(user = Depends(require_roles(["admin"]))):
    return {"message": "Admin access granted"}
```

## Security Best Practices

| Practice              | Implementation                   |
| --------------------- | -------------------------------- |
| **Token Storage**     | Encrypted file or system keyring |
| **Token Refresh**     | Automatic refresh before expiry  |
| **HTTPS Only**        | All requests over TLS            |
| **Secret Management** | Environment variables or vault   |

---

**Category:** Security / Identity  
**Last Updated:** 2026-01-29
