# TESAIoT Portable Deployment Package

Self-contained deployment package for TESAIoT CSR and Protected Update workflows.

---

## Contents

```
rpi_tesaiot_client/
├── bin/
│   ├── tesaiot_csr_client      # CSR workflow binary
│   ├── tesaiot_pu_client       # Protected Update workflow binary
│   └── trustm/                 # Required OPTIGA Trust M tools
│       ├── trustm_data
│       ├── trustm_data.sig         # ECDSA signature (NEW)
│       ├── trustm_protected_update
│       ├── trustm_protected_update.sig
│       ├── trustm_metadata
│       ├── trustm_metadata.sig
│       ├── trustm_chipinfo
│       └── trustm_chipinfo.sig
├── credentials/                # Device credentials (REQUIRED)
│   ├── tesaiot-ca-chain.pem    # TESAIoT CA certificate chain
│   └── infineon-optiga-trust-m-ca.pem
├── lib/
│   ├── libtesaiot.a            # Static library for custom builds
│   ├── libpaho_shim.so         # LD_PRELOAD shim for MQTT TLS
│   ├── libtrustm.so            # Pre-built OPTIGA Trust M library (v2.x compatible)
│   └── trustm_provider.so      # Pre-built OpenSSL provider (v2.x compatible)
├── include/tesaiot/            # Header files
│   ├── tesaiot.h
│   ├── tesaiot_config.h        # Device config (DEVICE_ID, LICENSE_KEY)
│   ├── tesaiot_csr.h
│   ├── tesaiot_protected_update.h
│   └── tesaiot_tool_verify.h   # External tool verification (NEW)
├── scripts/
│   ├── fix_libgpiod_v2.sh      # Auto-patch for libgpiod v2.x
│   └── libgpiod_v2.patch       # Patch file for manual apply
├── config/
│   └── tesaiot_trustm_openssl.cnf
├── install.sh                  # System-wide installation
└── README.md                   # This file
```

**IMPORTANT**: Binaries are compiled with path `../credentials/` relative to `bin/`.
Must run from `bin/` directory or use the runner scripts.

## Device Configuration

Each device requires its own credentials in `include/tesaiot/tesaiot_config.h`:
1. Update `TESAIOT_DEVICE_ID`, `TESAIOT_DEVICE_UID`, `TESAIOT_LICENSE_KEY`
2. Rebuild binaries from source (see tesaiot_library/)

## Quick Start

### Step 1: Set Up Environment

```bash
# On target RPi
cd /home/pi/rpi_tesaiot_client

# Create .env from template and edit with your paths
cp .env_example .env
nano .env    # Set OPENSSL_MODULES to your trustm_provider.so directory
```

### Step 2: Run Workflows

```bash
# CSR workflow — enroll device certificate
./run_csr.sh run --target-oid 0xE0E1

# Protected Update — receive certificate via MQTT
./run_pu.sh run --target-oid 0xE0E1

# Diagnostics — show certificate info, OID metadata, health status
./run_csr.sh diag --target-oid 0xE0E1

# License check
./run_csr.sh license

# Device identity
./run_csr.sh identity
```

### Alternative: Run Directly from bin/

```bash
cd /home/pi/rpi_tesaiot_client/bin

# Manual shim setup (runner scripts do this automatically)
export LD_PRELOAD=../lib/libpaho_shim.so
export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH
./tesaiot_csr_client run --target-oid 0xE0E1
```

### Alternative: System-wide Installation

```bash
cd rpi_tesaiot_client
sudo ./install.sh

# Reload environment
source /etc/profile.d/tesaiot.sh
```

## Prerequisites

### Important: This package requires external dependencies

Simply copying rpi_tesaiot_client to another RPi is **NOT** enough. You must install the dependencies listed below.

### System Dependencies

These must be installed on the target system:

```bash
sudo apt-get update
sudo apt-get install -y \
    libssl3 \
    libpaho-mqtt3c1 \
    i2c-tools \
    libgpiod2
```

### OPTIGA Trust M Library

This package includes **pre-built libraries** that work with libgpiod v2.x (Debian 12+, Ubuntu 24.04+).

#### Option A: Use Bundled Libraries (Recommended - Easiest)

The runner scripts automatically detect and use bundled libraries in `lib/`:
- `libtrustm.so` - OPTIGA Trust M library (patched for libgpiod v2.x)
- `trustm_provider.so` - OpenSSL provider

**No additional installation needed!** Just run:
```bash
./run_csr.sh run --target-oid 0xE0E1
```

#### Option B: Use Automatic Patch Script

If you want to build from source with the latest Infineon code:
```bash
# Clone upstream repository
git clone https://github.com/Infineon/linux-optiga-trust-m.git
cd linux-optiga-trust-m

# Run the automatic patch script
/path/to/rpi_tesaiot_client/scripts/fix_libgpiod_v2.sh

# Build and install
./build.sh
sudo make install
sudo ldconfig
```

#### Option C: Apply Patch Manually

```bash
git clone https://github.com/Infineon/linux-optiga-trust-m.git
cd linux-optiga-trust-m

# Apply patch
cd trustm_lib
patch -p1 < /path/to/rpi_tesaiot_client/scripts/libgpiod_v2.patch
cd ..

# Build and install
./build.sh
sudo make install
sudo ldconfig
```

#### Option D: Systems with libgpiod v1.x (Debian 11, Ubuntu 22.04)

If you're using an older OS with libgpiod v1.x, no patch is needed:
```bash
git clone https://github.com/Infineon/linux-optiga-trust-m.git
cd linux-optiga-trust-m
./build.sh
sudo make install
sudo ldconfig
```

**Quick Check** - Which libgpiod version do you have?
```bash
apt-cache policy libgpiod-dev | grep Installed
# v1.x (1.6.x) = No patch needed
# v2.x (2.0+)  = Use Option A, B, or C above
```

#### Verify Installation (if using Options B, C, or D)
```bash
# Check libtrustm
ldconfig -p | grep libtrustm
# Expected: libtrustm.so -> /lib/aarch64-linux-gnu/libtrustm.so

# Check trustm_provider.so
find /usr/lib -name "trustm_provider.so"
# Expected locations:
# - /usr/lib/aarch64-linux-gnu/ossl-modules/trustm_provider.so (64-bit)
# - /usr/lib/arm-linux-gnueabihf/ossl-modules/trustm_provider.so (32-bit)
```

### Hardware Setup

- OPTIGA Trust M Shield connected via I2C (address 0x30)
- GPIO 17 connected to OPTIGA reset pin
- I2C and GPIO enabled in Raspberry Pi config

```bash
# Enable I2C and SPI (if not already done)
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0

# Verify I2C connection
i2cdetect -y 1
# Should show 30 at address 0x30
```

### Quick Dependency Install

Use the included install.sh script which handles dependency installation:

```bash
cd rpi_tesaiot_client
sudo ./install.sh
```

This will:
1. Install system dependencies (libssl3, libpaho-mqtt3c1, libgpiod2, i2c-tools)
2. Check for OPTIGA Trust M library (warns if not found)
3. Install TESAIoT to /opt/tesaiot

## Environment Configuration

### `.env` File

Copy `.env_example` to `.env` and configure:

```bash
cp .env_example .env
nano .env
```

**Required setting:**
- `OPENSSL_MODULES` — Path to directory containing `trustm_provider.so`

**Optional settings:**
- `TRUSTM_DATA_PATH` — Path to `trustm_data` binary (auto-detected from binary location)
- `TESAIOT_LOG_LEVEL` — Logging level: ERROR, WARN, INFO, DEBUG

### Environment Variables

Runner scripts (`run_csr.sh`, `run_pu.sh`) automatically set these:

| Variable | Description | Default |
|----------|-------------|---------|
| `LD_PRELOAD` | libpaho_shim.so for OPTIGA mTLS | Set by runner script |
| `LD_LIBRARY_PATH` | Bundled libraries in `lib/` | Set by runner script |
| `TRUSTM_LIB_PATH` | Path to trustm tools directory | `$SCRIPT_DIR/bin/trustm` |
| `TRUSTM_DATA_PATH` | Path to trustm_data binary | `$SCRIPT_DIR/bin/trustm/trustm_data` |
| `OPENSSL_MODULES` | trustm_provider.so directory | From `.env` file |

### Tool Path Auto-Detection

External tools (`trustm_data`, `trustm_protected_update`) are found automatically:

1. **`TRUSTM_DATA_PATH`** environment variable (set by runner scripts)
2. **`/proc/self/exe`** auto-detection: `<binary_dir>/trustm/<tool_name>`

This means tools work whether run via `run_csr.sh`/`run_pu.sh` or directly from `bin/`.

### Tool Signature Verification

All external tools are **ECDSA P-256 verified** before execution (NIST SP 800-193):

```
bin/trustm/
├── trustm_data                  # Binary
├── trustm_data.sig              # ECDSA signature
├── trustm_protected_update      # Binary
├── trustm_protected_update.sig  # ECDSA signature
└── ...
```

If a tool binary is tampered with, verification fails and execution is **blocked**.

## Building Custom Applications

Use the static library and headers:

```c
#include <tesaiot/tesaiot.h>
#include <tesaiot/tesaiot_csr.h>
#include <tesaiot/tesaiot_protected_update.h>
```

```bash
gcc -o my_app my_app.c \
    -I/opt/tesaiot/include \
    -L/opt/tesaiot/lib \
    -ltesaiot -lssl -lcrypto -lpaho-mqtt3c -lpthread
```

## Troubleshooting

### OPTIGA Timeout

If you see timeout errors after TLS operations:

```bash
# Reset OPTIGA via GPIO
sudo gpioset gpiochip0 17=0
sleep 0.1
sudo gpioset gpiochip0 17=1
```

### Library Not Found

```bash
# Add library path
export LD_LIBRARY_PATH=/opt/tesaiot/lib:$LD_LIBRARY_PATH
```

### Permission Denied

```bash
# Runner scripts handle environment setup automatically
sudo ./run_pu.sh run --target-oid 0xE0E1
```

---

## Security Architecture

### Why Hardware Security Module (HSM)?

TESAIoT uses **OPTIGA Trust M** hardware security module for critical security operations:

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Traditional Software Security                    │
├─────────────────────────────────────────────────────────────────────┤
│  Private Key stored in:                                             │
│  - File system (easily extracted)                                   │
│  - Environment variables (memory dump vulnerable)                   │
│  - Application memory (side-channel attacks)                        │
│                                                                     │
│  Vulnerabilities:                                                   │
│  ✗ Key extraction via physical access                               │
│  ✗ Memory dump attacks                                              │
│  ✗ Firmware reverse engineering                                     │
│  ✗ No hardware attestation                                          │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│               TESAIoT Hardware Security (OPTIGA Trust M)            │
├─────────────────────────────────────────────────────────────────────┤
│  Private Key stored in:                                             │
│  - Secure Element (tamper-resistant silicon)                        │
│  - Never leaves the chip                                            │
│  - Protected by hardware countermeasures                            │
│                                                                     │
│  Security Features:                                                 │
│  ✓ Keys generated inside secure element                             │
│  ✓ Private keys NEVER exportable                                    │
│  ✓ Hardware-based cryptographic operations                          │
│  ✓ Tamper detection and response                                    │
│  ✓ Side-channel attack protection                                   │
│  ✓ Secure boot chain attestation                                    │
│  ✓ Protected Update with integrity verification                     │
└─────────────────────────────────────────────────────────────────────┘
```

### OID (Object Identifier) Separation

Critical design principle: **Complete service separation** between CSR and Protected Update workflows.

| OID | Name | CSR Workflow | PU Workflow | Purpose |
|-----|------|:------------:|:-----------:|---------|
| 0xE0C2 | Trust M UID | R | R | Unique 27-byte device identifier |
| 0xE0E0 | Factory Certificate | R | - | Infineon-signed certificate (bootstrap TLS) |
| **0xE0E1** | **Device Certificate 1** | W | W | **Default target** for device certificate |
| 0xE0E2 | Device Certificate 2 | W | W | Alternative certificate slot |
| 0xE0E3 | Device Certificate 3 | W | W | Alternative certificate slot |
| 0xE0E8 | Trust Anchor 1 | - | R | PU manifest signature verification |
| 0xE0E9 | Trust Anchor 2 / CA | R | R | PU verification + CSR CA Chain |
| 0xE0F0 | Factory Key | S | - | Factory private key |
| 0xE0F1 | CSR Key | G/S | - | ECC P-256 keypair for CSR signing |
| 0xE0F2 | PU Key Slot | - | U | Protected Update operations |
| 0xF1D4 | Secret | - | D | AES-128-CCM key for encrypted updates |

**Legend**: R=Read, W=Write, G=Generate, S=Sign, U=Update, D=Decrypt

**Note**: Use `--target-oid` to select which certificate slot to write to (default: 0xE0E1).

---

## Workflow Diagrams

### CSR (Certificate Signing Request) Workflow

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         CSR Workflow Sequence                            │
└──────────────────────────────────────────────────────────────────────────┘

    Device (RPi + OPTIGA)                    TESAIoT Platform
    ─────────────────────                    ─────────────────
            │                                        │
            │  1. Generate ECC P-256 Keypair         │
            │     (Private key at OID 0xE0F1)        │
            │     ┌─────────────────────────┐        │
            │     │   OPTIGA Trust M        │        │
            │     │   optiga_crypt_ecc_     │        │
            │     │   generate_keypair()    │        │
            │     └─────────────────────────┘        │
            │                                        │
            │  2. Generate CSR (PEM format)          │
            │     Subject: CN={Device UID}           │
            │     Public Key: From step 1            │
            │     Signature: OPTIGA signs with 0xE0F1│
            │                                        │
            │  3. Connect MQTT (TLS 1.2)             │
            │     Client Cert: Factory (0xE0E0)      │
            │     CA Chain: 0xE0E9                   │
            │                                        │
            │              CSR (PEM)                 │
            │ ─────────────────────────────────────> │
            │    Topic: device/{id}/commands/csr     │
            │                                        │
            │                                        │  4. Validate CSR
            │                                        │     - Verify signature
            │                                        │     - Check device ID
            │                                        │     - Policy evaluation
            │                                        │
            │                                        │  5. Sign with Vault
            │                                        │     (Intermediate CA)
            │                                        │
            │          Certificate (RAW PEM)         │
            │ <───────────────────────────────────── │
            │ Topic: device/{id}/commands/certificate│
            │                                        │
            │  6. Write Certificate to Target OID    │
            │     ┌─────────────────────────┐        │
            │     │   OPTIGA Trust M        │        │
            │     │   optiga_util_write_    │        │
            │     │   data(target_oid, cert)│        │
            │     └─────────────────────────┘        │
            │                                        │
            │  7. Verify Certificate Chain           │
            │     Device Cert → Intermediate → Root  │
            │                                        │
           ✓ CSR Workflow Complete                   │
            │                                        │
```

### Protected Update Workflow

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    Protected Update Workflow Sequence                    │
└──────────────────────────────────────────────────────────────────────────┘

    Device (RPi + OPTIGA)                    TESAIoT Platform
    ─────────────────────                    ─────────────────
            │                                        │
            │                                        │  1. Create Update Payload
            │                                        │     - New certificate/config
            │                                        │     - Target OID (from manifest)
            │                                        │
            │                                        │  2. Generate CBOR Manifest
            │                                        │     (COSE_Sign1, RFC 8152)
            │                                        │     ┌──────────────────┐
            │                                        │     │ protected_header │
            │                                        │     │(algorithm: ES256)│
            │                                        │     ├──────────────────┤
            │                                        │     │ payload          │
            │                                        │     │ (target OID,     │
            │                                        │     │  version, hash)  │
            │                                        │     ├──────────────────┤
            │                                        │     │ signature        │
            │                                        │     │ (Platform key)   │
            │                                        │     └──────────────────┘
            │                                        │
            │     Manifest + Fragments (CBOR)        │
            │ <───────────────────────────────────── │
            │  Topic: device/{id}/commands/protected_update
            │                                        │
            │  3. Parse CBOR COSE_Sign1              │
            │     ┌─────────────────────────┐        │
            │     │   TinyCBOR Parser       │        │
            │     │   Extract: header,      │        │
            │     │   payload, signature    │        │
            │     └─────────────────────────┘        │
            │                                        │
            │  4. Verify Signature                   │
            │     ┌─────────────────────────┐        │
            │     │   OPTIGA Trust M        │        │
            │     │   Trust Anchor (0xE0E9) │        │
            │     │   optiga_crypt_ecdsa_   │        │
            │     │   verify()              │        │
            │     └─────────────────────────┘        │
            │                                        │
            │  5. Protected Update Sequence          │
            │     ┌─────────────────────────┐        │
            │     │ optiga_util_protected_  │        │
            │     │ update_start()          │        │
            │     ├─────────────────────────┤        │
            │     │ optiga_util_protected_  │        │
            │     │ update_continue() x N   │        │
            │     ├─────────────────────────┤        │
            │     │ optiga_util_protected_  │        │
            │     │ update_final()          │        │
            │     └─────────────────────────┘        │
            │                                        │
            │  6. Verify Integrity                   │
            │     - Hash verification                │
            │     - Version check (anti-rollback)    │
            │                                        │
           ✓ Protected Update Complete               │
            │                                        │
```

---

## Standards Compliance

TESAIoT architecture is designed to comply with international IoT security standards:

### ETSI EN 303 645 (Consumer IoT Security)

| Provision | Requirement | TESAIoT Implementation |
|-----------|-------------|------------------------|
| **5.1** | No universal default passwords | ✅ Unique device identity via OPTIGA Trust M UID |
| **5.2** | Implement vulnerability disclosure | ✅ CSR workflow enables certificate rotation |
| **5.3** | Keep software updated | ✅ Protected Update workflow with integrity verification |
| **5.4** | Securely store credentials | ✅ Private keys stored in hardware secure element |
| **5.5** | Communicate securely | ✅ TLS 1.2 with hardware-backed keys |
| **5.6** | Minimize attack surfaces | ✅ OID separation, minimal exposed interfaces |
| **5.7** | Ensure software integrity | ✅ COSE_Sign1 signed manifests, hash verification |
| **5.8** | Ensure personal data protection | ✅ Hardware encryption, access control via LcsO |
| **5.11** | Make it easy for users to delete personal data | ✅ Protected Update can reset device certificates |

### IEC 62443 (Industrial Automation Security)

| Security Level | Requirement | TESAIoT Implementation |
|----------------|-------------|------------------------|
| **SL 1** | Protection against casual violation | ✅ TLS encryption, authentication |
| **SL 2** | Protection against intentional violation | ✅ Hardware key storage, signed updates |
| **SL 3** | Protection using sophisticated means | ✅ OPTIGA tamper resistance, anti-rollback |
| **SL 4** | Protection with state-sponsored resources | ✅ Side-channel protection, secure boot chain |

### ISO/IEC 27001:2022 (Information Security)

| Control | Requirement | TESAIoT Implementation |
|---------|-------------|------------------------|
| **A.8.24** | Use of cryptography | ✅ ECC P-256, AES-128-CCM, SHA-256 |
| **A.8.25** | Secure development lifecycle | ✅ Hardware Root of Trust from manufacturing |
| **A.8.26** | Application security requirements | ✅ Certificate-based authentication |
| **A.8.27** | Secure system architecture | ✅ Separation of concerns (CSR vs PU OIDs) |
| **A.8.28** | Secure coding | ✅ Stack allocation, memory safety |

### NIST Cybersecurity Framework

| Function | Category | TESAIoT Implementation |
|----------|----------|------------------------|
| **Identify** | Asset Management | ✅ Unique device UID (0xE0C2) |
| **Protect** | Identity Management | ✅ X.509 certificates, hardware keys |
| **Protect** | Data Security | ✅ Hardware encryption, TLS 1.2 |
| **Detect** | Anomalies | ✅ Certificate validation, signature verification |
| **Respond** | Mitigation | ✅ Certificate revocation via CSR workflow |
| **Recover** | Recovery Planning | ✅ Protected Update for secure recovery |

### OWASP IoT Top 10 (2018)

| Vulnerability | OWASP Description | TESAIoT Mitigation |
|---------------|-------------------|-------------------|
| **I1** | Weak/Hardcoded Passwords | ✅ Certificate-based auth, no passwords |
| **I2** | Insecure Network Services | ✅ TLS 1.2 only, strong cipher suites |
| **I3** | Insecure Ecosystem Interfaces | ✅ MQTT with mTLS, signed manifests |
| **I4** | Lack of Secure Update | ✅ Protected Update with COSE_Sign1 |
| **I5** | Use of Insecure Components | ✅ Certified OPTIGA Trust M (CC EAL6+) |
| **I6** | Insufficient Privacy Protection | ✅ Hardware key isolation |
| **I7** | Insecure Data Transfer | ✅ TLS 1.2 with hardware keys |
| **I8** | Lack of Device Management | ✅ Certificate lifecycle management |
| **I9** | Insecure Default Settings | ✅ Secure-by-default configuration |
| **I10** | Lack of Physical Hardening | ✅ Tamper-resistant secure element |

### OPTIGA Trust M Certifications

The OPTIGA Trust M secure element provides:

- **Common Criteria EAL6+ (high)** - Highest security assurance level for smartcard ICs
- **AIS-31 Class PTG.2** - True random number generator certification
- **FIPS 140-2 Level 3 equivalent** - Cryptographic module security
- **EMVCo** - Payment card industry compliance ready

### External Tool Verification (NEW in v2.2)

TESAIoT Library implements ECDSA code signing for bundled external tools:

- **NIST SP 800-193 Compliant** - Verify integrity before execution
- **IEC 62443-4-2 SL3** - Protection against sophisticated attacks
- **TOCTOU Prevention** - Minimize time-of-check to time-of-use window

```
bin/trustm/
├── trustm_data                  # Binary
├── trustm_data.sig              # ECDSA P-256 signature
├── trustm_protected_update
├── trustm_protected_update.sig
└── ...
```

Library automatically verifies signatures before executing tools. If a tool is tampered, verification fails and execution is blocked.

---

## Known Issues

See [ISSUES.md](../TESAIoT_IMPROVEMENT/v3.0.0_Official_TESAIoT_Secure_Library/ISSUES.md) for details.

| Issue | Description | Workaround |
|-------|-------------|------------|
| ISSUE-001 | OPTIGA API hangs after MQTT (trustm_provider I2C lock) | External tools fallback |
| ISSUE-002 | `trustm_protected_update` fails with 0x8029 | Direct write via `trustm_data` |
| ISSUE-003 | Some devices have OID locked (Change:NEV) | Use `--target-oid` to select an available OID |
| ISSUE-004 | Root-owned temp files | `run_*.sh` auto-cleanup |

---

## Version

- Package Version: 3.0.0
- Build Date: 2026-02-08
- Platform: Raspberry Pi (aarch64/armhf)
- Changes (v3.0.0):
  - **NEW**: 16 cryptographic utility functions (`tesaiot_crypto.h`, `tesaiot_advanced.h`)
  - **NEW**: `diag` command — certificate info, OID metadata, health check
  - **NEW**: Top-level runner scripts (`run_csr.sh`, `run_pu.sh`)
  - **NEW**: `.env_example` template for environment configuration
  - **NEW**: Auto-detect tool paths via `/proc/self/exe`
  - **NEW**: Post-MQTT external tool fallback (30s timeout eliminated)
  - **NEW**: Monotonic counter support (anti-rollback)
  - Trust Anchor changed from 0xE0E8 (locked) to 0xE0E9
  - Library size: ~800 KB (was 363 KB)
- Changes (v2.2):
  - External tool ECDSA signature verification (NIST SP 800-193)
  - All bundled tools include `.sig` signature files
- Changes (v2.1):
  - Added `--target-oid` argument
  - Shim enabled by default
  - TESAIoT Platform Guideline v1.0.0 compliant
