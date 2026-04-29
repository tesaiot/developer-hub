# TESAIoT Examples Catalog

**Version:** 1.0  
**Date:** 2026-04-08  
**Scope:** Security/Identity-SSO, MQTT Integration, and REST API Applications

---

## Table of Contents

1. [Quick Reference](#quick-reference)
2. [Security / Identity-SSO](#1-security--identity-sso)
3. [MQTT Integration](#2-mqtt-integration)
4. [Applications (REST API)](#3-applications-rest-api)
5. [Language Coverage Summary](#4-language-coverage-summary)
6. [Recommendations for New Examples](#5-recommendations-for-new-examples)
7. [Simplification Opportunities](#6-simplification-opportunities)

---

## Quick Reference

### By Difficulty Level

| Level | Count | Examples |
|-------|-------|----------|
| **Beginner** | 5 | Basic MQTT clients, REST API CLI |
| **Intermediate** | 6 | SSO integrations, device simulators |
| **Advanced** | 3 | QUIC protocol, complete OAuth2 implementation |

### By Programming Language

| Language | Examples Count | Categories |
|----------|----------------|------------|
| Python | 8 | SSO, MQTT, REST API |
| JavaScript/TypeScript | 4 | MQTT, REST API |
| C/C++ | 3 | MQTT (including QUIC) |
| Rust | 2 | MQTT |
| Dart/Flutter | 1 | SSO Mobile |
| Node-RED | 1 | MQTT (Visual) |

---

## 1. Security / Identity-SSO

Location: `examples/security/identity-sso/`

| Example | Language | Difficulty | Lines | Description |
|---------|----------|------------|-------|-------------|
| **keycloak-python** | Python 3.9+ | **Advanced** | 470 | Complete OAuth2 + PKCE implementation with CLI & FastAPI server |
| **keycloak-flutter** | Dart/Flutter | **Intermediate** | 350 | Mobile SSO with secure storage and token refresh |
| **keycloak-react** | TypeScript/React | **Intermediate** | 280 | Web SSO with PKCE and role-based access control |

### Feature Comparison

| Feature | Python | Flutter | React |
|---------|--------|---------|-------|
| **OAuth2 Flow** | Auth Code + PKCE, Device Code, Client Credentials | Auth Code + PKCE | Auth Code + PKCE |
| **Token Storage** | File (encrypted), Keyring, Memory | Keychain (iOS), Keystore (Android) | Browser memory |
| **Auto Token Refresh** | Yes | Yes | Yes |
| **CLI Interface** | Yes (Typer + Rich) | No | No |
| **API Server** | Yes (FastAPI) | No | No |
| **Platform** | Desktop/Server | iOS/Android | Web |

### Languages Used
- **Python** - Backend/CLI implementation
- **Dart** - Mobile (Flutter)
- **TypeScript** - Web (React)

### Missing Languages
- Go, Rust, Java, Kotlin (Android native), Swift (iOS native)

---

## 2. MQTT Integration

Location: `examples/integrations/mqtt-integration/`

### 2.1 WebSocket Streaming (WSS)

| Example | Language | Difficulty | Lines | Description |
|---------|----------|------------|-------|-------------|
| **wss-live-streaming/python** | Python 3.9+ | **Beginner** | 318 | MQTT over WebSocket with token authentication |
| **wss-live-streaming/nodejs** | Node.js 18+ | **Beginner** | 246 | MQTT.js with WSS transport |
| **wss-live-streaming/rust** | Rust 1.70+ | **Intermediate** | 229 | Async MQTT with rumqttc |
| **wss-live-streaming/c** | C11 | **Intermediate** | 454 | libmosquitto with WebSocket support |
| **wss-live-streaming/nodered** | Node-RED | **Beginner** | JSON | Visual flow-based MQTT client |

### 2.2 Advanced MQTT

| Example | Language | Difficulty | Lines | Description |
|---------|----------|------------|-------|-------------|
| **mqtt-quic-c** | C11 | **Advanced** | 380 | MQTT over QUIC protocol (UDP-based) |
| **edge-ai-device-simulator** | Python 3.9+ | **Intermediate** | 344 | Device simulator with anomaly injection for Edge AI testing |

### Security Comparison (TLS/SSL)

| Language | TLS Verification | Production Ready | Notes |
|----------|------------------|------------------|-------|
| **Python** | ❌ Disabled by default | No | `cert_reqs=ssl.CERT_NONE` |
| **Node.js** | ❌ Disabled | No | `rejectUnauthorized: false` |
| **Rust** | ✅ System default | **Yes** | Uses native-tls defaults |
| **C** | ❌ Disabled | No | `mosquitto_tls_insecure_set()` |

### Languages Used
- **Python** - WSS streaming, device simulator
- **JavaScript** - Node.js WSS, Node-RED
- **Rust** - WSS streaming
- **C** - WSS streaming, QUIC protocol

### Missing Languages
- Go, Java, C#, MicroPython

---

## 3. Applications (REST API)

Location: `examples/applications/rest-api/` (symlink to `examples/embedded-devices/entry/python-cli/`)

| Example | Language | Difficulty | Lines | Description |
|---------|----------|------------|-------|-------------|
| **python-cli** | Python 3.9+ | **Beginner** | 650 | REST API client with Typer CLI for TESAIoT Platform |

### Features

| Feature | Description |
|---------|-------------|
| **Authentication** | API Key via `X-API-Key` header |
| **CLI Commands** | `devices list`, `devices show`, `devices telemetry`, `devices stream`, `stats summary` |
| **Real-time** | WebSocket streaming for live telemetry |
| **Time Ranges** | `--since`, `--start`, `--end` filters for telemetry queries |
| **Configuration** | YAML config + Environment variables |

### API Endpoints Used
- `GET /devices` - List devices
- `GET /devices/{id}` - Device details
- `GET /devices/{id}/telemetry` - Historical telemetry
- `GET /devices/schemas/{type}` - Schema definitions
- `WS /ws/telemetry` - Real-time telemetry stream

### Languages Used
- **Python** - Only implementation available

### Missing Languages
- JavaScript/Node.js, Go, Rust, Java

---

## 4. Language Coverage Summary

### Current Matrix

| Language | SSO | MQTT | REST API | **Total** |
|----------|-----|------|----------|-----------|
| **Python** | ✅ (Advanced) | ✅ (2 examples) | ✅ (Beginner) | **4** |
| **JavaScript** | ❌ | ✅ (2 examples) | ❌ | **2** |
| **TypeScript** | ✅ (React) | ❌ | ❌ | **1** |
| **Dart/Flutter** | ✅ (Mobile) | ❌ | ❌ | **1** |
| **Rust** | ❌ | ✅ (1 example) | ❌ | **1** |
| **C** | ❌ | ✅ (2 examples) | ❌ | **2** |
| **Node-RED** | ❌ | ✅ (1 example) | ❌ | **1** |

### Coverage Analysis

| Category | Python Coverage | Other Languages | Priority to Add |
|----------|-----------------|-----------------|-----------------|
| **Security/SSO** | ✅ Complete | ❌ None | Go, Rust |
| **MQTT** | ✅ WSS + Simulator | JS, Rust, C, Node-RED | Go, Java |
| **REST API** | ✅ CLI Only | ❌ None | Node.js, Go |

---

## 5. Recommendations for New Examples

### High Priority

#### 1. MQTT Go Client
**Category:** MQTT Integration  
**Language:** Go  
**Difficulty:** Intermediate  
**Estimated Lines:** 250

**Why:** Go is popular for backend services and IoT gateways. Currently missing from MQTT examples.

**Features:**
- WSS connection with token auth
- Concurrent message handling with goroutines
- Auto-reconnection with exponential backoff
- JSON message processing

**Library:** `paho.mqtt.golang`

---

#### 2. REST API Node.js Client
**Category:** Applications (REST API)  
**Language:** JavaScript/Node.js  
**Difficulty:** Beginner  
**Estimated Lines:** 200

**Why:** Many developers prefer JavaScript for API integration. Complements the Python CLI.

**Features:**
- Commander.js or native CLI
- Device listing and telemetry fetch
- Environment-based configuration
- Simple table output formatting

---

#### 3. SSO Go Implementation
**Category:** Security/Identity-SSO  
**Language:** Go  
**Difficulty:** Intermediate  
**Estimated Lines:** 400

**Why:** Go is commonly used for backend microservices that need SSO integration.

**Features:**
- Authorization Code + PKCE flow
- JWT token validation middleware
- Gin or Echo framework integration
- Secure cookie-based token storage

---

### Medium Priority

#### 4. MQTT MicroPython (ESP32)
**Category:** MQTT Integration  
**Language:** MicroPython  
**Difficulty:** Beginner  
**Estimated Lines:** 150

**Why:** ESP32 is widely used for IoT; MicroPython lowers entry barrier compared to C.

**Hardware:** ESP32 DevKit  
**Features:**
- WiFi connection management
- MQTT publish with token auth
- DHT22 temperature/humidity sensor reading
- Deep sleep for power saving

---

#### 5. REST API Rust Client
**Category:** Applications (REST API)  
**Language:** Rust  
**Difficulty:** Intermediate  
**Estimated Lines:** 350

**Why:** Type-safe API client for production use.

**Features:**
- Struct-based request/response types
- Async/await with tokio
- Retry logic with exponential backoff
- CLI with clap

---

#### 6. SSO Kotlin (Android Native)
**Category:** Security/Identity-SSO  
**Language:** Kotlin  
**Difficulty:** Intermediate  
**Estimated Lines:** 350

**Why:** Native Android alternative to Flutter.

**Features:**
- AppAuth-Android for OAuth2
- Encrypted SharedPreferences
- Auto token refresh
- Biometric auth option

---

### Low Priority

#### 7. MQTT Java Client
**Category:** MQTT Integration  
**Language:** Java  
**Difficulty:** Intermediate  
**Estimated Lines:** 300

**Why:** Enterprise environments often use Java.

**Library:** Eclipse Paho Java

---

#### 8. REST API Go Client
**Category:** Applications (REST API)  
**Language:** Go  
**Difficulty:** Intermediate  
**Estimated Lines:** 300

**Why:** Many DevOps tools and backends are written in Go.

**Features:**
- Cobra CLI framework
- Structured logging
- Configuration with Viper

---

## 6. Simplification Opportunities

### Advanced Examples That Need Beginner Versions

#### 1. keycloak-python → "keycloak-python-simple"

**Current (Advanced - 470 lines):**
- Multiple OAuth flows (PKCE, Device Code, Client Credentials)
- Multiple storage backends (file, keyring, memory)
- Rich CLI output
- FastAPI server included

**Proposed Beginner Version (150 lines):**
```python
# hello-sso-python
"""Simple SSO example - Get token and call API"""
import requests
from urllib.parse import urlencode
import webbrowser
from http.server import HTTPServer, BaseHTTPRequestHandler

# 1. Generate PKCE
# 2. Open browser
# 3. Wait for callback
# 4. Exchange code for token
# 5. Call API with token
```

**Target Audience:** First-time OAuth developers  
**Key Difference:** Single flow only, no CLI framework, minimal dependencies

---

#### 2. mqtt-quic-c → "mqtt-basic-c"

**Current (Advanced - 380 lines):**
- QUIC protocol (UDP-based)
- TLS 1.3 configuration
- Complex connection handling

**Proposed Beginner Version (100 lines):**
```c
/* hello-mqtt-c - Basic MQTT over TCP */
#include <mosquitto.h>
// Connect to broker
// Subscribe to topic
// Print received messages
```

**Target Audience:** Embedded developers new to MQTT  
**Key Difference:** TCP instead of QUIC, no TLS complexity

---

### Suggested New "Hello" Examples

| Example | Category | Language | Purpose | Lines |
|---------|----------|----------|---------|-------|
| hello-mqtt-python | MQTT | Python | Connect + publish one message | 30 |
| hello-mqtt-js | MQTT | JavaScript | Browser MQTT with MQTT.js | 40 |
| hello-sso-python | SSO | Python | Get token + call one API | 50 |
| hello-rest-api | REST API | Python | Fetch devices list | 40 |

---

## Summary

### Current State
- **14 examples** across **3 categories**
- **7 programming languages** supported
- **Python has best coverage** (4/5 areas)
- **Rust is production-ready** for MQTT (only one with proper TLS)

### Key Gaps
1. **No Go examples** in any category (high demand language)
2. **No Java** for MQTT (enterprise language)
3. **REST API** only has Python (needs Node.js, Go)
4. **SSO** missing backend languages (Go, Rust)

### Target Metrics

| Metric | Current | Target |
|--------|---------|--------|
| Total Examples | 14 | 20+ |
| Languages | 7 | 9+ |
| Beginner Examples | 5 | 10+ |
| Production-Ready | 3 | 10+ |

---

**Document Maintainers:** TESAIoT Developer Relations  
**Last Updated:** 2026-04-08
