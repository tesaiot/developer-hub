# Identity SSO Architecture

Technical architecture documentation for TESAIoT Single Sign-On integration.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              TESAIoT Platform                                   │
│                                                                                 │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                         Keycloak Identity Service                         │  │
│  │                                                                           │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐   │  │
│  │  │     Realm       │  │     Clients     │  │   Identity Providers    │   │  │
│  │  │"tesa-iot-platform"│ │                 │  │                         │   │  │
│  │  │                 │  │  • web-app      │  │  • Google               │   │  │
│  │  │  Users & Groups │  │  • mobile-app   │  │  • GitHub               │   │  │
│  │  │  Roles & Scopes │  │  • cli-tool     │  │  • Microsoft            │   │  │
│  │  │  Sessions       │  │  • api-service  │  │  • LDAP/AD              │   │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘   │  │
│  │                                                                           │  │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │  │
│  │  │                        Token Services                                │ │  │
│  │  │  • Access Token (JWT, RS256)     • ID Token (OIDC Claims)           │ │  │
│  │  │  • Refresh Token (Rotation)       • Offline Token (Long-lived)      │ │  │
│  │  └─────────────────────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│                                      │                                          │
│                                      ▼                                          │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                         TESA Core API (FastAPI)                          │  │
│  │                                                                           │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐   │  │
│  │  │   JWT Validator │  │   RBAC Enforcer │  │   API Endpoints         │   │  │
│  │  │                 │  │                 │  │                         │   │  │
│  │  │  Verify RS256   │  │  Check roles    │  │  /api/v1/devices        │   │  │
│  │  │  Validate exp   │  │  Check scopes   │  │  /api/v1/telemetry      │   │  │
│  │  │  Check issuer   │  │  Resource owner │  │  /api/v1/organizations  │   │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘   │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Client Types

### Public Clients

For applications where the client secret cannot be kept confidential.

| Client Type     | Use Case        | Auth Flow        | Token Storage    |
| --------------- | --------------- | ---------------- | ---------------- |
| **React SPA**   | Web dashboard   | Auth Code + PKCE | Memory / Session |
| **Flutter App** | Mobile app      | Auth Code + PKCE | Secure Storage   |
| **CLI Tool**    | Developer tools | Device Code      | Keychain         |

### Confidential Clients

For server-side applications that can securely store credentials.

| Client Type        | Use Case      | Auth Flow          | Token Storage |
| ------------------ | ------------- | ------------------ | ------------- |
| **Backend API**    | Microservice  | Client Credentials | Environment   |
| **Python Service** | Data pipeline | Client Credentials | Vault         |

## Token Flow Details

### 1. Authorization Code Flow with PKCE

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                              Browser / Mobile App                              │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                │
│  1. Generate PKCE Parameters                                                   │
│     ┌────────────────────────────────────────────────────────────────────┐    │
│     │  code_verifier = random(43-128 chars, [A-Za-z0-9-._~])             │    │
│     │  code_challenge = base64url(sha256(code_verifier))                 │    │
│     │  state = random(32 chars)  // CSRF protection                      │    │
│     └────────────────────────────────────────────────────────────────────┘    │
│                                                                                │
│  2. Redirect to Authorization Endpoint                                         │
│     ┌────────────────────────────────────────────────────────────────────┐    │
│     │  GET https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/  │    │
│     │      openid-connect/auth                                           │    │
│     │      ?client_id=tesa-iot-web                                       │    │
│     │      &response_type=code                                            │    │
│     │      &redirect_uri=https://your-app.com/callback                   │    │
│     │      &scope=openid profile email                                    │    │
│     │      &state=abc123                                                  │    │
│     │      &code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM   │    │
│     │      &code_challenge_method=S256                                    │    │
│     └────────────────────────────────────────────────────────────────────┘    │
│                                                                                │
│  3. User Authenticates at Keycloak                                             │
│     • Enter username/password                                                  │
│     • Optional: MFA verification                                               │
│     • Optional: Consent screen                                                 │
│                                                                                │
│  4. Receive Authorization Code                                                 │
│     ┌────────────────────────────────────────────────────────────────────┐    │
│     │  Redirect: https://your-app.com/callback                           │    │
│     │            ?code=authorization_code_here                            │    │
│     │            &state=abc123                                            │    │
│     └────────────────────────────────────────────────────────────────────┘    │
│                                                                                │
│  5. Exchange Code for Tokens                                                   │
│     ┌────────────────────────────────────────────────────────────────────┐    │
│     │  POST https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/ │    │
│     │       openid-connect/token                                         │    │
│     │  Content-Type: application/x-www-form-urlencoded                   │    │
│     │                                                                     │    │
│     │  grant_type=authorization_code                                      │    │
│     │  &client_id=tesa-iot-web                                            │    │
│     │  &code=authorization_code_here                                      │    │
│     │  &redirect_uri=https://your-app.com/callback                       │    │
│     │  &code_verifier=original_code_verifier                              │    │
│     └────────────────────────────────────────────────────────────────────┘    │
│                                                                                │
│  6. Receive Tokens                                                             │
│     ┌────────────────────────────────────────────────────────────────────┐    │
│     │  {                                                                  │    │
│     │    "access_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6...",             │    │
│     │    "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6...",            │    │
│     │    "id_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6...",                 │    │
│     │    "token_type": "Bearer",                                          │    │
│     │    "expires_in": 300                                                │    │
│     │  }                                                                  │    │
│     └────────────────────────────────────────────────────────────────────┘    │
│                                                                                │
└────────────────────────────────────────────────────────────────────────────────┘
```

### 2. Token Refresh Flow

```
┌─────────────────┐                                    ┌──────────────────┐
│    Your App     │                                    │     Keycloak     │
└────────┬────────┘                                    └────────┬─────────┘
         │                                                      │
         │  Access token expired (401 Unauthorized)             │
         │                                                      │
         │  POST /token                                         │
         │  grant_type=refresh_token                            │
         │  refresh_token=current_refresh_token                 │
         │  client_id=your-client-id                            │
         │─────────────────────────────────────────────────────>│
         │                                                      │
         │  New tokens (with rotated refresh_token)             │
         │  {                                                   │
         │    "access_token": "new_access_token",               │
         │    "refresh_token": "new_refresh_token",             │
         │    "expires_in": 300                                 │
         │  }                                                   │
         │<─────────────────────────────────────────────────────│
         │                                                      │
```

## JWT Token Structure

### Access Token Claims

```json
{
  "header": {
    "alg": "RS256",
    "typ": "JWT",
    "kid": "key-id-from-keycloak"
  },
  "payload": {
    "exp": 1706540400,
    "iat": 1706540100,
    "jti": "unique-token-id",
    "iss": "https://auth.tesaiot.org/realms/tesa-iot-platform",
    "aud": "account",
    "sub": "user-uuid",
    "typ": "Bearer",
    "azp": "your-client-id",
    "session_state": "session-uuid",
    "acr": "1",
    "allowed-origins": ["https://your-app.com"],
    "realm_access": {
      "roles": ["default-roles-tesa-iot-platform", "user"]
    },
    "resource_access": {
      "tesa-iot-web": {
        "roles": ["admin", "device-manager"]
      }
    },
    "scope": "openid profile email",
    "email_verified": true,
    "name": "John Doe",
    "preferred_username": "johndoe",
    "given_name": "John",
    "family_name": "Doe",
    "email": "john.doe@example.com",
    "organization_id": "org-uuid",
    "organization_name": "ACME Corp"
  }
}
```

## Role-Based Access Control

### Realm Roles

Global roles across all applications:

| Role     | Description            | Permissions                  |
| -------- | ---------------------- | ---------------------------- |
| `admin`  | Platform administrator | Full access to all resources |
| `user`   | Standard user          | Read/write own organization  |
| `viewer` | Read-only user         | Read-only access             |

### Client Roles

Application-specific roles:

| Role             | Description        | Permissions                  |
| ---------------- | ------------------ | ---------------------------- |
| `device-manager` | Manage devices     | CRUD devices, view telemetry |
| `data-analyst`   | Analytics access   | Read telemetry, run queries  |
| `org-admin`      | Organization admin | Manage users, settings       |

## Security Considerations

### Token Security

| Aspect         | Recommendation                                        |
| -------------- | ----------------------------------------------------- |
| **Storage**    | Never store tokens in localStorage for sensitive apps |
| **Transport**  | Always use HTTPS                                      |
| **Expiration** | Keep access tokens short-lived (5-15 min)             |
| **Refresh**    | Enable refresh token rotation                         |
| **Revocation** | Implement logout that revokes tokens                  |

### PKCE Security

| Aspect              | Requirement                    |
| ------------------- | ------------------------------ |
| **Code Verifier**   | Min 43 chars, max 128 chars    |
| **Code Challenge**  | SHA256 hash, base64url encoded |
| **State Parameter** | Random, verify on callback     |

## Integration Points

### API Authentication

```
GET /api/v1/devices
Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6...
```

### WebSocket Authentication

```javascript
const ws = new WebSocket("wss://mqtt.tesaiot.com/ws");
ws.onopen = () => {
  ws.send(
    JSON.stringify({
      type: "auth",
      token: accessToken,
    }),
  );
};
```

### MQTT Authentication

```
CONNECT
  username: access_token
  password: <empty>
```

---

**Last Updated:** 2026-01-29
