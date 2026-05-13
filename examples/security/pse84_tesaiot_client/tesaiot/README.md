# TESAIoT Library

**Hardware-Secured IoT Device Provisioning for PSoC Edge + OPTIGA Trust M**

Version: 3.0.0
Last Updated: 2026-02-08

## Overview

TESAIoT Library provides secure device provisioning and certificate management for IoT devices using Infineon PSoC Edge and OPTIGA Trust M secure element.

### Features

- **License Key System** - ECDSA signature-based device licensing
- **Developer Crypto Utilities** (v3.0.0) - 14 hardware-accelerated crypto functions
- **Protected Update Only** - Certificate renewal via Protected Update mechanism
- **MQTT with mTLS** - Secure MQTT communication with mutual TLS authentication
- **OPTIGA Integration** - Full integration with OPTIGA Trust M secure element
- **Protected Update** - Secure firmware/certificate update workflow
- **SNTP Client** - Time synchronization for certificate validation

---

## Header Files

The library provides 10 header files:

| Header | Description |
|--------|-------------|
| `tesaiot.h` | Main umbrella - includes all headers + license API |
| `tesaiot_config.h` | Configuration, debug levels, OID definitions |
| `tesaiot_crypto.h` | **Developer Crypto Utilities API** (NEW v3.0.0) |
| `tesaiot_license.h` | License verification API |
| `tesaiot_license_config.h` | **Customer editable** - Your UID + license key |
| `tesaiot_optiga.h` | OPTIGA Trust M integration |
| `tesaiot_optiga_core.h` | OPTIGA manager (init, acquire, release) |
| `tesaiot_platform.h` | Platform services (MQTT, SNTP) |
| `tesaiot_protected_update.h` | Protected Update workflow |

### Usage

```c
// Option 1: Single include (recommended)
#include "tesaiot.h"

// Option 2: Domain-specific includes
#include "tesaiot_platform.h"  // MQTT, SNTP
#include "tesaiot_optiga.h"    // OPTIGA Trust M
```

---

## License Key Configuration (v3.0.0)

### 3-Layer Architecture

**Layer 1: Configuration (Customer Edits)**

Edit `tesaiot/include/tesaiot_license_config.h`:

```c
#define TESAIOT_DEVICE_UID    "your_device_uid_here"
#define TESAIOT_LICENSE_KEY   "your_license_key_here"
```

**Layer 2: Data Binding (Customer Compiles)**

`tesaiot/src/tesaiot_license_data.c` - Link-time binding:

```c
#include "tesaiot_license_config.h"

const char* tesaiot_device_uid = TESAIOT_DEVICE_UID;
const char* tesaiot_license_key = TESAIOT_LICENSE_KEY;
```

**Layer 3: Verification Logic (IP-Protected)**

`tesaiot/src/tesaiot_license.c` - Compiled into `libtesaiot.a`:
- Embedded public key (customer never sees it)
- ECDSA signature verification
- UID verification with OPTIGA

### How to Get Your License

1. Build and flash the project (first time)
2. Run **Print factory UID** (Menu 1) to get your device's OPTIGA Trust M UID
3. Register on TESAIoT Server with the UID
4. Download your `tesaiot_license_config.h`
5. Place in `tesaiot/include/` folder (moved from `proj_cm33_ns/`)
6. Rebuild and flash - Licensed!

### Security Benefits (v3.0.0)

| Aspect | Old (v2.6) | New (v2.8) |
|--------|-----------|-----------|
| **Config Location** | `proj_cm33_ns/` | `tesaiot/include/` |
| **Data Binding** | ❌ None | ✅ Link-time binding |
| **Public Key** | ⚠️ In header? | ✅ Embedded in .c (IP-protected) |
| **Verification Logic** | ⚠️ Distributed | ✅ Compiled in library only |
| **IP Protection** | Partial | ✅ Complete |

---

## API Reference

### License Functions (from tesaiot.h)

| Function | Description |
|----------|-------------|
| `tesaiot_license_init()` | Initialize and verify license |
| `tesaiot_is_licensed()` | Check if library is licensed |
| `tesaiot_get_device_uid()` | Get device OPTIGA UID |
| `tesaiot_print_device_uid()` | Print UID for registration |
| `tesaiot_license_status_str()` | Get human-readable status |

### Platform Functions (from tesaiot_platform.h)

| Function | Description |
|----------|-------------|
| `tesaiot_mqtt_connect()` | Connect to MQTT broker |
| `tesaiot_mqtt_is_connected()` | Check MQTT connection status |
| `tesaiot_sntp_sync_time()` | Synchronize time via NTP |
| `tesaiot_sntp_get_time()` | Get current Unix timestamp |
| `tesaiot_sntp_is_time_synced()` | Check if time is synced |

### Crypto Utility Functions (from tesaiot_crypto.h) - NEW v3.0.0

| Function | Description |
|----------|-------------|
| `tesaiot_random_generate()` | Hardware TRNG random bytes (CC EAL6+) |
| `tesaiot_secure_store_write()` | Write to OPTIGA data object (14 slots) |
| `tesaiot_secure_store_read()` | Read from OPTIGA data object |
| `tesaiot_aes_generate_key()` | Generate AES key in hardware (128/192/256) |
| `tesaiot_aes_encrypt()` | AES-CBC encrypt (key never leaves OPTIGA) |
| `tesaiot_aes_decrypt()` | AES-CBC decrypt |
| `tesaiot_hmac_sha256()` | HMAC-SHA256 with hardware key |
| `tesaiot_ecdh_shared_secret()` | ECDH P-256 shared secret derivation |
| `tesaiot_hkdf_derive()` | HKDF-SHA256 key derivation (RFC 5869) |
| `tesaiot_optiga_hash()` | SHA-256 hardware hash |
| `tesaiot_sign_data()` | SHA-256 + ECDSA composite signing |
| `tesaiot_counter_read()` | Read monotonic counter (anti-replay) |
| `tesaiot_counter_increment()` | Increment monotonic counter |
| `tesaiot_health_check()` | Comprehensive device health check |

### OPTIGA Functions (from tesaiot_optiga.h)

| Function | Description |
|----------|-------------|
| `tesaiot_optiga_generate_keypair()` | Generate keypair |
| `tesaiot_optiga_write_cert()` | Write certificate |
| `tesaiot_optiga_get_cert_oid()` | Get current cert OID |

---

## Usage Example

```c
#include "tesaiot.h"

void app_init(void) {
    // 1. Initialize OPTIGA first
    optiga_manager_init();

    // 2. Verify license
    tesaiot_license_status_t status = tesaiot_license_init();
    if (status != TESAIOT_LICENSE_OK) {
        printf("License error: %s\n", tesaiot_license_status_str(status));
        return;
    }

    printf("License verified!\n");

    // 3. Use library functions
    tesaiot_mqtt_connect();
}
```

---

## Security Architecture

**Security Level: Very High** - Hardware-backed security with OPTIGA Trust M secure element.

### Why "Very High" Security?

| Security Aspect | Protection | Mechanism |
|-----------------|------------|-----------|
| **Device Identity (UID)** | Hardware-bound | Factory-programmed in OPTIGA, read-only |
| **Private Keys** | Never extractable | Generated and stored inside OPTIGA, signing happens on-chip |
| **mTLS Authentication** | Hardware-backed | Private key never leaves OPTIGA during TLS handshake |
| **License Verification** | Cryptographically signed | ECDSA P-256 signature verified with public key |

### What Makes This Secure?

1. **Hardware Root of Trust**
   - OPTIGA Trust M is CC EAL6+ certified secure element
   - Tamper-resistant hardware with secure key storage
   - Factory UID cannot be modified or cloned

2. **Private Key Protection**
   - Keys generated inside OPTIGA, never exported
   - All cryptographic operations happen on-chip
   - Even firmware cannot read the private key bytes

3. **License Key is NOT a Secret**
   - License key is an ECDSA signature (public data)
   - Verification uses embedded public key
   - The secret (signing key) stays on TESAIoT Server

4. **mTLS Mutual Authentication**
   - Both device and broker verify each other
   - Device proves identity using hardware-bound key
   - Man-in-the-middle attacks are prevented

---

## Platform Support

| Component | Specification |
|-----------|---------------|
| MCU | Infineon PSoC Edge E84 (Cortex-M33) |
| Secure Element | OPTIGA Trust M |
| RTOS | FreeRTOS |
| Network | lwIP |
| TLS | mbedTLS |

---

## Support

For licensing and technical support:

- Email: support@tesaiot.com
- Website: https://tesaiot.com

---

## Authors

**Assoc. Prof. Wiroon Sriborrirux (BDH)**

- Thai Embedded Systems Association (TESA)
- TESAIoT Platform Creator
- Email: sriborrirux@gmail.com / wiroon@tesa.or.th

**TESAIoT Platform Developer Team**

- In collaboration with Infineon Technologies AG

---

## Copyright

(c) 2025-2026 TESAIoT AIoT Foundation Platform. All rights reserved.

Developed by Assoc. Prof. Wiroon Sriborrirux (BDH) and Thai Embedded Systems Association (TESA).

This library is protected by hardware-bound licensing. Unauthorized use, copying, or distribution is prohibited.
