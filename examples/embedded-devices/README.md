# Embedded Devices Examples

Examples for IoT developers working with MCU and Embedded Linux platforms.

## Target Audience

| Platform Type | Examples | Architecture |
|---------------|----------|--------------|
| **Cortex-M** | ESP32, STM32, Microchip SAMD, PSoC | Bare-metal, RTOS (FreeRTOS) |
| **Cortex-A** | Raspberry Pi, Jetson, BeagleBone | Linux, Android |
| **RISC-V** | ESP32-C6, K210 | Bare-metal, Linux |
| **Others** | Arduino, FPGA (with soft-core) | Various |

## Learning Path

```
Level 1: Entry
├── device-servertls     (C - Linux/RTOS, Server TLS)
├── mqtt-quic-python     (Python - QUIC protocol)
└── rpi-servertls        (Python - Raspberry Pi)

Level 2: Intermediate
├── device-mtls          (C - Mutual TLS)
├── esp32-servertls      (Arduino - ESP32)
├── stm32-servertls      (STM32CubeIDE)
└── rpi-mtls             (Python - CSR → Platform)

Level 3: Advanced
├── optiga-trustm        (Linux + Trust M HSM)
├── rpi-mtls-trustm      (RPI + Trust M)
└── psoc-edge-reference  (PSoC Edge E84)
```

## Quick Start

### Entry Level - Start Here

1. **[device-servertls](./entry/device-servertls/)** - Simple TLS connection with API key
2. **[rpi-servertls](./entry/rpi-servertls/)** - Raspberry Pi with Python

### Intermediate - Add Security

1. **[device-mtls](./intermediate/device-mtls/)** - Mutual TLS with client certificates
2. **[rpi-mtls](./intermediate/rpi-mtls/)** - RPI with CSR workflow

---

## Examples

### Entry Level

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [device-servertls](./entry/device-servertls/) | C | Server TLS + API Key auth | Ready |
| [mqtt-quic-python](./entry/mqtt-quic-python/) | Python | MQTT over QUIC protocol | Ready |
| [rpi-servertls](./entry/rpi-servertls/) | Python | Raspberry Pi Server TLS | NEW |

### Intermediate Level

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [device-mtls](./intermediate/device-mtls/) | C | Mutual TLS authentication | Ready |
| [esp32-servertls](./intermediate/esp32-servertls/) | Arduino C++ | ESP32 MQTT client | NEW |
| [stm32-servertls](./intermediate/stm32-servertls/) | C | STM32CubeIDE project | Planned |
| [rpi-mtls](./intermediate/rpi-mtls/) | Python | RPI with CSR signing | NEW |

### Advanced Level

| Example | Language | Description | Status |
|---------|----------|-------------|--------|
| [optiga-trustm](./advanced/optiga-trustm/) | C | Linux + OPTIGA Trust M | Future |
| [rpi-mtls-trustm](./advanced/rpi-mtls-trustm/) | Python/C | RPI + Trust M HSM | Future |
| [psoc-edge-reference](./advanced/psoc-edge-reference/) | C | PSoC Edge E84 | Separate Repo |

---

## Key Skills

- MQTT/MQTTS protocol implementation
- TLS certificate handling
- Schema-driven telemetry generation
- Resource-constrained device programming
- Hardware Security Module integration

---

## NCSA Compliance

| Level | Security Feature | Examples |
|-------|------------------|----------|
| Level 1 | TLS 1.2+, API Key | device-servertls |
| Level 2 | Mutual TLS, Client Certs | device-mtls, rpi-mtls |
| Level 3 | HSM, Secure Boot | optiga-trustm |

---

**Category:** Embedded Devices
**Last Updated:** 2025-12-27
