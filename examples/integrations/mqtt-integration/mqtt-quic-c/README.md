# MQTT over QUIC - C/C++ Example (NanoSDK + MsQuic)

This example demonstrates connecting to TESAIoT Platform using **MQTT over QUIC** with the NanoSDK library (C/C++).

## Supported Platforms

| Platform | Status |
|----------|--------|
| Linux (Ubuntu/Debian) | ✅ Fully supported |
| Raspberry Pi (ARM64) | ✅ Fully supported |
| macOS | ❌ Not supported (MsQuic uses Linux-only APIs) |
| Windows | ❌ Not supported |

## Features

- **50% faster initial connection** (1-RTT vs 2-RTT)
- **90% faster reconnection** (0-RTT session resumption)
- **Connection migration** (survives IP address changes)
- **TLS 1.3** built into QUIC protocol
- **Lightweight C implementation** for embedded devices

## Prerequisites

| Item | Description |
|------|-------------|
| GCC | 11+ with C11 support |
| CMake | 3.16+ |
| MsQuic | Microsoft QUIC library |
| NanoSDK | NanoMQ SDK with QUIC support |
| Credentials | MQTT-QUIC bundle from TESAIoT Platform |

## Installation

### Linux (Ubuntu/Debian 22.04+)

```bash
# Run the install script
chmod +x install_deps.sh
./install_deps.sh

# Or manually install dependencies:
sudo apt update && sudo apt install -y \
    build-essential cmake pkg-config git \
    libssl-dev liblttng-ust-dev lttng-tools

# Install MsQuic (from Microsoft)
wget https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt update && sudo apt install -y libmsquic

# Clone and build NanoSDK
git clone https://github.com/nanomq/NanoSDK.git
cd NanoSDK && mkdir build && cd build
cmake -DNNG_ENABLE_QUIC=ON ..
make && sudo make install
```

### Raspberry Pi (ARM64)

```bash
# Install dependencies
sudo apt update && sudo apt install -y \
    build-essential cmake pkg-config git \
    libssl-dev

# MsQuic for ARM64 - build from source
git clone --recursive https://github.com/microsoft/msquic.git
cd msquic
mkdir build && cd build
cmake -G 'Unix Makefiles' ..
cmake --build . --config Release
sudo cmake --install .

# Clone and build NanoSDK
git clone https://github.com/nanomq/NanoSDK.git
cd NanoSDK && mkdir build && cd build
cmake -DNNG_ENABLE_QUIC=ON ..
make && sudo make install
```

## Getting Credentials from TESAIoT Platform

### Step 1: Download MQTT-QUIC Bundle

1. Login to **TESAIoT Admin Portal**: https://admin.tesaiot.com
2. Navigate to **Devices** > Select your device
3. Go to the **Credentials** tab
4. Click **Download MQTT-QUIC Server-TLS Bundle**
5. Extract the bundle to get:
   - `ca-chain.pem` - CA certificate chain
   - `mqtt-quic-config.json` - Connection configuration
   - `endpoints.json` - MQTT-QUIC endpoint info

### Step 2: Place Credentials

Copy the CA certificate from bundle:
```bash
cp /path/to/downloaded/bundle/ca-chain.pem ./certs/
```

**Alternatively**, store in shared credentials folder:
```bash
cp -r /path/to/downloaded/bundle/* ../../shared/devices_credentials/<device-name>/
```

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# The executable will be at ./mqtt_quic_client
```

## Configuration

Edit the source or use environment variables:

```bash
export MQTT_HOST=mqtt.tesaiot.com
export MQTT_PORT=14567
export MQTT_USERNAME=your-device-id
export MQTT_PASSWORD=your-password
export MQTT_CA_CERT=./certs/ca-chain.pem
```

## Running the Example

```bash
./mqtt_quic_client
```

## Connection Details

| Setting | Value |
|---------|-------|
| Protocol | MQTT over QUIC |
| Host | mqtt.tesaiot.com |
| Port | 14567 (UDP) |
| TLS | 1.3 (built-in) |
| Auth | Username/Password |

## Shared Credentials Folder

> **Note**: The `examples/shared/devices_credentials/` folder is for storing downloaded credentials. This folder is NOT committed to git repositories.

## Network Requirements

- **Firewall**: Allow UDP port 14567 outbound
- **NAT**: QUIC supports NAT traversal (no special config needed)
- **IPv4/IPv6**: Both supported

## Requirements

### Server-TLS Authentication Mode

These examples use **Server-TLS** authentication mode:
- Server presents certificate (verified by CA chain)
- Client authenticates with **username (device_id)** and **password**
- **No client certificate required**

---

## Examples

### 1. C/C++ Example (NanoSDK)

**Directory:** `c_cpp/`

Uses the NanoSDK library which supports MQTT over QUIC.

**Features:**
- QUIC connection with TLS 1.3
- Server certificate verification
- Username/password authentication
- Automatic reconnection
- Telemetry publishing

**Files:**
- `mqtt_quic_client.c` - Main client implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Detailed build and run instructions

### 2. Python Example (Paho MQTT)

**Directory:** `python/`

Uses paho-mqtt library (requires QUIC support build).

**Features:**
- QUIC connection using NanoSDK Python bindings
- Configuration loaded from `mqtt-quic-config.json`
- TLS/SSL context with CA certificate
- Telemetry publishing with JSON payload
- Error handling and logging

**Files:**
- `mqtt_quic_client.py` - Main client implementation
- `requirements.txt` - Python dependencies
- `README.md` - Detailed setup and run instructions

---

## Connection Details

### Endpoint
```
Protocol: mqtts://
Host: tesaiot.com
Port: 14567
Transport: QUIC (UDP)
```

### Authentication
```
Method: Username + Password
Username: {your_device_id}
Password: {from bundle or password reset}
```

### TLS Configuration
```
Version: TLS 1.3 (mandatory, built into QUIC)
CA Certificate: ca-chain.pem (from bundle)
Verify Server: Yes
Client Certificate: Not required (Server-TLS mode)
```

---

## Performance Comparison

| Metric | TCP+TLS (MQTTs) | QUIC+TLS (MQTT-QUIC) | Improvement |
|--------|----------------|---------------------|-------------|
| Initial Connection | 2-RTT (~200ms) | 1-RTT (~100ms) | 50% faster |
| Reconnection | 2-RTT | 0-RTT (~10ms) | 90% faster |
| IP Change | Reconnect required | Connection migration | Seamless |
| Packet Loss Impact | Head-of-line blocking | Independent streams | Better resilience |
| Network Conditions | Good on stable networks | Excellent on weak networks | 40% better |

---

## Use Cases

### Ideal For:
- **Internet of Vehicles (IoV)** - Seamless connectivity while moving
- **Mobile IoT Devices** - Weak/intermittent cellular networks
- **Low-latency Applications** - Real-time control systems
- **High-density Deployments** - Resource-efficient connections

### Not Recommended For:
- Corporate networks that block UDP traffic
- Devices that require mTLS authentication (use MQTTs port 8883 instead)
- Environments where TCP is mandated by policy

---

## Troubleshooting

### Connection Fails

**Error:** `Connection timeout` or `QUIC connection failed`

**Solutions:**
1. Check firewall allows UDP port 14567 outbound
2. Verify `ca-chain.pem` is present and readable
3. Check network supports UDP (some corporate firewalls block it)
4. Try TCP fallback: Use MQTTs (port 8884) instead

### Certificate Verification Fails

**Error:** `SSL certificate verify failed` or `CA certificate not found`

**Solutions:**
1. Verify `ca-chain.pem` path is correct
2. Check certificate file is not corrupted: `openssl x509 -in ca-chain.pem -text -noout`
3. Ensure certificate has not expired

### Authentication Fails

**Error:** `Connection refused: Bad user name or password`

**Solutions:**
1. Verify device_id is correct (username)
2. Reset password in Credentials tab and update in code
3. Check password was copied correctly (no extra whitespace)

---

## Network Requirements

- **Firewall:** Allow UDP port 14567 outbound
- **NAT:** QUIC supports NAT traversal (no special config needed)
- **IPv4/IPv6:** Both supported
- **MTU:** Recommend 1280 bytes minimum (QUIC standard)

---

## Support

- **Documentation:** https://docs.tesaiot.com/mqtt-quic
- **Example Issues:** https://github.com/tesaiot/examples/issues
- **Contact:** support@tesaiot.com

---

## License

Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)

Licensed under Apache License 2.0 - see LICENSE file for details.
