# PSoC Edge E84 + OPTIGA Trust M: TESAIoT MQTT Client

**Firmware Version:** 2026.02 Public Beta
**TESAIoT Library:** v2.8.0
**Date:** 2026-02-01
**Status:** Public Beta

This project demonstrates a **secure IoT device provisioning workflow** using:

- **PSoC Edge E84** (Cortex-M33 + M55)
- **OPTIGA Trust M** secure element for hardware-protected private keys
- **TESAIoT Platform** for Certificate Signing Request (CSR) workflow
- **MQTT over TLS** with device certificates stored in OPTIGA

---

## Key Features

| Feature | Description |
|---------|-------------|
| **Hardware Root of Trust** | Private keys generated and stored in OPTIGA Trust M (never leave the chip) |
| **CSR Workflow** | Automated certificate enrollment via TESAIoT Platform |
| **Two-Certificate PKI** | Factory/Bootstrap certificate + Operational/Device certificate |
| **SAFE MODE** | Automatic fallback to Factory Certificate for recovery |
| **Secure MQTT** | TLS 1.2/1.3 with hardware-bound keys |
| **IoT Cybersecurity Compliant** | Follows NIST IR 8259A, IEC 62443, IEEE 802.1AR |

---

## Main Menu Options

```sh
================ PSoC Edge-to-TESAIoT Platform Menu ================
[VERSION] PSE84 Trust M + TESAIoT Firmware 2026.02 Public Beta
[STATUS] Wi-Fi: connected | MQTT: idle
[LICENSE] Valid - UID: CD16339301001C000500000A01BB820003004000AE801010712440
[TIME] 2026-02-01 10:00:00 UTC+7 (NTP synced)
1) Print factory UID and factory certificate
2) Test MQTT connection with current certificate
3) Full CSR workflow with TESAIoT
4) Full Protected Update workflow with TESAIoT
5) Test OPTIGA Trust M metadata operations (diagnostics)
Select option (1-5) then press Enter:
```

### Menu Descriptions

| Menu | Function | Description |
|------|----------|-------------|
| **1** | Print Factory Info | Displays OPTIGA Trust M UID and reads Factory Certificate from OID 0xE0E0 |
| **2** | Test MQTT | Tests TLS connection to broker using current certificate (Factory or Device) |
| **3** | **CSR Workflow** | Full automated workflow: Generate keys, create CSR, publish to TESAIoT, receive signed certificate |
| **4** | **Protected Update** | Full workflow: Request certificate renewal from TESAIoT Platform via Protected Update |
| **5** | Metadata Diagnostics | Read OPTIGA metadata, test certificate parsing with DER fallback |

---

## OPTIGA Trust M OID Reference

### Certificate and Key Slots

| OID | Name | Type | Purpose | Lifecycle |
|-----|------|------|---------|-----------|
| **0xE0C2** | Factory UID | Read-Only | Hardware identity (27 bytes) | Permanent |
| **0xE0E0** | Factory Certificate | Certificate | Pre-provisioned TLS certificate | Read-only after provisioning |
| **0xE0E1** | Device Certificate | Certificate | Operational certificate from CSR | Writable (up to 1728 bytes) |
| **0xE0E3** | Trust Anchor (ROOT_CA) | Public Key | Protected Update signature verification | Provisioned |
| **0xE0F0** | Factory Private Key | ECC P-256 | Paired with Factory Cert (0xE0E0) | Read-only |
| **0xE0F1** | Device Private Key | ECC P-256 | Paired with Device Cert (0xE0E1) | Generated during CSR |
| **0xF1D0-0xF1DF** | User Data Objects | Data | Application-specific storage | Read/Write |

### OID Access Conditions

```
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

```
+========================================================================================+
|                         TESAIoT Two-Certificate PKI Architecture                        |
+========================================================================================+

    FACTORY CERTIFICATE (Bootstrap)              DEVICE CERTIFICATE (Operational)
    +------------------------------+             +------------------------------+
    | OID: 0xE0E0                  |             | OID: 0xE0E1                  |
    | Key: 0xE0F0                  |             | Key: 0xE0F1                  |
    | Type: Pre-provisioned        |  -------->  | Type: Enrolled via CSR       |
    | Purpose: Initial TLS auth    |  CSR Flow   | Purpose: Production MQTT     |
    | Status: Read-only            |             | Status: Renewable            |
    +------------------------------+             +------------------------------+
              |                                            |
              v                                            v
    +------------------------------+             +------------------------------+
    | When Used:                   |             | When Used:                   |
    | - First boot                 |             | - After successful CSR       |
    | - After board reset          |             | - Normal production mode     |
    | - Recovery mode              |             | - Until cert expires/reset   |
    | - CSR workflow connection    |             |                              |
    +------------------------------+             +------------------------------+
```

### Certificate Selection Logic (SAFE MODE)

After device reset, the firmware uses **SAFE MODE** to ensure reliable operation:

```c
/*
 * Certificate Selection Priority (after reset):
 * 1. Force Factory Certificate (SAFE MODE) - Prevents key/cert mismatch
 * 2. After CSR workflow - Use Device Certificate
 */
```

**Why SAFE MODE?**
- Device Certificate (0xE0E1) may not match Device Key (0xE0F1) after reset
- Key generated during CSR is lost on power cycle (RAM only until cert written)
- Factory Certificate + Factory Key are always paired correctly
- Guarantees MQTT connection works for recovery

---

## CSR Workflow - Complete Flow

### Official State Machine Diagram

```
+==============================================================================+
|                    TESAIoT CSR Workflow State Machine                        |
+==============================================================================+

                         +------------------+
                         |      IDLE        |
                         +--------+---------+
                                  |
                                  | CSR Workflow selected
                                  | tesaiot_csr_workflow_start()
                                  v
                         +------------------+
                         | GENERATE_KEYPAIR |
                         |   OID: 0xE0F1    |
                         | ECC P-256 Key    |
                         +--------+---------+
                                  |
                                  | optiga_crypt_ecc_generate_keypair()
                                  | Public key: 68 bytes
                                  v
                         +------------------+
                         |   GENERATE_CSR   |
                         | X.509 CSR format |
                         | CN=<Device UID>  |
                         +--------+---------+
                                  |
                                  | CSR ~440-450 bytes (DER)
                                  | Convert to PEM for JSON
                                  v
                         +------------------+
                         |  CONNECT_MQTT    |
                         | Use Factory Cert |
                         |   OID: 0xE0E0    |
                         +--------+---------+
                                  |
                                  | TLS handshake with 0xE0E0 + 0xE0F0
                                  | mqtt.tesaiot.com:8883
                                  v
                         +------------------+
                         |   PUBLISH_CSR    |
                         | Topic: device/   |
                         | <uuid>/commands/ |
                         | csr              |
                         +--------+---------+
                                  |
                                  | JSON: {"uid":"...", "csr":"-----BEGIN..."}
                                  v
                         +------------------+
                         | WAIT_CERTIFICATE |
                         | Subscribe to:    |
                         | device/<uuid>/   |
                         | commands/#       |
                         +--------+---------+
                                  |
                                  | Platform signs CSR, sends cert
                                  | Timeout: 60 seconds
                                  v
                         +------------------+
                         | WRITE_TO_OPTIGA  |
                         |   OID: 0xE0E1    |
                         | DER format ~639  |
                         | bytes            |
                         +--------+---------+
                                  |
                                  | optiga_util_write_data(0xE0E1)
                                  | Reset fallback flag
                                  v
                         +------------------+
                         |      DONE        |
                         | Device Cert now  |
                         | matches Device   |
                         | Key (0xE0F1)     |
                         +------------------+
                                  |
                                  v
                    +---------------------------+
                    | Ready for Production MQTT |
                    | Using 0xE0E1 + 0xE0F1     |
                    +---------------------------+
```

### MQTT Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `device/<uuid>/commands/csr` | Device → Platform | Publish CSR request |
| `device/<uuid>/commands/#` | Platform → Device | Receive signed certificate |
| `device/<uuid>/telemetry` | Device → Platform | Operational data |

---

## Certificate Renewal Use Cases

### When Does Certificate Renewal Happen?

| Scenario | Trigger | Action Required |
|----------|---------|-----------------|
| **First Boot** | No Device Cert exists | Run CSR Workflow |
| **After Board Reset** | Key/Cert mismatch possible | Factory Cert used (SAFE MODE), then CSR Workflow |
| **Certificate Expired** | TLS handshake fails | Run CSR Workflow to get new certificate |
| **Before Expiry** | Proactive renewal | Run CSR Workflow (manual trigger) |
| **Key Compromise** | Security incident | Run CSR Workflow to generate new keypair |
| **Factory Reset** | User-initiated | Factory Cert used, then CSR Workflow |

### Certificate Renewal Flow Diagram

```
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
| User runs CSR    |--------------->| CSR Workflow     |
| Workflow         |                | Runs completely  |
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

```
Timeline: Certificate Lifecycle
============================================================================

   Factory Provisioning          CSR Workflow           Certificate Expires
          |                           |                        |
          v                           v                        v
   [FACTORY CERT]              [DEVICE CERT]             [RENEWAL NEEDED]
   Valid: 10 years             Valid: 1 year             Must run CSR Workflow
          |                           |                        |
          +------ Bootstrap ----------+------ Production ------+

   Renewal Trigger Points:
   1. Before Expiry (Recommended):
      - Run CSR Workflow anytime to get fresh certificate
      - Platform issues new cert with extended validity

   2. At Expiry:
      - TLS handshake fails with Device Cert
      - System falls back to Factory Cert (SAFE MODE)
      - User runs CSR Workflow to renew

   3. After Expiry (Recovery):
      - Factory Cert still valid (longer validity period)
      - Connect to platform, run CSR Workflow
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
- TESAIoT Platform account (for CSR workflow)

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

```
1. Power on device
2. Connect serial terminal (115200 baud)
3. Wait for Wi-Fi connection and NTP sync
4. Observe SAFE MODE message (Factory Certificate)
5. Select CSR Workflow from menu
6. Wait for "Workflow completed successfully"
7. Select MQTT Connection Test to verify Device Certificate connection
```

### After Board Reset

```
1. Power on device (or reset)
2. System automatically uses Factory Certificate (SAFE MODE)
3. MQTT Connection Test will work immediately (Factory Cert connection)
4. Run CSR Workflow if you need Device Certificate
5. After CSR Workflow, system uses Device Certificate
```

### Certificate Renewal

```
1. Connect to device terminal
2. If certificate expired: System uses Factory Cert (SAFE MODE)
3. Run CSR Workflow
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
| `TESAIOT_AUTO_CSR_RENEWAL_ENABLED` | tesaiot_debug_config.h | `0` | Auto CSR on Factory Cert connect |
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
- Automatic certificate rotation via CSR workflow
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
│   ├── tesaiot_csr_workflow.c       # CSR state machine
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

## TESAIoT Library Headers (v2.8.0)

The project uses consolidated TESAIoT headers:

```
tesaiot/include/
├── tesaiot.h                  # Main umbrella (includes all)
├── tesaiot_config.h           # Configuration
├── tesaiot_csr.h              # CSR workflow
├── tesaiot_license_config.h   # Customer editable
├── tesaiot_optiga.h           # OPTIGA integration
├── tesaiot_optiga_core.h      # OPTIGA manager
├── tesaiot_platform.h         # MQTT + SNTP
└── tesaiot_protected_update.h # Protected Update
```

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

**Last Updated:** 2026-02-01
