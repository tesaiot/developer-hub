# TESAIoT OTA HTTPS Client (C Version)

A C implementation of the TESAIoT OTA (Over-The-Air) update client for embedded devices.

## Overview

This is a reference implementation that demonstrates how to:
- Poll the TESAIoT Platform for firmware updates
- Parse job documents containing update information
- Download firmware with integrity verification
- Report update status back to the platform

## Files

| File | Description |
|------|-------------|
| `ota_config.h` | Configuration constants and data structures |
| `ota_client.h` | OTA client API declarations |
| `ota_client.c` | OTA client implementation (uses libcurl) |
| `main.c` | Command-line interface program |
| `Makefile` | Build configuration |

## Building

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libcurl4-openssl-dev

# macOS
brew install curl

# Fedora/RHEL
sudo dnf install gcc libcurl-devel
```

### Compile

```bash
make
```

### Clean

```bash
make clean
```

## Usage

```bash
# Show help
./ota_client --help

# Check for updates (single poll)
./ota_client --device-id "your-device-uuid" \
    --server "admin.tesaiot.com" \
    --token "your-jwt-token"

# Continuous polling every 60 seconds
./ota_client -d "device-uuid" -s "admin.tesaiot.com" -t "token" -l 60

# Check job document only (don't download)
./ota_client -d "device-uuid" -s "admin.tesaiot.com" -t "token" --job-only

# Local testing without TLS
./ota_client -d "test-device" -s "localhost" -p 8080 --no-tls
```

## API Overview

### Initialization

```c
ota_client_t ota;
ota_init(&ota, "device-uuid", "1.0.0", "PSE84");
ota_set_server(&ota, "admin.tesaiot.com", 443, 1);
ota_set_auth_token(&ota, "jwt-token");
```

### Check for Updates

```c
ota_error_t err = ota_check_for_update(&ota);
if (err == OTA_OK) {
    printf("Update available: %s v%s\n",
           ota.job.firmware_id, ota.job.version);
}
```

### Full Update Cycle

```c
ota_error_t err = ota_run_update_cycle(&ota);
if (err == OTA_OK) {
    printf("Update successful!\n");
}
```

## Porting to Embedded Platforms

To use this on embedded devices like PSoC Edge (E84), ESP32, or STM32:

1. **Replace libcurl** with platform-specific HTTP client:
   - PSoC Edge: Use Cypress WiFi + HTTP libraries
   - ESP32: Use ESP-IDF HTTP client
   - STM32: Use LwIP + mbedTLS

2. **Modify TLS handling** in `ota_client.c`:
   - Use hardware TLS acceleration if available
   - Integrate with secure element (Trust M, ATECC608A)

3. **Implement flash callbacks**:
   ```c
   ota.write_chunk = my_flash_write;
   ota.verify_firmware = my_verify_checksum;
   ota.apply_firmware = my_bootloader_switch;
   ```

4. **Adjust buffer sizes** in `ota_config.h` for constrained memory

## Protocol Details

### OTA Job Endpoint

```
GET /api/v1/ota/devices/{device_id}/job
Headers:
  X-Device-Version: 1.0.0
  X-Hardware-Type: PSE84
  Authorization: Bearer <token>

Response 200: Job document (update available)
Response 204: No update available
```

### Infineon Chunk Format

For PSoC Edge devices, firmware is delivered in 4KB chunks with 32-byte headers:

```c
typedef struct {
    uint8_t  magic[8];       // "OTAImage"
    uint16_t offset_to_data;
    uint16_t chunk_number;
    uint16_t chunk_size;
    uint16_t total_chunks;
    uint32_t total_size;
    uint32_t crc32;
    uint16_t reserved[3];
} infineon_chunk_header_t;
```

## License

Copyright (c) 2026 TESAIoT Platform (TESA)
Licensed under Apache License 2.0
