# TESAIoT Identity & SSO Examples

Single Sign-On (SSO) and Third-party Authentication integration examples for TESAIoT Platform using Keycloak Identity Service.

## Overview

TESAIoT Platform uses **Keycloak** as the centralized Identity and Access Management (IAM) service, providing:

- **Single Sign-On (SSO)** - One login for all TESAIoT services
- **OAuth 2.0 / OpenID Connect** - Industry-standard authentication protocols
- **Social Login** - Google, GitHub, Microsoft integration
- **Multi-Factor Authentication (MFA)** - TOTP, WebAuthn support
- **Role-Based Access Control (RBAC)** - Fine-grained permissions

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TESAIoT Platform                             │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    Keycloak Identity Service                 │   │
│  │  ┌─────────────────┐ ┌─────────────┐ ┌─────────────────────┐│   │
│  │  │     Realm:      │ │   Clients   │ │   Identity          ││   │
│  │  │tesa-iot-platform│ │   (Apps)    │ │   Providers         ││   │
│  │  └─────────────────┘ └─────────────┘ └─────────────────────┘│   │
│  └─────────────────────────────────────────────────────────────┘   │
│         ▲                    ▲                    ▲                 │
│         │ OIDC               │ OIDC               │ OIDC            │
│         │                    │                    │                 │
└─────────┼────────────────────┼────────────────────┼─────────────────┘
          │                    │                    │
    ┌─────┴─────┐        ┌─────┴─────┐        ┌─────┴─────┐
    │   React   │        │   Python  │        │  Flutter  │
    │    Web    │        │    CLI    │        │   Mobile  │
    └───────────┘        └───────────┘        └───────────┘
```

## Examples

| Example                                 | Language         | Description               | Status |
| --------------------------------------- | ---------------- | ------------------------- | ------ |
| [keycloak-react](./keycloak-react/)     | React/TypeScript | Web SSO with PKCE flow    | Ready  |
| [keycloak-python](./keycloak-python/)   | Python           | CLI & API SSO integration | Ready  |
| [keycloak-flutter](./keycloak-flutter/) | Flutter/Dart     | Mobile SSO with AppAuth   | Ready  |

## Keycloak Configuration

### TESAIoT Keycloak Endpoints

| Endpoint              | URL                                                                                  |
| --------------------- | ------------------------------------------------------------------------------------ |
| **Keycloak URL**      | `https://auth.tesaiot.org`                                                           |
| **Realm**             | `tesa-iot-platform`                                                                  |
| **Client ID**         | `tesa-iot-web`                                                                       |
| **Auth Endpoint**     | `https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/openid-connect/auth`     |
| **Token Endpoint**    | `https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/openid-connect/token`    |
| **UserInfo Endpoint** | `https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/openid-connect/userinfo` |
| **Logout Endpoint**   | `https://auth.tesaiot.org/realms/tesa-iot-platform/protocol/openid-connect/logout`   |

### Keycloak JSON Configuration (keycloak.json)

ตัวอย่างไฟล์ `keycloak.json` สำหรับการตั้งค่า Keycloak client:

```json
{
  "auth-server-url": "https://auth.tesaiot.org",
  "realm": "tesa-iot-platform",
  "resource": "tesa-iot-web",
  "public-client": true,
  "confidential-port": 0,
  "ssl-required": "external"
}
```

| Field             | Description                                      |
| ----------------- | ------------------------------------------------ |
| `auth-server-url` | URL ของ Keycloak server                          |
| `realm`           | ชื่อ Realm ที่ใช้งาน                             |
| `resource`        | Client ID ที่ลงทะเบียนใน Keycloak                |
| `public-client`   | `true` สำหรับ SPA/Mobile, `false` สำหรับ Backend |
| `ssl-required`    | `external` หรือ `all` สำหรับ production          |

### Client Configuration

Register your application in TESAIoT Admin Portal:

1. Navigate to **Settings → OAuth Clients**
2. Click **Create Client**
3. Configure:
   - **Client ID**: Your application identifier
   - **Client Type**: Public (SPA/Mobile) or Confidential (Backend)
   - **Redirect URIs**: Your callback URLs
   - **Allowed Origins**: CORS origins (for web apps)

## Authentication Flows

### Authorization Code Flow with PKCE (Recommended)

Best for: **Web Apps (React), Mobile Apps (Flutter)**

```
┌──────────┐                              ┌──────────────┐                              ┌──────────┐
│   User   │                              │  Your App    │                              │ Keycloak │
└────┬─────┘                              └──────┬───────┘                              └────┬─────┘
     │                                           │                                           │
     │  1. Click "Login"                         │                                           │
     │──────────────────────────────────────────>│                                           │
     │                                           │                                           │
     │                                           │  2. Generate code_verifier + code_challenge
     │                                           │─────────────────────────────────────────>│
     │                                           │                                           │
     │  3. Redirect to Keycloak login            │                                           │
     │<──────────────────────────────────────────│                                           │
     │                                           │                                           │
     │  4. Enter credentials                     │                                           │
     │──────────────────────────────────────────────────────────────────────────────────────>│
     │                                           │                                           │
     │  5. Redirect back with authorization_code │                                           │
     │<──────────────────────────────────────────────────────────────────────────────────────│
     │                                           │                                           │
     │                                           │  6. Exchange code + code_verifier         │
     │                                           │──────────────────────────────────────────>│
     │                                           │                                           │
     │                                           │  7. Return access_token + refresh_token   │
     │                                           │<──────────────────────────────────────────│
     │                                           │                                           │
     │  8. Authenticated!                        │                                           │
     │<──────────────────────────────────────────│                                           │
```

### Client Credentials Flow

Best for: **Backend Services, CLI Tools**

```
┌─────────────┐                              ┌──────────┐
│  Your App   │                              │ Keycloak │
└──────┬──────┘                              └────┬─────┘
       │                                          │
       │  1. POST /token                          │
       │     client_id + client_secret            │
       │─────────────────────────────────────────>│
       │                                          │
       │  2. Return access_token                  │
       │<─────────────────────────────────────────│
       │                                          │
```

## Quick Start

### 1. React Web App

```bash
cd keycloak-react
npm install
cp .env.example .env
# Edit .env with your client configuration
npm run dev
```

### 2. Python CLI/Backend

```bash
cd keycloak-python
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
# Edit .env with your client configuration
python sso_client.py login
```

### 3. Flutter Mobile App

```bash
cd keycloak-flutter
flutter pub get
# Configure lib/config/keycloak_config.dart
flutter run
```

## Token Management

### Access Token

- Short-lived (default: 5 minutes)
- Used for API authorization
- Sent in `Authorization: Bearer <token>` header

### Refresh Token

- Long-lived (default: 30 minutes)
- Used to obtain new access tokens
- Store securely (httpOnly cookie or secure storage)

### ID Token

- Contains user identity claims
- JWT format with user profile data
- Used for frontend user display

## Security Best Practices

| Practice            | Description                                       |
| ------------------- | ------------------------------------------------- |
| **Use PKCE**        | Always use PKCE for public clients (SPA, Mobile)  |
| **Secure Storage**  | Store tokens in secure storage (not localStorage) |
| **Token Rotation**  | Enable refresh token rotation                     |
| **Short Expiry**    | Keep access token lifetime short (5-15 min)       |
| **HTTPS Only**      | Always use HTTPS in production                    |
| **Validate Tokens** | Verify token signature and claims server-side     |

## NCSA Compliance

This example supports **NCSA Level 2** requirements:

- [x] JWT authentication with RS256 signing
- [x] OAuth 2.0 / OpenID Connect standards
- [x] Secure token storage patterns
- [x] Multi-factor authentication ready
- [x] Role-based access control

---

**Category:** Security / Identity  
**NCSA Level:** Level 2  
**Last Updated:** 2026-01-29
