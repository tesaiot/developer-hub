# PSoC Edge E84 + OPTIGA Trust M: TESAIoT MQTT Client

**Firmware Version:** v3.0.0 Production
**TESAIoT Library:** v3.0.0
**Date:** 2026-02-08

This project demonstrates a **secure IoT device provisioning workflow** using:

- **PSoC Edge E84** (Cortex-M33 + M55)
- **OPTIGA Trust M** secure element for hardware-protected private keys
- **TESAIoT Platform** for Protected Update certificate renewal
- **MQTT over TLS** with device certificates stored in OPTIGA

---

## Key Features

| Feature | Description |
|---------|-------------|
| **Hardware Root of Trust** | Private keys generated and stored in OPTIGA Trust M (never leave the chip) |
| **Two-Certificate PKI** | Factory/Bootstrap certificate + Operational/Device certificate |
| **SAFE MODE** | Automatic fallback to Factory Certificate for recovery |
| **Secure MQTT** | TLS 1.2/1.3 with hardware-bound keys |
| **IoT Cybersecurity Compliant** | Follows NIST IR 8259A, IEC 62443, IEEE 802.1AR |
| **Developer Crypto Utilities** | 14 hardware-accelerated functions (TRNG, AES, HMAC, ECDH, HKDF, ECDSA, counters) |

---

## Main Menu Options

```sh
================ PSoC Edge-to-TESAIoT Platform Menu ================
[VERSION] PSE84 Trust M + TESAIoT Firmware v3.0.0 Production
[STATUS] Wi-Fi: connected | MQTT: idle
[LICENSE] Valid - UID: CD16339301001C000500000A01BB820003004000AE801010712440
[TIME] 2026-02-01 17:52:29 UTC+7 (NTP synced)
1) Print factory UID and factory certificate
2) Test MQTT connection with current certificate
3) Full Protected Update workflow with TESAIoT
4) Test OPTIGA Trust M metadata operations (diagnostics)
5) Test OPTIGA Trust M metadata operations (diagnostics)
Select option (1-4) then press Enter:
```

### Menu Descriptions

| Menu | Function | Description |
|------|----------|-------------|
| **1** | Print Factory Info | Displays OPTIGA Trust M UID and reads Factory Certificate from OID 0xE0E0 |
| **2** | Test MQTT | Tests TLS connection to broker using current certificate (Factory or Device) |
| **3** | **Protected Update** | Full Protected Update workflow: Receive signed certificate via manifest verification |
| **4** | **Diagnostics** | Test OPTIGA Trust M metadata operations (read/write access conditions) |
| **5** | Metadata Diagnostics | Read OPTIGA metadata, display installed certificates, lifecycle state |

---

## OPTIGA Trust M OID Reference

### Certificate and Key Slots

| OID | Name | Type | Purpose | Lifecycle |
|-----|------|------|---------|-----------|
| __0xE0C2__ | Factory UID | Read-Only | Hardware identity (27 bytes) | Permanent |
| __0xE0E0__ | Factory Certificate | Certificate | Pre-provisioned TLS certificate | Read-only after provisioning |
| __0xE0E1__ | Device Certificate | Certificate | Operational certificate from TESAIoT Platform (factory pre-provisioned) | Writable (up to 1728 bytes) |
| __0xE0E3__ | Trust Anchor (ROOT_CA) | Public Key | Protected Update signature verification | Provisioned |
| __0xE0F0__ | Factory Private Key | ECC P-256 | Paired with Factory Cert (0xE0E0) | Read-only |
| __0xE0F1__ | Device Private Key | ECC P-256 | Paired with Device Cert (0xE0E1) | Pre-provisioned in secure factory (Infineon CC EAL6+) |
| __0xF1D0-0xF1DF__ | User Data Objects | Data | Application-specific storage | Read/Write |

### OID Access Conditions

```sql
LcsO (Lifecycle State Object) = 0x07 (Operational)
+------------------------------------------------------------------+
|  OID   |  Read Access  |  Change Access  |  Execute Access       |
+--------+---------------+-----------------+-----------------------+
| 0xE0E0 |    Always     |      Never      |    N/A (Certificate)  |
| 0xE0E1 |    Always     |     Always*     |    N/A (Certificate)  |
| 0xE0F0 |    Never      |      Never      |    Always (TLS Sign)  |
| 0xE0F1 |    Never      |     Always*     |    Always (TLS Sign)  |
+------------------------------------------------------------------+
* Write operations allowed, metadata change blocked (LcsO >= 0x07)
```

---

## Two-Certificate PKI Model

```yaml
+========================================================================================+
|                         TESAIoT Two-Certificate PKI Architecture                       |
+========================================================================================+

    FACTORY CERTIFICATE (Bootstrap)                        DEVICE CERTIFICATE (Operational)
    +------------------------------+                       +------------------------------+
    | OID: 0xE0E0                  |                       | OID: 0xE0E1                  |
    | Key: 0xE0F0                  |                       | Key: 0xE0F1                  |
    | Type: Pre-provisioned        |  ------------------>  | Type: Factory pre-provisioned|
    | Purpose: Initial TLS auth    |  Factory Provisioning | Purpose: Production MQTT     |
    | Status: Read-only            |                       | Status: Renewable            |
    +------------------------------+                       +------------------------------+
              |                                                           |
              v                                                           v
    +------------------------------+                       +------------------------------+
    | When Used:                   |                       | When Used:                   |
    | - First boot                 |                       | - After factory provisioning |
    | - After board reset          |                       | - Normal production mode     |
    | - Recovery mode              |                       | - Until cert expires/reset   |
    | - factory provisioning       |                       |                              |
    +------------------------------+                       +------------------------------+
```

### Certificate Selection Logic (SAFE MODE)

After device reset, the firmware uses **SAFE MODE** to ensure reliable operation:

```c
/*
 * Certificate Selection Priority (after reset):
 * 1. Force Factory Certificate (SAFE MODE) - Prevents key/cert mismatch
 * 2. After factory provisioning - Use Device Certificate
 */
```

**Why SAFE MODE?**

- Device Certificate (0xE0E1) may not match Device Key (0xE0F1) after reset
- Pre-provisioned factory key is lost on power cycle (RAM only until cert written)
- Factory Certificate + Factory Key are always paired correctly
- Guarantees MQTT connection works for recovery

---

## Certificate Renewal Use Cases

### When Does Certificate Renewal Happen?

| Scenario | Trigger | Action Required |
|----------|---------|-----------------|
| **First Boot** | No Device Cert exists | Use Protected Update for certificate renewal |
| **After Board Reset** | Key/Cert mismatch possible | Factory Cert used (SAFE MODE), then Protected Update |
| **Certificate Expired** | TLS handshake fails | Use Protected Update for certificate renewal to get new certificate |
| **Before Expiry** | Proactive renewal | Use Protected Update for certificate renewal (manual trigger) |
| **Key Compromise** | Security incident | Use Protected Update for certificate renewal to generate new keypair |
| **Factory Reset** | User-initiated | Factory Cert used, then Protected Update |

### Certificate Renewal Flow Diagram

```md
+==============================================================================+
|                      Certificate Lifecycle Management                         |
+==============================================================================+

[Device Power On / Reset]
         |
         v
+------------------+     NO      +------------------+
| g_force_factory_ |------------>| Check Device Cert|
| cert = true?     |             | (0xE0E1) valid?  |
+--------+---------+             +--------+---------+
         | YES                            |
         v                                | NO (or mismatch)
+------------------+                      v
| SAFE MODE Active |             +------------------+
| Use Factory Cert |<------------| Fallback to      |
| (0xE0E0 + 0xE0F0)|             | Factory Cert     |
+--------+---------+             +------------------+
         |
         | MQTT Connect Successful
         v
+------------------+     Manual     +------------------+
|User initiates    |--------------->| Protected Update |
| renewal Workflow |                | Runs completely  |
+------------------+                +--------+---------+
                                             |
                                             | New cert in 0xE0E1
                                             | New key in 0xE0F1
                                             v
                                    +------------------+
                                    | g_force_factory_ |
                                    | cert = false     |
                                    +--------+---------+
                                             |
                                             v
                                    +------------------+
                                    | Device Cert Mode |
                                    | Use 0xE0E1+0xE0F1|
                                    +------------------+
                                             |
                                             | (Until next reset)
                                             v
                                    +------------------+
                                    | Production MQTT  |
                                    | Operational mode |
                                    +------------------+
```

### Certificate Expiry Scenarios

```md
Timeline: Certificate Lifecycle
============================================================================

   Factory Provisioning          Protected Update           Certificate Expires
          |                           |                        |
          v                           v                        v
   [FACTORY CERT]              [DEVICE CERT]             [RENEWAL NEEDED]
   Valid: 10 years             Valid: 1 year             Must run Protected Update
          |                           |                        |
          +------ Bootstrap ----------+------ Production ------+

   Renewal Trigger Points:
   1. Before Expiry (Recommended):
      - Use Protected Update for certificate renewal anytime to get fresh certificate
      - Platform issues new cert with extended validity

   2. At Expiry:
      - TLS handshake fails with Device Cert
      - System falls back to Factory Cert (SAFE MODE)
      - User runs Protected Update to renew

   3. After Expiry (Recovery):
      - Factory Cert still valid (longer validity period)
      - Connect to platform, run Protected Update
      - Get new Device Certificate
```

---

## Requirements

### Hardware

- PSoC Edge E84 Evaluation Kit (KIT_PSE84_EVAL_EPC2 or EPC4)
- PSoC Edge E84 AI Kit (KIT_PSE84_AI)
- OPTIGA Trust M shield (SLS32AIA010MS)

### Software

- ModusToolbox 3.6+
- GNU Arm Embedded Compiler v14.2.1
- Terminal emulator (115200 baud, 8N1)

### Network

- Wi-Fi access point (2.4GHz)
- TESAIoT Platform account (for Protected Update)

---

## Quick Start

### 1. Clone and Setup

```bash
# Clone the project
git clone <repository-url>
cd pse84_trustm_tesaiot_mqtt_mTLS

# Download libraries
make getlibs
```

### 2. Apply Patches (CRITICAL)

After `make getlibs`, you **MUST** apply patches for OPTIGA Trust M integration:

```bash
./apply_patches.sh
```

This patches:

1. __ifx-mbedtls__ - PSA headers + `mbedtls_ms_time()` for FreeRTOS
2. __secure-sockets__ - OPTIGA Trust M TLS key binding (`cy_tls_optiga_key.c`)
3. __lwipopts.h__ - Fix ERR_WOULDBLOCK (-7) on MQTT publish

### 3. Configure Wi-Fi

Edit `proj_cm33_ns/wifi_config.h`:

```c
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define WIFI_SECURITY   CY_WCM_SECURITY_WPA2_AES_PSK
```

### 4. Configure MQTT (TESAIoT)

Edit `proj_cm33_ns/mqtt_client_config.h`:

```c
#define MQTT_BROKER_ADDRESS     "mqtt.tesaiot.com"
#define MQTT_PORT               8883
#define MQTT_SECURE_CONNECTION  1
```

### 5. Build and Flash

```bash
make clean && make build -j8
make program
```

---

## Typical Usage Workflow

### First Time Setup

```csv
1. Power on device
2. Connect serial terminal (115200 baud)
3. Wait for Wi-Fi connection and NTP sync
4. Observe SAFE MODE message (Factory Certificate)
5. Select Protected Update from menu
6. Wait for "Workflow completed successfully"
7. Select MQTT Connection Test to verify Device Certificate connection
```

### After Board Reset

```md
1. Power on device (or reset)
2. System automatically uses Factory Certificate (SAFE MODE)
3. MQTT Connection Test will work immediately (Factory Cert connection)
4. Use Protected Update for certificate renewal if you need Device Certificate
5. After Protected Update, system uses Device Certificate
```

### Certificate Renewal

```sh
1. Connect to device terminal
2. If certificate expired: System uses Factory Cert (SAFE MODE)
3. Use Protected Update for certificate renewal
4. New certificate will be issued by platform
5. Device automatically switches to new Device Certificate
```

---

## Patches Explained

The project requires patches to external libraries because:

| Patch | Why Needed |
|-------|------------|
| __ifx-mbedtls.patch__ | mbedTLS needs PSA error types and FreeRTOS-compatible `mbedtls_ms_time()` |
| __secure-sockets.patch__ | Enable TLS handshake using OPTIGA Trust M hardware keys instead of software keys |
| __lwipopts.h__ | Default `LWIP_TCPIP_CORE_LOCKING_INPUT=1` causes mutex deadlock during MQTT publish |

### Re-applying Patches

If you run `make getlibs` again, patches will be reset. Always re-run:

```bash
./apply_patches.sh
make clean && make build -j8
```

---

## Configuration Flags

| Flag | Location | Default | Purpose |
|------|----------|---------|---------|
| `g_force_factory_cert` | tesaiot_optiga_trust_m.c | `true` | Force Factory Cert on boot (SAFE MODE) |
| `TESAIOT_DEBUG_LEVEL` | tesaiot_debug_config.h | `2` (WARNING) | Debug output verbosity |

---

## IoT Cybersecurity Compliance

This implementation follows industry standards:

| Standard | Compliance |
|----------|------------|
| **NIST IR 8259A** | Device identity, secure boot, secure communication |
| **IEC 62443** | Industrial automation security |
| **IEEE 802.1AR** | Device identity with hardware root of trust |

### Security Features

- Private keys never leave OPTIGA Trust M hardware
- Certificate-based authentication (no passwords)
- Automatic certificate rotation via Protected Update
- Factory certificate cannot be modified (read-only after provisioning)
- SAFE MODE ensures recovery is always possible

---

## Library Versions

### Key Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| optiga-trust-m | release-v5.3.0 | OPTIGA Trust M driver |
| ifx-mbedtls | release-v3.6.400 | TLS/crypto (patched) |
| secure-sockets | release-v3.12.1 | TLS sockets (patched) |
| mqtt | release-v4.7.0 | MQTT client |
| freertos | release-v10.6.202 | RTOS |
| lwip | STABLE-2_1_2_RELEASE | TCP/IP stack |

---

## Project Structure

```ini
pse84_trustm_tesaiot_mqtt_mTLS/
├── proj_cm33_ns/                    # Main application (Non-Secure CM33)
│   ├── main.c                       # Menu system and task orchestration
│   ├── tesaiot_optiga_trust_m.c     # OPTIGA operations + cert selection
│   ├── optiga_trust_helpers.c       # OPTIGA helpers + DER parser
│   ├── mqtt_task.c                  # MQTT connection management
│   ├── subscriber_task.c            # MQTT subscription handler
│   ├── wifi_config.h                # Wi-Fi credentials
│   └── mqtt_client_config.h         # MQTT/TLS configuration
├── proj_cm33_s/                     # Secure CM33 (protection settings)
├── proj_cm55/                       # CM55 coprocessor
├── tesaiot/                         # TESAIoT library source
├── apply_patches.sh                 # Patch script (run after make getlibs)
├── ifx-mbedtls.patch               # mbedTLS patches
└── secure-sockets.patch            # secure-sockets patches
```

---

## Related Resources

| Resource | Link |
|----------|------|
| PSoC Edge E84 Datasheet | [Infineon](https://www.infineon.com/psoc-edge) |
| OPTIGA Trust M Datasheet | [Infineon](https://www.infineon.com/optiga-trust-m) |
| ModusToolbox | [Download](https://www.infineon.com/modustoolbox) |
| TESAIoT Platform | [tesaiot.com](https://tesaiot.com) |

---

## Developer Crypto Utilities (v3.0.0)

14 hardware-accelerated crypto functions wrapping OPTIGA Trust M:

| Function | Category | Description |
|----------|----------|-------------|
| `tesaiot_random_generate()` | TRNG | Hardware random bytes (CC EAL6+) |
| `tesaiot_secure_store_write()` | Storage | Write to OPTIGA data object (14 slots) |
| `tesaiot_secure_store_read()` | Storage | Read from OPTIGA data object |
| `tesaiot_aes_generate_key()` | AES | Generate AES key in hardware (128/192/256) |
| `tesaiot_aes_encrypt()` | AES | AES-CBC encrypt (key never leaves OPTIGA) |
| `tesaiot_aes_decrypt()` | AES | AES-CBC decrypt |
| `tesaiot_hmac_sha256()` | HMAC | HMAC-SHA256 with hardware key |
| `tesaiot_ecdh_shared_secret()` | ECDH | P-256 shared secret derivation |
| `tesaiot_hkdf_derive()` | KDF | HKDF-SHA256 key derivation (RFC 5869) |
| `tesaiot_optiga_hash()` | Hash | SHA-256 hardware hash |
| `tesaiot_sign_data()` | Sign | SHA-256 + ECDSA composite sign |
| `tesaiot_counter_read()` | Counter | Read monotonic counter (anti-replay) |
| `tesaiot_counter_increment()` | Counter | Increment monotonic counter |
| `tesaiot_health_check()` | Diag | Comprehensive device health check |

### Example Codes

10 example files in `proj_cm33_ns/examples/` organized by OID use case:

| Category | Examples | Description |
|----------|----------|-------------|
| A: Secure Storage | A1, A2 | Credential store, data encryption at rest |
| B: Secure Communication | B1, B2, B3 | HMAC MQTT, E2E encryption, D2D channel |
| C: Identification | C1, C2 | Signed telemetry, challenge-response auth |
| D: Security Operations | D1, D2 | Anti-replay counter, health dashboard |
| E: Application Patterns | E1 | Complete smart sensor workflow |

---

## TESAIoT Library Architecture (v3.0.0)

### Header Files

The project uses 10 consolidated TESAIoT headers (v3.0.0):

```ini
tesaiot/include/
├── tesaiot.h                  # Main umbrella (includes all)
├── tesaiot_config.h           # Configuration
├── tesaiot_crypto.h           # Developer Crypto Utilities API (NEW in v3.0.0)
├── tesaiot_license_config.h   # Customer editable (UID + License Key)
├── tesaiot_license.h          # License API
├── tesaiot_optiga.h           # OPTIGA integration
├── tesaiot_optiga_core.h      # OPTIGA manager
├── tesaiot_platform.h         # MQTT + SNTP
└── tesaiot_protected_update.h # Protected Update
```

### 3-Layer License Architecture (NEW in v3.0.0)

```ini
Layer 1: Configuration (Customer Edits)
├── tesaiot/include/tesaiot_license_config.h
│   └── #define TESAIOT_DEVICE_UID / TESAIOT_LICENSE_KEY

Layer 2: Data Binding (Customer Compiles)
├── tesaiot/src/tesaiot_license_data.c
│   └── extern variables (link-time binding)

Layer 3: Verification Logic (IP-Protected)
├── tesaiot/src/tesaiot_license.c
    └── Compiled into libtesaiot.a (embedded public key)
```

**Security Benefits:**

- Public key embedded in `.c` file (customer never sees it)
- Verification logic compiled into library (IP-protected)
- Customer provides UID + License Key via link-time binding

---

## Authors

**Assoc. Prof. Wiroon Sriborrirux (BDH)**

- Thai Embedded Systems Association (TESA)
- TESAIoT Platform Creator
- Email: sriborrirux@gmail.com / wiroon@tesa.or.th

**TESAIoT Platform Developer Team**

- In collaboration with Infineon Technologies AG

---

## License

OPTIGA Trust M integration and TESAIoT workflow by Assoc. Prof. Wiroon Sriborrirux, Thai Embedded Systems Association (TESA).

---

**Last Updated:** 2026-02-08
