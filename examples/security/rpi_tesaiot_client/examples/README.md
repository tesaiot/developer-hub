# TESAIoT Client Examples - Developer Guide

คู่มือนี้สำหรับ Developer ที่ต้องการพัฒนาต่อยอดบน TESAIoT Library

**Version**: 3.0.0
**Last Updated**: 2026-02-08
**Copyright**: Thai Embedded Systems Association (TESA)

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Project Structure](#project-structure)
4. [API Reference](#api-reference)
5. [Building Examples](#building-examples)
6. [Running Examples](#running-examples)
7. [Integration Guide](#integration-guide)
8. [OID Reference](#oid-reference)
9. [Error Codes](#error-codes)
10. [Troubleshooting](#troubleshooting)
11. [Security Considerations](#security-considerations)

---

## Overview

TESAIoT Library (`libtesaiot.a`) เป็น Static Library สำหรับ Raspberry Pi และ Embedded Linux ที่รองรับ:

| Workflow | Description |
|----------|-------------|
| **CSR Workflow** | Certificate Signing Request - ขอ Device Certificate จาก TESAIoT Platform |
| **Protected Update** | Secure Firmware/Data Update - อัพเดตข้อมูลแบบปลอดภัยผ่าน CBOR COSE_Sign1 |
| **Crypto Utilities** | ฟังก์ชันเข้ารหัส: ECDSA sign/verify, SHA-256, AES-CBC, keygen, RNG (v3.0.0) |
| **Health & Diagnostics** | ตรวจสอบสถานะ: health check, cert info, OID metadata, counters (v3.0.0) |

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Your Application                         │
├─────────────────────────────────────────────────────────────┤
│                  TESAIoT Library (libtesaiot.a)             │
│  ┌─────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │ tesaiot.h   │  │ tesaiot_csr.h   │  │ tesaiot_pu.h    │  │
│  │ License API │  │ CSR Workflow    │  │ Protected Update│  │
│  └─────────────┘  └─────────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│  OPTIGA Trust M    │    Paho MQTT     │    OpenSSL/TLS      │
│  (libtrustm.so)    │ (libpaho-mqtt)   │ (trustm_provider)   │
└─────────────────────────────────────────────────────────────┘
```

---

## Prerequisites

### Hardware Requirements

- Raspberry Pi 4/5 (aarch64) หรือ Raspberry Pi 3 (armhf)
- OPTIGA Trust M chip (I2C: 0x30 @ /dev/i2c-1)

### Software Requirements

```bash
# System libraries
sudo apt-get install -y \
    libssl-dev \
    libpaho-mqtt-dev \
    libpaho-mqttpp-dev

# OPTIGA Trust M libraries (bundled or system)
# - libtrustm.so
# - trustm_provider.so (OpenSSL provider)
```

### Bundled Libraries

ใน `../lib/` มี libraries ที่จำเป็น:

| File | Description |
|------|-------------|
| `libtesaiot.a` | TESAIoT Static Library |
| `libtrustm.so` | OPTIGA Trust M Library |
| `trustm_provider.so` | OpenSSL Provider for Trust M |
| `libpaho_shim.so` | TLS Shim for Paho MQTT + Trust M |

---

## Project Structure

```
Portable_Deployment/
├── bin/                          # Pre-built binaries
│   ├── tesaiot_csr_client        # CSR Workflow client
│   ├── tesaiot_pu_client         # Protected Update client
│   └── trustm/                   # Trust M CLI tools
├── lib/                          # Libraries
│   ├── libtesaiot.a              # TESAIoT static library
│   ├── libtrustm.so              # Trust M shared library
│   ├── trustm_provider.so        # OpenSSL provider
│   └── libpaho_shim.so           # MQTT TLS shim
├── include/tesaiot/              # Header files
│   ├── tesaiot.h                 # Core API (license, init)
│   ├── tesaiot_csr.h             # CSR Workflow API
│   ├── tesaiot_protected_update.h # Protected Update API
│   ├── tesaiot_crypto.h          # Crypto utilities (v3.0.0)
│   ├── tesaiot_advanced.h        # Health, diag, counters (v3.0.0)
│   ├── tesaiot_config.h          # Device configuration
│   └── tesaiot_optiga.h          # Low-level OPTIGA API
├── scripts/                      # Runner scripts
│   ├── run_csr_workflow.sh       # Run CSR with proper env
│   └── run_pu_workflow.sh        # Run PU with proper env
└── examples/                     # Example source code
    ├── csr_client_example.c      # CSR client example
    ├── pu_client_example.c       # PU client example
    └── README.md                 # This file
```

---

## API Reference

### Core API (`tesaiot.h`)

#### Initialization

```c
#include <tesaiot.h>

// Initialize library (must call first)
int tesaiot_init(void);

// Deinitialize library (call on exit)
int tesaiot_deinit(void);
```

#### License Verification

```c
// Verify device license (REQUIRED before workflows)
int tesaiot_verify_license(void);

// Get license status
tesaiot_license_status_t tesaiot_get_license_status(void);

// License status values:
typedef enum {
    TESAIOT_LICENSE_VALID = 0,
    TESAIOT_LICENSE_INVALID_UID,
    TESAIOT_LICENSE_INVALID_SIGNATURE,
    TESAIOT_LICENSE_TRUSTM_ERROR,
    TESAIOT_LICENSE_PARSE_ERROR,
    TESAIOT_LICENSE_NOT_VERIFIED
} tesaiot_license_status_t;
```

#### Device Information

```c
// Get device info structure
typedef struct {
    char uid[55];                    // Device UID (54 hex chars)
    const char *device_id;           // Device UUID
    char license_key[256];           // License key (Base64)
    tesaiot_license_status_t status;
    bool initialized;
} tesaiot_device_info_t;

int tesaiot_get_device_info(tesaiot_device_info_t *info);

// Read Trust M UID directly
int tesaiot_read_trustm_uid(char *uid_hex, size_t uid_hex_size);
```

#### Logging

```c
// Set log level
typedef enum {
    TESAIOT_LOG_LEVEL_ERROR = 0,
    TESAIOT_LOG_LEVEL_WARN,
    TESAIOT_LOG_LEVEL_INFO,
    TESAIOT_LOG_LEVEL_DEBUG
} tesaiot_log_level_t;

void tesaiot_set_log_level(tesaiot_log_level_t level);
```

---

### CSR Workflow API (`tesaiot_csr.h`)

#### State Machine

```c
// CSR Workflow states
typedef enum {
    TESAIOT_CSR_STATE_IDLE = 0,
    TESAIOT_CSR_STATE_GENERATE_KEYPAIR,  // Generate ECC P-256 at OID 0xE0F1
    TESAIOT_CSR_STATE_GENERATE_CSR,      // Create X.509 CSR
    TESAIOT_CSR_STATE_CONNECT_MQTT,      // Connect with Factory Certificate
    TESAIOT_CSR_STATE_PUBLISH_CSR,       // Publish to device/{id}/commands/csr
    TESAIOT_CSR_STATE_WAIT_CERTIFICATE,  // Wait for signed certificate
    TESAIOT_CSR_STATE_DONE,
    TESAIOT_CSR_STATE_ERROR
} tesaiot_csr_state_t;
```

#### Configuration

```c
typedef struct {
    char device_id[64];              // Device UUID (MQTT topic routing)
    char device_uid[128];            // Trust M UID (MQTT client ID)
    char mqtt_broker_url[256];       // e.g., "ssl://mqtt.tesaiot.com:8883"
    char ca_cert_path[256];          // TESAIoT CA certificate
    char factory_cert_path[256];     // Factory certificate OID
    char factory_key_path[256];      // Factory key OID
    uint16_t target_oid;             // Target OID (0xE0E1-0xE0E3)
} tesaiot_csr_workflow_config_t;
```

#### Functions

```c
// Blocking wrapper (recommended)
int tesaiot_csr_workflow_run_blocking(const tesaiot_csr_workflow_config_t *config);

// Non-blocking API (advanced)
int tesaiot_csr_workflow_init(const tesaiot_csr_workflow_config_t *config);
int tesaiot_csr_workflow_start(void);
int tesaiot_csr_workflow_run(tesaiot_csr_context_t *context);
tesaiot_csr_state_t tesaiot_csr_workflow_get_state(void);

// Certificate notification (called by MQTT callback)
int tesaiot_csr_workflow_notify_certificate(const char *pem_cert, size_t pem_len);
```

#### Example Usage

```c
#include <tesaiot.h>
#include <tesaiot_csr.h>
#include <tesaiot_config.h>

int main(void)
{
    int ret;

    // 1. Initialize library
    ret = tesaiot_init();
    if (ret != 0) {
        fprintf(stderr, "Init failed: %d\n", ret);
        return 1;
    }

    // 2. Verify license (REQUIRED)
    ret = tesaiot_verify_license();
    if (ret != 0) {
        fprintf(stderr, "License invalid\n");
        tesaiot_deinit();
        return 1;
    }

    // 3. Configure CSR workflow
    tesaiot_csr_workflow_config_t config = {0};
    snprintf(config.device_id, sizeof(config.device_id), "%s", TESAIOT_DEVICE_ID);
    snprintf(config.device_uid, sizeof(config.device_uid), "%s", TESAIOT_DEVICE_UID);
    snprintf(config.mqtt_broker_url, sizeof(config.mqtt_broker_url), "%s", TESAIOT_MQTT_BROKER_URL);
    snprintf(config.ca_cert_path, sizeof(config.ca_cert_path), "%s", TESAIOT_CA_CERT_PATH);
    snprintf(config.factory_cert_path, sizeof(config.factory_cert_path), "%s", TESAIOT_FACTORY_CERT_OID);
    snprintf(config.factory_key_path, sizeof(config.factory_key_path), "%s", TESAIOT_FACTORY_KEY_OID);
    config.target_oid = 0xE0E2;  // Target certificate slot

    // 4. Run CSR workflow (blocking)
    ret = tesaiot_csr_workflow_run_blocking(&config);
    if (ret == 0) {
        printf("Certificate enrolled successfully!\n");
    }

    // 5. Cleanup
    tesaiot_deinit();
    return ret == 0 ? 0 : 1;
}
```

---

### Protected Update API (`tesaiot_protected_update.h`)

#### State Machine

```c
typedef enum {
    TESAIOT_PU_STATE_IDLE = 0,
    TESAIOT_PU_STATE_RECEIVE_MANIFEST,   // Receive CBOR manifest via MQTT
    TESAIOT_PU_STATE_VERIFY_MANIFEST,    // Verify COSE_Sign1 signature
    TESAIOT_PU_STATE_RECEIVE_FRAGMENTS,  // Receive data fragments
    TESAIOT_PU_STATE_PROCESS_FRAGMENTS,  // Write to target OID
    TESAIOT_PU_STATE_DONE,
    TESAIOT_PU_STATE_ERROR
} tesaiot_pu_state_t;
```

#### Configuration

```c
typedef struct {
    const char *device_id;
    uint16_t trust_anchor_oid;    // Default: 0xE0E8
    uint16_t secret_oid;          // Default: 0xF1D4 (for encryption)
    bool verify_version;          // Enable version downgrade protection
    uint32_t current_version;
} tesaiot_pu_config_t;
```

#### Functions

```c
// Blocking wrapper (recommended)
int tesaiot_run_protected_update_workflow(void);

// Set target OID override
void tesaiot_pu_set_default_target_oid(uint16_t target_oid);

// Manual Protected Update (advanced)
int tesaiot_protected_update_execute(
    const uint8_t *manifest, uint16_t manifest_len,
    const uint8_t **fragments, uint16_t *fragment_lens,
    uint8_t num_fragments
);

// Trust Anchor management
bool tesaiot_pu_is_trust_anchor_provisioned(void);
int tesaiot_pu_provision_trust_anchor(const uint8_t *cert_der, uint16_t cert_len);

// State query
tesaiot_pu_state_t tesaiot_pu_workflow_get_state(void);
```

#### Example Usage

```c
#include <tesaiot.h>
#include <tesaiot_protected_update.h>

int main(int argc, char *argv[])
{
    int ret;
    uint16_t target_oid = 0;  // 0 = use OID from manifest

    // Parse --target-oid argument
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--target-oid") == 0 && i + 1 < argc) {
            sscanf(argv[i + 1], "0x%hx", &target_oid);
        }
    }

    // 1. Initialize
    ret = tesaiot_init();
    if (ret != 0) return 1;

    // 2. Verify license
    ret = tesaiot_verify_license();
    if (ret != 0) {
        tesaiot_deinit();
        return 1;
    }

    // 3. Set target OID override (optional)
    if (target_oid != 0) {
        tesaiot_pu_set_default_target_oid(target_oid);
    }

    // 4. Run Protected Update workflow
    // - Connects to MQTT
    // - Subscribes to device/{id}/commands/protected_update
    // - Receives manifest + fragments
    // - Writes to target OID
    ret = tesaiot_run_protected_update_workflow();

    // 5. Cleanup
    tesaiot_deinit();
    return ret == 0 ? 0 : 1;
}
```

---

## Building Examples

### Method 1: Using GCC Directly

```bash
cd /path/to/Portable_Deployment/examples

# Build CSR Client
gcc -o csr_client csr_client_example.c \
    -I../include \
    -L../lib -ltesaiot \
    -lpaho-mqtt3cs -lssl -lcrypto -lpthread

# Build PU Client
gcc -o pu_client pu_client_example.c \
    -I../include \
    -L../lib -ltesaiot \
    -lpaho-mqtt3cs -lssl -lcrypto -lpthread
```

### Method 2: Using Makefile

```makefile
# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -I../include
LDFLAGS = -L../lib -ltesaiot -lpaho-mqtt3cs -lssl -lcrypto -lpthread

all: csr_client pu_client

csr_client: csr_client_example.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

pu_client: pu_client_example.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f csr_client pu_client
```

### Cross-Compilation

สำหรับ Cross-compile จาก x86_64 ไปยัง ARM:

```bash
# For aarch64 (RPi 4/5)
aarch64-linux-gnu-gcc -o csr_client csr_client_example.c \
    -I../include -L../lib -ltesaiot \
    -lpaho-mqtt3cs -lssl -lcrypto -lpthread

# For armhf (RPi 3)
arm-linux-gnueabihf-gcc -o csr_client csr_client_example.c \
    -I../include -L../lib -ltesaiot \
    -lpaho-mqtt3cs -lssl -lcrypto -lpthread
```

---

## Running Examples

### Important: Use Runner Scripts

ต้องใช้ runner scripts เพราะต้องตั้งค่า environment variables:

```bash
# CSR Workflow
cd /path/to/Portable_Deployment
./scripts/run_csr_workflow.sh run

# Protected Update
./scripts/run_pu_workflow.sh run
```

### Environment Variables

Runner scripts ตั้งค่าดังนี้:

```bash
# Library path for bundled libraries
export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH

# OpenSSL config for Trust M provider
export TRUSTM_OPENSSL_CONF=/tmp/tesaiot_trustm_openssl.cnf

# Trust M tools path
export TRUSTM_LIB_PATH=/path/to/bin/trustm
export TRUSTM_DATA_PATH=/path/to/bin/trustm/trustm_data

# MQTT TLS shim (required for Trust M + Paho)
export LD_PRELOAD=/path/to/lib/libpaho_shim.so
```

### Command Line Options

```bash
# CSR Workflow
./scripts/run_csr_workflow.sh [command] [options]

Commands:
  run         Run CSR workflow [DEFAULT]
  identity    Print factory UID
  license     Verify license
  help        Show help

Options:
  --target-oid 0xE0E2   Target OID for certificate (0xE0E1-0xE0E3)

# Protected Update
./scripts/run_pu_workflow.sh [command] [options]

Commands:
  run         Run Protected Update workflow [DEFAULT]
  identity    Print factory UID
  license     Verify license
  help        Show help

Options:
  --target-oid 0xE0E2   Override target OID (normally from manifest)
```

---

## Integration Guide

### Step 1: Configure Device

แก้ไข `include/tesaiot/tesaiot_config.h`:

```c
// Device identification
#define TESAIOT_DEVICE_ID       "your-device-uuid"
#define TESAIOT_DEVICE_UID      "your-trust-m-uid-54-hex-chars"

// MQTT broker
#define TESAIOT_MQTT_BROKER_URL "ssl://mqtt.tesaiot.com:8883"

// License key (get from TESAIoT Platform)
#define TESAIOT_LICENSE_KEY     "your-base64-license-key..."

// CA certificate (embedded)
#define TESAIOT_CA_CERTIFICATE  "-----BEGIN CERTIFICATE-----\n..."
```

### Step 2: Link Library

```makefile
# Add to your project's Makefile
LDFLAGS += -L/path/to/lib -ltesaiot
LDFLAGS += -lpaho-mqtt3cs -lssl -lcrypto -lpthread
CFLAGS += -I/path/to/include
```

### Step 3: Initialize in Application

```c
#include <tesaiot.h>
#include <tesaiot_csr.h>
#include <tesaiot_protected_update.h>

int app_init(void)
{
    int ret;

    // Initialize TESAIoT library
    ret = tesaiot_init();
    if (ret != 0) {
        return ret;
    }

    // Verify license before using any workflow
    ret = tesaiot_verify_license();
    if (ret != 0) {
        tesaiot_deinit();
        return ret;
    }

    return 0;
}

void app_cleanup(void)
{
    tesaiot_deinit();
}
```

### Step 4: Implement Workflows

#### CSR Workflow (Certificate Enrollment)

```c
int enroll_certificate(void)
{
    tesaiot_csr_workflow_config_t config = {0};

    // Fill configuration from tesaiot_config.h
    snprintf(config.device_id, sizeof(config.device_id), "%s", TESAIOT_DEVICE_ID);
    snprintf(config.device_uid, sizeof(config.device_uid), "%s", TESAIOT_DEVICE_UID);
    snprintf(config.mqtt_broker_url, sizeof(config.mqtt_broker_url), "%s", TESAIOT_MQTT_BROKER_URL);
    snprintf(config.ca_cert_path, sizeof(config.ca_cert_path), "%s", TESAIOT_CA_CERT_PATH);
    snprintf(config.factory_cert_path, sizeof(config.factory_cert_path), "0xE0E0");
    snprintf(config.factory_key_path, sizeof(config.factory_key_path), "0xE0F0");
    config.target_oid = 0xE0E2;

    return tesaiot_csr_workflow_run_blocking(&config);
}
```

#### Protected Update Workflow

```c
int run_protected_update(uint16_t target_oid)
{
    // Optional: override target OID
    if (target_oid != 0) {
        tesaiot_pu_set_default_target_oid(target_oid);
    }

    // Run workflow (connects to MQTT, waits for manifest)
    return tesaiot_run_protected_update_workflow();
}
```

---

## OID Reference

### OPTIGA Trust M OID Layout

OPTIGA Trust M มี OID สำหรับเก็บ Certificate หลายตำแหน่ง:

| OID | Name | Access | Description |
|-----|------|--------|-------------|
| **0xE0C2** | Trust M UID | Read-only | Factory unique identifier (27 bytes) |
| **0xE0E0** | Factory Certificate | Read-only | Infineon pre-provisioned certificate |
| **0xE0E1** | Device Certificate 1 | Read/Write | Device certificate slot 1 |
| **0xE0E2** | Device Certificate 2 | Read/Write | Device certificate slot 2 (**Recommended**) |
| **0xE0E3** | Device Certificate 3 | Read/Write | Device certificate slot 3 |
| **0xE0E8** | Trust Anchor 1 | **Locked** (LcsO=0x07) | PU verification for 0xE0E2 |
| **0xE0E9** | **Trust Anchor 2** | Read/Write | **Current TA** for 0xE0E3 PU + CSR CA Chain |
| **0xE0E9** | CA Chain | Read/Write | CSR CA certificate chain |
| **0xE0F0** | Factory Key | Key-only | Factory private key (Read-Never) |
| **0xE0F1** | CSR Key | Key-only | CSR-generated private key |
| **0xE0F2** | PU Key Slot 1 | Key-only | Protected Update key slot |
| **0xE0F3** | PU Key Slot 2 | Key-only | Protected Update key slot |
| **0xF1D4** | Secret | Read-Never | AES-128-CCM for encrypted PU |

### Device Certificate OID Selection

Infineon กำหนด OID สำหรับ Device Certificate ไว้ 3 slots:

| OID | Recommendation | Use Case |
|-----|----------------|----------|
| **0xE0E1** | Available | Primary device certificate slot |
| **0xE0E2** | **Recommended (Default)** | TESAIoT default target - safe choice for most devices |
| **0xE0E3** | Available | Alternative/backup certificate slot |

**คำแนะนำ**:
- ใช้ `0xE0E2` เป็น default เพราะเป็น slot ที่ปลอดภัยและ Infineon แนะนำ
- สามารถใช้ `--target-oid` เพื่อเปลี่ยนไปใช้ OID อื่นได้ตามต้องการ
- ตรวจสอบ metadata ของ OID ก่อนใช้ด้วย `trustm_metadata -r 0xE0Ex`

### OID Usage by Workflow

| Workflow | Private Key | Certificate | Trust Anchor |
|----------|-------------|-------------|--------------|
| CSR | 0xE0F1 | 0xE0E1/E2/E3 | 0xE0E9 |
| Protected Update | 0xE0F2/F3 | 0xE0E1/E2/E3 | 0xE0E8 |

**Note**: ทั้งสอง workflow สามารถ write ไปยัง OID 0xE0E1, 0xE0E2, หรือ 0xE0E3 ได้
ใช้ `--target-oid 0xE0Ex` เพื่อระบุ OID ที่ต้องการ

---

## Error Codes

### Return Code Convention

```c
// Success
#define TESAIOT_SUCCESS 0

// Error codes follow Linux errno convention (negative values)
// -EINVAL (-22): Invalid argument
// -EACCES (-13): Permission denied / License invalid
// -EIO    (-5):  I/O error (Trust M communication)
// -ENOMEM (-12): Out of memory
// -ETIMEDOUT (-110): Operation timeout
```

### CSR Workflow Errors

| Error | Description |
|-------|-------------|
| -1 | Invalid configuration |
| -2 | Keypair generation failed |
| -3 | CSR generation failed |
| -4 | MQTT connection failed |
| -5 | CSR publish failed |
| -6 | Certificate write failed |
| -7 | Certificate wait timeout |

### Protected Update Errors

| Error | Description |
|-------|-------------|
| `TESAIOT_PU_ERROR_INVALID_PARAM` (-1) | Invalid parameter |
| `TESAIOT_PU_ERROR_NOT_INIT` (-2) | PU not initialized |
| `TESAIOT_PU_ERROR_INVALID_MANIFEST` (-3) | Invalid CBOR manifest |
| `TESAIOT_PU_ERROR_SIGNATURE_VERIFY` (-4) | Signature verification failed |
| `TESAIOT_PU_ERROR_INVALID_FRAGMENT` (-5) | Invalid fragment |
| `TESAIOT_PU_ERROR_VERSION_DOWNGRADE` (-6) | Version downgrade detected |
| `TESAIOT_PU_ERROR_TARGET_OID_WRITE` (-7) | Target OID write failed |
| `TESAIOT_PU_ERROR_TRUST_ANCHOR_READ` (-8) | Trust Anchor read failed |
| `TESAIOT_PU_ERROR_OPTIGA_OP` (-9) | OPTIGA operation failed |

### OPTIGA Error Codes

| Code | Description |
|------|-------------|
| 0x0305 | Trust Anchor not provisioned |
| 0x8029 | Invalid certificate format |
| 0x8007 | Access denied (OID locked) |

---

## Troubleshooting

### Issue: "OPTIGA initialization failed"

```bash
# Check I2C device
ls -la /dev/i2c-1

# Check permissions
sudo chmod 666 /dev/i2c-1

# Verify Trust M is detected
i2cdetect -y 1  # Should show 0x30
```

### Issue: "License verification failed"

1. ตรวจสอบ `TESAIOT_DEVICE_UID` ใน `tesaiot_config.h` ตรงกับ Trust M UID จริง
2. ตรวจสอบ `TESAIOT_LICENSE_KEY` ถูกต้อง

```bash
# Read actual UID from Trust M
./bin/trustm/trustm_data -r 0xE0C2 -X
```

### Issue: "MQTT connection failed"

```bash
# Test TLS connection
openssl s_client -connect mqtt.tesaiot.com:8883

# Check CA certificate
openssl verify -CAfile /tmp/tesaiot_ca.pem /tmp/tesaiot_ca.pem
```

### Issue: "Error 0x0305 - Trust Anchor not provisioned"

Protected Update ต้องการ Trust Anchor ที่ OID 0xE0E8:

```bash
# Check if Trust Anchor exists
./bin/trustm/trustm_data -r 0xE0E8 -X

# Trust Anchor จะถูก provision อัตโนมัติจาก signing_certificate ใน manifest JSON
```

### Issue: "Error 0x8029 - Invalid certificate format"

Trust Anchor ต้องเป็น X.509 Certificate (~580 bytes) ไม่ใช่ raw public key:

```bash
# Verify format (should start with 0x30 = ASN.1 SEQUENCE)
xxd /tmp/tesaiot_ta_cert.der | head -1
```

### Issue: TLS Hash Algorithm Warning

```
trustm_provider: Unsupported input padding algorithm. Only SHA256 is supported
```

นี่เป็น benign warning จาก trustm_provider - TLS จะ fallback ไปใช้ SHA256 และทำงานปกติ

---

## Security Considerations

### Private Key Protection

- Private keys (0xE0F0, 0xE0F1, 0xE0F2, 0xE0F3) ถูกสร้างและเก็บใน Trust M
- Private keys **ไม่มีทาง** export ออกมาได้
- Signing operations ทำภายใน Trust M เท่านั้น

### License Verification

- License key ถูก sign ด้วย TESAIoT Platform private key
- Verification ใช้ constant-time comparison เพื่อป้องกัน timing attacks
- UID comparison ทำภายใน library เพื่อป้องกัน spoofing

### Protected Update Security

- Manifest ถูก sign ด้วย ECDSA (P-256/P-384)
- Trust Anchor verification ทำภายใน Trust M
- Version downgrade protection (optional)

### MQTT TLS

- TLS 1.2/1.3 with mutual authentication (mTLS)
- Factory certificate สำหรับ bootstrap (CSR workflow)
- Device certificate สำหรับ operations (PU workflow)

---

## Additional Resources

### Documentation

- [OPTIGA Trust M Datasheet](https://www.infineon.com/optiga-trust-m)
- [TESAIoT Platform API](https://docs.tesaiot.com)

### Support

- GitHub Issues: https://github.com/wiroon/rpi-linux-optiga-trust-m/issues
- TESAIoT Support: support@tesaiot.com

---

**Copyright (c) 2026 Thai Embedded Systems Association (TESA). All rights reserved.**
