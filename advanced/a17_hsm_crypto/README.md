# A15 — HSM Crypto Benchmark

Production-derived OPTIGA Trust M crypto benchmark display adapted from `page_hsm.c` Tab 4 (Crypto Benchmark) in the TESAIoT firmware.

## What It Shows

- **5 Crypto Operations**: Random (32B), SHA-256 (256B), ECC P-256 KeyGen, ECDSA Sign, ECDSA Verify
- **Typical vs Actual timing**: Datasheet typical times compared with measured results
- **Progress bars**: Visual progress for each operation during sequential benchmark
- **Run All**: Sequential execution of all 5 benchmarks with progress animation
- **Individual Run**: Per-operation execution buttons for targeted testing
- **IPC command reference**: Each operation shows its IPC command code

## Key Patterns

- **Benchmark table layout**: Operation | Typical (ms) | Actual (ms) | Result — matches production UI
- **Sequential timer animation**: `lv_timer_create` drives progress bars one operation at a time
- **Color-coded timing**: Green if actual <= 2x typical, amber otherwise (Astro UX)
- **IPC architecture**: CM55 sends `IPC_CMD_HSM_BENCHMARK` to CM33_NS, which calls OPTIGA crypto APIs
- **CY_SECTION_SHAREDMEM**: TX/RX buffers must be in shared memory for IPC

## IPC Commands (Production Reference)

| Command | Code | Description |
|---------|------|-------------|
| `IPC_CMD_HSM_BENCHMARK` | 0xD4 | Run all 5 crypto benchmarks (30s timeout) |
| `IPC_CMD_OPTIGA_SHA256` | 0xE3 | SHA-256 hash |
| `IPC_CMD_OPTIGA_SIGN` | 0xE4 | ECDSA signature |
| `IPC_CMD_OPTIGA_RANDOM` | 0xE2 | Random bytes |

## Typical Timings (Infineon SLS 32AIA Datasheet)

| Operation | Typical (ms) |
|-----------|-------------|
| Random (32 B) | 15 |
| SHA-256 (256 B) | 30 |
| ECC P-256 Key Gen | 80 |
| ECDSA Sign | 70 |
| ECDSA Verify | 80 |

## Files

| File | Purpose |
|------|---------|
| `main_example.c` | Crypto benchmark UI with simulated timing |
