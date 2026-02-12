# TESAIoT v3.0.0 - Developer Crypto Utility Examples

Practical IoT application patterns using the TESAIoT crypto utility API (`tesaiot_crypto.h`).

All examples use **only the public `tesaiot.h` API** and can be distributed alongside the pre-built `libtesaiot.a` library.

## Quick Start

Add to your `main.c`:

```c
#include "examples/examples.h"

// In your menu handler:
case '6': example_A1_secure_credential_store(); break;
case '7': example_B1_hmac_mqtt_payload(); break;
case '8': example_E1_smart_sensor_complete(); break;
```

## Example Categories

### Category A: Secure Storage

| Example | Description | Functions Used |
|---------|-------------|----------------|
| **A1** - Secure Credential Store | Store WiFi, API tokens in OPTIGA hardware vault | `tesaiot_secure_store_write/read()`, `tesaiot_random_generate()` |
| **A2** - Data Encryption at Rest | Encrypt sensor data before flash storage | `tesaiot_aes_generate_key()`, `tesaiot_aes_encrypt/decrypt()` |

### Category B: Secure Communication

| Example | Description | Functions Used |
|---------|-------------|----------------|
| **B1** - HMAC MQTT Payload | Sign MQTT payloads for integrity | `tesaiot_hmac_sha256()`, `tesaiot_secure_store_write()` |
| **B2** - E2E Encrypted MQTT | Encrypt-then-MAC for end-to-end security | `tesaiot_aes_encrypt()`, `tesaiot_hmac_sha256()` |
| **B3** - Device-to-Device Channel | ECDH key agreement + HKDF + AES session | `tesaiot_ecdh_shared_secret()`, `tesaiot_hkdf_derive()`, `tesaiot_aes_*()` |

### Category C: Identification & Attestation

| Example | Description | Functions Used |
|---------|-------------|----------------|
| **C1** - Signed Telemetry | Non-repudiation signed sensor data | `tesaiot_optiga_hash()`, `tesaiot_sign_data()` |
| **C2** - Challenge-Response Auth | Peer authentication protocol | `tesaiot_random_generate()`, `tesaiot_sign_data()` |

### Category D: Security Operations

| Example | Description | Functions Used |
|---------|-------------|----------------|
| **D1** - Anti-Replay Counter | Monotonic counter for replay protection | `tesaiot_counter_read/increment()`, `tesaiot_sign_data()` |
| **D2** - Health Dashboard | Comprehensive device diagnostics | `tesaiot_health_check()`, `tesaiot_counter_read()` |

### Category E: Complete Application Patterns

| Example | Description | Functions Used |
|---------|-------------|----------------|
| **E1** - Smart Sensor Complete | Full secure IoT flow (all functions combined) | All 10+ crypto functions |

## OID Reference

| OID | Purpose | Used By Examples |
|-----|---------|------------------|
| 0xF1D0-0xF1D3 | Data storage (140B each) | A1 |
| 0xF1D4 | RESERVED (Protected Update) | - |
| 0xF1D5-0xF1DB | Data storage (140B each) | B1, B2, E1 |
| 0xF1E0-0xF1E1 | Large data (1500B each) | A1 |
| 0xE200 | AES symmetric key | A2, B2, B3, E1 |
| 0xE0F1 | Device private key | C1, C2, D1, E1 |
| 0xE0F2 | Application key | B3 |
| 0xE120-0xE123 | Monotonic counters | D1, D2, E1 |

## Prerequisites

- TESAIoT Library v3.0.0 (`libtesaiot.a`)
- Valid license (`tesaiot_license_init()` must succeed)
- OPTIGA Trust M initialized
- For signing examples (C1, C2, D1): Device Key in OID 0xE0F1 (run CSR workflow)
- For ECDH example (B3): ECC key pair in OID 0xE0F2

## Security Properties by Example

| Property | A1 | A2 | B1 | B2 | B3 | C1 | C2 | D1 | D2 | E1 |
|----------|----|----|----|----|----|----|----|----|----|----|
| Confidentiality | | x | | x | x | | | | | x |
| Integrity | | | x | x | | | | | | x |
| Authenticity | | | x | x | | x | x | x | | x |
| Non-repudiation | | | | | | x | | x | | x |
| Anti-replay | | | | | | | | x | | x |
| Key protection | x | x | x | x | x | x | x | x | | x |

---

*TESAIoT v3.0.0 - Developer Crypto Utilities*
*Copyright (c) 2025-2026 TESAIoT AIoT Foundation Platform*
