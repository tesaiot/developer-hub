# TESAIoT Headers

**Version:** 2.8.0
**Last Updated:** 2026-02-01

## Header Files (8 files)

| Header | Description |
|--------|-------------|
| `tesaiot.h` | Main umbrella - includes all headers + license API |
| `tesaiot_config.h` | Configuration, debug levels, OID definitions |
| `tesaiot_csr.h` | CSR workflow API |
| `tesaiot_license_config.h` | Customer editable (UID + license key) |
| `tesaiot_optiga.h` | OPTIGA Trust M integration |
| `tesaiot_optiga_core.h` | OPTIGA manager (init, acquire, release) |
| `tesaiot_platform.h` | Platform services (MQTT, SNTP) |
| `tesaiot_protected_update.h` | Protected Update workflow |

## Usage

### Option 1: Single Include (Recommended)

```c
#include "tesaiot.h"  // Includes everything
```

### Option 2: Domain-Specific Includes

```c
#include "tesaiot_platform.h"  // MQTT + SNTP only
#include "tesaiot_csr.h"       // CSR workflow only
#include "tesaiot_optiga.h"    // OPTIGA integration only
```

## Customer Configuration

Edit `tesaiot_license_config.h` with your device UID and license key:

```c
#define TESAIOT_DEVICE_UID    "your_device_uid_here"
#define TESAIOT_LICENSE_KEY   "your_license_key_here"
```

## Header Dependency Graph

```
tesaiot.h (main umbrella)
├── tesaiot_config.h
├── tesaiot_platform.h (MQTT + SNTP)
├── tesaiot_csr.h
├── tesaiot_protected_update.h
├── tesaiot_optiga.h
└── tesaiot_optiga_core.h

tesaiot_license_config.h (standalone - customer fills UID + key)
```

---

(c) 2025-2026 TESAIoT AIoT Foundation Platform
