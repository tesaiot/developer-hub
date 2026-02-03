"""
TESAIoT Keycloak Authentication Library

Provides authentication functionality for TESAIoT Platform SSO.

Usage:
    from keycloak_auth import KeycloakAuth

    auth = KeycloakAuth()
    tokens = await auth.login_interactive()
"""

import asyncio
import base64
import hashlib
import secrets
import webbrowser
from http.server import HTTPServer, BaseHTTPRequestHandler
from typing import Optional, Callable
from urllib.parse import urlencode, urlparse, parse_qs

import httpx
from jose import jwt, JWTError

from config import get_settings, Settings
from models import TokenResponse, DeviceCodeResponse, UserInfo
from token_storage import TokenStorage, get_token_storage


class KeycloakAuth:
    """
    Keycloak Authentication Client

    Supports multiple OAuth 2.0 flows:
    - Authorization Code with PKCE (interactive)
    - Client Credentials (service-to-service)
    - Device Code (headless environments)
    """

    def __init__(
        self,
        settings: Optional[Settings] = None,
        token_storage: Optional[TokenStorage] = None,
    ):
        self.settings = settings or get_settings()
        self.storage = token_storage or get_token_storage()
        self._http_client: Optional[httpx.AsyncClient] = None
        self._jwks_cache: Optional[dict] = None

    async def _get_client(self) -> httpx.AsyncClient:
        """Get or create HTTP client."""
        if self._http_client is None:
            self._http_client = httpx.AsyncClient(timeout=30.0)
        return self._http_client

    async def close(self) -> None:
        """Close HTTP client."""
        if self._http_client:
            await self._http_client.aclose()
            self._http_client = None

    # =========================================================================
    # Authorization Code Flow with PKCE
    # =========================================================================

    def _generate_pkce(self) -> tuple[str, str]:
        """Generate PKCE code verifier and challenge."""
        # Generate random code verifier (43-128 characters)
        code_verifier = secrets.token_urlsafe(64)

        # Create code challenge using S256
        code_challenge = base64.urlsafe_b64encode(
            hashlib.sha256(code_verifier.encode()).digest()
        ).decode().rstrip("=")

        return code_verifier, code_challenge

    async def login_interactive(
        self,
        redirect_uri: str = "http://localhost:8400/callback",
        scopes: list[str] = None,
    ) -> TokenResponse:
        """
        Start interactive login using Authorization Code flow with PKCE.

        Opens browser for user authentication.
        """
        scopes = scopes or ["openid", "profile", "email"]
        state = secrets.token_urlsafe(32)
        code_verifier, code_challenge = self._generate_pkce()

        # Build authorization URL
        auth_params = {
            "client_id": self.settings.keycloak_client_id,
            "response_type": "code",
            "redirect_uri": redirect_uri,
            "scope": " ".join(scopes),
            "state": state,
            "code_challenge": code_challenge,
            "code_challenge_method": "S256",
        }

        auth_url = f"{self.settings.auth_url}?{urlencode(auth_params)}"

        # Start local callback server
        authorization_code = await self._wait_for_callback(redirect_uri, state)

        # Exchange code for tokens
        tokens = await self._exchange_code(
            authorization_code, redirect_uri, code_verifier
        )

        # Save tokens
        await self.storage.save(tokens)

        # Open browser for login
        print(f"\n🔐 Opening browser for login...")
        print(f"   URL: {auth_url[:80]}...")
        webbrowser.open(auth_url)

        return tokens

    async def _wait_for_callback(self, redirect_uri: str, expected_state: str) -> str:
        """Wait for OAuth callback and extract authorization code."""
        parsed = urlparse(redirect_uri)
        port = parsed.port or 8400

        # Storage for callback result
        result = {"code": None, "error": None}

        class CallbackHandler(BaseHTTPRequestHandler):
            def do_GET(self):
                query = parse_qs(urlparse(self.path).query)

                # Check state
                state = query.get("state", [None])[0]
                if state != expected_state:
                    result["error"] = "State mismatch"
                    self._send_error("State mismatch. Possible CSRF attack.")
                    return

                # Check for error
                if "error" in query:
                    result["error"] = query["error"][0]
                    self._send_error(f"Authentication error: {result['error']}")
                    return

                # Extract code
                code = query.get("code", [None])[0]
                if code:
                    result["code"] = code
                    self._send_success()
                else:
                    result["error"] = "No authorization code received"
                    self._send_error("No authorization code received")

            def _send_success(self):
                self.send_response(200)
                self.send_header("Content-type", "text/html")
                self.end_headers()
                self.wfile.write(b"""
                    <html>
                    <head><title>Login Successful</title></head>
                    <body style="font-family: sans-serif; text-align: center; padding: 50px;">
                        <h1 style="color: #10b981;">&#10004; Login Successful</h1>
                        <p>You can close this window and return to the terminal.</p>
                    </body>
                    </html>
                """)

            def _send_error(self, message: str):
                self.send_response(400)
                self.send_header("Content-type", "text/html")
                self.end_headers()
                self.wfile.write(f"""
                    <html>
                    <head><title>Login Error</title></head>
                    <body style="font-family: sans-serif; text-align: center; padding: 50px;">
                        <h1 style="color: #ef4444;">&#10008; Login Error</h1>
                        <p>{message}</p>
                    </body>
                    </html>
                """.encode())

            def log_message(self, format, *args):
                pass  # Suppress HTTP server logs

        # Run server in background
        server = HTTPServer(("localhost", port), CallbackHandler)
        server.timeout = 120  # 2 minute timeout

        def handle_request():
            server.handle_request()

        loop = asyncio.get_event_loop()
        await loop.run_in_executor(None, handle_request)

        if result["error"]:
            raise Exception(f"Authentication failed: {result['error']}")

        return result["code"]

    async def _exchange_code(
        self, code: str, redirect_uri: str, code_verifier: str
    ) -> TokenResponse:
        """Exchange authorization code for tokens."""
        client = await self._get_client()

        data = {
            "grant_type": "authorization_code",
            "client_id": self.settings.keycloak_client_id,
            "code": code,
            "redirect_uri": redirect_uri,
            "code_verifier": code_verifier,
        }

        # Add client secret for confidential clients
        if self.settings.keycloak_client_secret:
            data["client_secret"] = self.settings.keycloak_client_secret

        response = await client.post(
            self.settings.token_url,
            data=data,
            headers={"Content-Type": "application/x-www-form-urlencoded"},
        )

        if response.status_code != 200:
            error = response.json()
            raise Exception(
                f"Token exchange failed: {error.get('error_description', error)}"
            )

        return TokenResponse(**response.json())

    # =========================================================================
    # Client Credentials Flow
    # =========================================================================

    async def login_client_credentials(self) -> TokenResponse:
        """
        Login using Client Credentials flow.

        For service-to-service authentication.
        Requires client_secret to be configured.
        """
        if not self.settings.keycloak_client_secret:
            raise ValueError("Client secret is required for client credentials flow")

        client = await self._get_client()

        data = {
            "grant_type": "client_credentials",
            "client_id": self.settings.keycloak_client_id,
            "client_secret": self.settings.keycloak_client_secret,
            "scope": "openid",
        }

        response = await client.post(
            self.settings.token_url,
            data=data,
            headers={"Content-Type": "application/x-www-form-urlencoded"},
        )

        if response.status_code != 200:
            error = response.json()
            raise Exception(
                f"Client credentials login failed: {error.get('error_description', error)}"
            )

        tokens = TokenResponse(**response.json())
        await self.storage.save(tokens)
        return tokens

    # =========================================================================
    # Device Code Flow
    # =========================================================================

    async def start_device_code_flow(
        self, scopes: list[str] = None
    ) -> DeviceCodeResponse:
        """
        Start device code flow.

        For headless environments (no browser available).
        """
        scopes = scopes or ["openid", "profile", "email"]
        client = await self._get_client()

        data = {
            "client_id": self.settings.keycloak_client_id,
            "scope": " ".join(scopes),
        }

        response = await client.post(
            self.settings.device_auth_url,
            data=data,
            headers={"Content-Type": "application/x-www-form-urlencoded"},
        )

        if response.status_code != 200:
            error = response.json()
            raise Exception(
                f"Device code request failed: {error.get('error_description', error)}"
            )

        return DeviceCodeResponse(**response.json())

    async def poll_device_code(
        self,
        device_code: DeviceCodeResponse,
        callback: Optional[Callable[[str], None]] = None,
    ) -> TokenResponse:
        """
        Poll for device code completion.

        Waits for user to complete authentication on another device.
        """
        client = await self._get_client()

        data = {
            "grant_type": "urn:ietf:params:oauth:grant-type:device_code",
            "client_id": self.settings.keycloak_client_id,
            "device_code": device_code.device_code,
        }

        # Poll until success or expiry
        for _ in range(device_code.expires_in // device_code.interval):
            await asyncio.sleep(device_code.interval)

            response = await client.post(
                self.settings.token_url,
                data=data,
                headers={"Content-Type": "application/x-www-form-urlencoded"},
            )

            if response.status_code == 200:
                tokens = TokenResponse(**response.json())
                await self.storage.save(tokens)
                return tokens

            result = response.json()
            error = result.get("error")

            if error == "authorization_pending":
                if callback:
                    callback("Waiting for user authentication...")
                continue
            elif error == "slow_down":
                await asyncio.sleep(5)
                continue
            elif error == "expired_token":
                raise Exception("Device code expired")
            elif error == "access_denied":
                raise Exception("Access denied by user")
            else:
                raise Exception(f"Device code error: {error}")

        raise Exception("Device code polling timeout")

    # =========================================================================
    # Token Management
    # =========================================================================

    async def refresh_token(self) -> TokenResponse:
        """Refresh access token using refresh token."""
        tokens = await self.storage.load()
        if not tokens or not tokens.refresh_token:
            raise Exception("No refresh token available")

        client = await self._get_client()

        data = {
            "grant_type": "refresh_token",
            "client_id": self.settings.keycloak_client_id,
            "refresh_token": tokens.refresh_token,
        }

        if self.settings.keycloak_client_secret:
            data["client_secret"] = self.settings.keycloak_client_secret

        response = await client.post(
            self.settings.token_url,
            data=data,
            headers={"Content-Type": "application/x-www-form-urlencoded"},
        )

        if response.status_code != 200:
            error = response.json()
            await self.storage.clear()
            raise Exception(
                f"Token refresh failed: {error.get('error_description', error)}"
            )

        new_tokens = TokenResponse(**response.json())
        await self.storage.save(new_tokens)
        return new_tokens

    async def get_valid_token(self) -> Optional[str]:
        """Get a valid access token, refreshing if necessary."""
        tokens = await self.storage.load()
        if not tokens:
            return None

        if tokens.is_access_token_expired():
            if tokens.refresh_token and not tokens.is_refresh_token_expired():
                tokens = await self.refresh_token()
            else:
                return None

        return tokens.access_token

    async def logout(self) -> None:
        """Logout and clear tokens."""
        tokens = await self.storage.load()

        if tokens and tokens.refresh_token:
            try:
                client = await self._get_client()
                await client.post(
                    self.settings.logout_url,
                    data={
                        "client_id": self.settings.keycloak_client_id,
                        "refresh_token": tokens.refresh_token,
                    },
                )
            except Exception:
                pass  # Best effort logout

        await self.storage.clear()

    # =========================================================================
    # User Information
    # =========================================================================

    async def get_user_info(self) -> Optional[UserInfo]:
        """Get user information from current token."""
        token = await self.get_valid_token()
        if not token:
            return None

        try:
            # Decode token without verification for claims
            claims = jwt.get_unverified_claims(token)
            return UserInfo.from_token_claims(
                claims, self.settings.keycloak_client_id
            )
        except JWTError:
            return None

    async def verify_token(self, token: str) -> Optional[dict]:
        """Verify token signature and claims."""
        # Fetch JWKS if not cached
        if not self._jwks_cache:
            client = await self._get_client()
            response = await client.get(self.settings.jwks_url)
            self._jwks_cache = response.json()

        try:
            claims = jwt.decode(
                token,
                self._jwks_cache,
                algorithms=["RS256"],
                audience="account",
                issuer=self.settings.keycloak_issuer,
            )
            return claims
        except JWTError as e:
            print(f"Token verification failed: {e}")
            return None
