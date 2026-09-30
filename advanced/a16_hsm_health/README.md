# A14 — HSM Health Status Dashboard

Production-derived OPTIGA Trust M health status display adapted from `page_hsm.c` Tab 1 (Health/Status) in the TESAIoT firmware.

## What It Shows

- **Chip Identity**: OPTIGA Trust M V3 type, firmware version, lifecycle state, I2C address
- **UID Display**: Full 27-byte UID hex string from OID 0xE0C2
- **8-Point Health Check**: HW Init, UID Read, Cert Read, RNG, SHA-256, ECC Sign, Data R/W, Metadata — each with green/red status dot
- **Certificate Slots**: OIDs 0xE0E0-0xE0E3 with Pre-provisioned/Available/Empty status
- **Key Store Slots**: OIDs 0xE0F0-0xE0F3 with usage description and color-coded indicators
- **IPC Reference**: Documents IPC_CMD_HSM_REQUEST and IPC_CMD_HSM_HEALTH commands

## Key Patterns

- **Astro UX Status System**: Green (0x50D890) = OK, Amber (0xF2B84B) = Warning, Red (0xE85B5B) = Critical
- **2-column card grid**: Chip Identity + Health Check on top row, Certs + Keys on bottom row
- **Status dot indicators**: 10px circles with color-coded status
- **IPC architecture**: CM55 sends IPC command to CM33_NS, which accesses OPTIGA via I2C (SCB0, 100kHz)
- **CY_SECTION_SHAREDMEM**: TX/RX buffers must be in shared memory for IPC

## IPC Commands (Production Reference)

| Command | Code | Description |
|---------|------|-------------|
| `IPC_CMD_HSM_REQUEST` | 0xD0 | Read all OPTIGA data (UID, LCS, counters, certs) |
| `IPC_CMD_HSM_HEALTH` | 0xD5 | Run 8-point health check (60s timeout) |

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | HSM health dashboard with simulated OPTIGA data |
