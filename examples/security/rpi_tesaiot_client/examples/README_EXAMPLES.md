# TESAIoT v3.0.0 Example Codes

**(c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.**

---

## Overview

These examples demonstrate how to use the TESAIoT Secure Library v3.0.0 crypto utility functions for real IoT applications. Each example maps to practical use cases from the OID analysis document.

## Prerequisites

- Raspberry Pi with OPTIGA Trust M connected via I2C
- TESAIoT Secure Library v3.0.0 (`libtesaiot.a`)
- Valid device license from TESAIoT Platform

## Examples

| # | File | Phase | Use Cases | Functions Demonstrated |
|---|------|-------|-----------|----------------------|
| 01 | `example_01_random_and_store.c` | 1 | A1, A2, A3 | `random_generate`, `secure_store_write/read` |
| 02 | `example_02_aes_encrypt_sensor.c` | 1 | D5 | `aes_generate_key`, `aes_encrypt/decrypt` |
| 03 | `example_03_hmac_mqtt_integrity.c` | 1 | B2 | `hmac_sha256`, `random_generate`, `secure_store_write` |
| 04 | `example_04_ecdh_device_pairing.c` | 2 | B3 | `ecdh_shared_secret`, `hkdf_derive` |
| 05 | `example_05_sign_verify_telemetry.c` | 2 | B4, C4, D1 | `sign_data`, `verify_data` |
| 06 | `example_06_health_monitor.c` | 3 | D1, D2, D3 | `health_check`, `counter_read/increment` |

## Use Case Reference

| Code | Category | Description |
|------|----------|-------------|
| A1 | Storage | WiFi credential storage in OPTIGA |
| A2 | Storage | MQTT broker credentials (write-only vault) |
| A3 | Storage | Application JSON config |
| B2 | Communication | End-to-end encrypted MQTT payloads |
| B3 | Communication | Device-to-device secure channel (ECDH) |
| B4 | Communication | Signed MQTT telemetry (non-repudiation) |
| C4 | Identification | Peer authentication (challenge-response) |
| D1 | Security Ops | Anti-replay protection (monotonic counter) |
| D2 | Security Ops | Firmware anti-rollback |
| D3 | Security Ops | Usage metering / pay-per-use |
| D5 | Security Ops | Data encryption at rest |

## Application Patterns

### Pattern 1: Smart Home Sensor
- WiFi creds in slot 0, MQTT creds in slot 1
- Sign sensor data for non-repudiation
- Anti-replay counter for message sequencing

### Pattern 2: Industrial Gateway
- ECDH device-to-device pairing with sensor nodes
- AES encrypted data cache on edge
- Firmware anti-rollback counter

### Pattern 3: Medical IoT Device
- Signed vital signs (FDA audit trail)
- Hash chain with monotonic counter (tamper-evident)
- Usage metering for subscription model

## How to Build

```bash
cd /home/wiroon/TESAIoT_Mission/tesaiot_library

# Build example (link with libtesaiot.a)
gcc -Wall -Wextra -std=c11 -O2 \
    -I./include \
    -I./external/trustm/include \
    -o example_01 examples/example_01_random_and_store.c \
    -L./lib -ltesaiot \
    -L./external/trustm/lib -ltrustm \
    -L./external/openssl/lib -lssl -lcrypto \
    -lpthread -lgpiod
```

## How to Extend

1. Copy any example as a starting template
2. Combine functions from different examples
3. Follow the existing pattern: `init -> verify_license -> use crypto -> deinit`
4. Refer to `tesaiot_crypto.h` for Phase 1-2 APIs
5. Refer to `tesaiot_advanced.h` for Phase 3 APIs

## OID Safety Reminder

**NEVER write to these OIDs from application code:**

| OID | Reason |
|-----|--------|
| 0xE0E0 | Factory Certificate |
| 0xE0E1 | Device Certificate (CSR/PU target) |
| 0xE0E8 | Trust Anchor (PU verification) |
| 0xE0F0 | Factory Key |
| 0xE0F1 | Device Key (use for sign only) |
| 0xF1D4 | Protected Update Shared Secret |

---

*(c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.*
