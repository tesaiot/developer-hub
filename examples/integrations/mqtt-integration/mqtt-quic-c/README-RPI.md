# MQTT over QUIC Client for Raspberry Pi

Build and run the C/C++ MQTT over QUIC client on Raspberry Pi.

## Prerequisites

- Raspberry Pi OS (Bullseye or later)
- CMake 3.10+
- GCC/G++ compiler
- Git

```bash
sudo apt update
sudo apt install -y cmake gcc g++ git libssl-dev
```

## Install NanoSDK (NNG with QUIC support)

NanoSDK is the NNG fork with MQTT over QUIC support by EMQX.

### Option 1: Build from Source (Recommended)

```bash
# Clone NanoSDK (NNG with QUIC support)
cd ~
git clone https://github.com/nanomq/NanoSDK.git
cd NanoSDK

# Initialize submodules (including MsQuic)
git submodule update --init --recursive

# Build MsQuic first (required for QUIC)
cd extern/msquic
mkdir build && cd build
cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
cd ../../..

# Build NanoSDK with QUIC support
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON \
      -DNNG_ENABLE_QUIC=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc)
sudo make install

# Update library cache
sudo ldconfig
```

### Option 2: Pre-built Package (if available)

Check if pre-built packages are available:

```bash
# For ARM64 Raspberry Pi (RPi 4/5)
# Check NanoMQ releases: https://github.com/emqx/nanomq/releases
```

## Build the MQTT QUIC Client

```bash
# Navigate to the example directory
cd /path/to/mqtt-quic-c

# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)
```

## Prepare Credentials

1. Copy the CA certificate from your MQTT-QUIC bundle:

```bash
cp /path/to/bundle/ca-chain.pem ./
```

2. Note your device credentials:
   - Device ID (username)
   - Password

## Run the Client

```bash
./mqtt_quic_client <device_id> <password>

# Example:
./mqtt_quic_client 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123
```

## Expected Output

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  TESA IoT Platform - MQTT over QUIC Client
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[INFO] ✅ CA certificate loaded successfully
[INFO] 🔌 Connecting to mqtts://tesaiot.com:14567 via QUIC...
[INFO]    Username: 05f8968a-b400-4727-9678-b53cb0889fce
[INFO]    Transport: QUIC (UDP)
[INFO]    TLS Version: 1.3
[INFO] ✅ Connected to MQTT broker via QUIC
[INFO] ✅ Successfully connected and authenticated

[INFO] 🚀 Starting telemetry loop (Ctrl+C to stop)...

[INFO] 📤 Publishing telemetry to devices/05f8968a-.../telemetry
[INFO]    Payload: {"timestamp":"2025-12-30T12:00:00Z","data":{"temperature":25.50,"humidity":55.30}}
[INFO] ✅ Telemetry #1 sent successfully
```

## Troubleshooting

### Build Errors

**Error:** `nng/nng.h: No such file or directory`

```bash
# Verify NanoSDK is installed
ls /usr/local/include/nng/

# If not found, reinstall NanoSDK
cd ~/NanoSDK/build
sudo make install
sudo ldconfig
```

**Error:** `undefined reference to nng_mqtt_quic_client_open`

NanoSDK was built without QUIC support. Rebuild with:

```bash
cmake -DNNG_ENABLE_QUIC=ON ..
make clean && make -j$(nproc)
sudo make install
```

### Runtime Errors

**Error:** `QUIC connection failed` or `Connection timeout`

1. Verify UDP port 14567 is not blocked:
   ```bash
   nc -uvz tesaiot.com 14567
   ```

2. Check firewall rules:
   ```bash
   sudo iptables -L -n | grep 14567
   ```

3. Test with TCP fallback (port 8884):
   - Modify code to use TCP instead of QUIC

### Certificate Errors

**Error:** `SSL certificate verify failed`

```bash
# Verify CA certificate is valid
openssl x509 -in ca-chain.pem -text -noout

# Check expiration
openssl x509 -in ca-chain.pem -enddate -noout
```

## Performance Tips for Raspberry Pi

1. **Use wired Ethernet** for stable UDP performance
2. **Disable WiFi power saving** if using wireless:
   ```bash
   sudo iw wlan0 set power_save off
   ```
3. **Increase UDP buffer sizes**:
   ```bash
   sudo sysctl -w net.core.rmem_max=2500000
   sudo sysctl -w net.core.wmem_max=2500000
   ```

## See Also

- [NanoSDK Documentation](https://github.com/nanomq/NanoSDK)
- [MQTT over QUIC Specification](https://github.com/mqtt/mqtt.org/wiki/MQTT-over-QUIC)
- [MsQuic on ARM](https://github.com/microsoft/msquic/blob/main/docs/Platforms.md)

## License

Apache 2.0 - Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)
