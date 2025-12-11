# C/C++ MQTT over QUIC Client Example

This example demonstrates connecting to the TESA IoT Platform using MQTT over QUIC with the NanoSDK library.

---

## Requirements

### System Requirements
- Linux, macOS, or Windows (WSL)
- GCC or Clang compiler
- CMake 3.10 or higher
- Git

### Dependencies

**NanoSDK Library (with QUIC support)**
```bash
git clone https://github.com/nanomq/NanoSDK.git
cd NanoSDK
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON -DNNG_ENABLE_QUIC=ON ..
make
sudo make install
```

**Note:** QUIC support requires NanoSDK built with `-DNNG_ENABLE_QUIC=ON`

---

## Setup

### 1. Download Bundle

1. Navigate to your device in the Admin UI
2. Go to **Credentials** tab
3. Click **Download MQTT-QUIC Server-TLS Bundle**
4. Extract the bundle to this directory

You should have:
- `ca-chain.pem` - CA certificate chain
- `mqtt-quic-config.json` - Connection configuration

### 2. Verify NanoSDK Installation

```bash
pkg-config --modversion nng
# OR
ldconfig -p | grep nng
```

### 3. Get Device Credentials

- **Username:** Your device_id
- **Password:** From bundle or password reset in Admin UI

---

## Build

### Using CMake (Recommended)

```bash
mkdir build
cd build
cmake ..
make
```

### Manual Compilation

```bash
gcc mqtt_quic_client.c -o mqtt_quic_client -lnng -lpthread
```

---

## Usage

### Basic Usage

```bash
./mqtt_quic_client <device_id> <password>
```

### Example

```bash
./mqtt_quic_client 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123
```

### Expected Output

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

[INFO] 📤 Publishing telemetry to devices/05f8968a-b400-4727-9678-b53cb0889fce/telemetry
[INFO]    Payload: {"timestamp":"2025-10-05T12:00:00Z","data":{"temperature":25.5,"humidity":60.2}}
[INFO] ✅ Telemetry #1 sent successfully
```

---

## Code Structure

### Main Components

1. **SSL/TLS Context Setup**
   - Loads CA certificate for server verification
   - Configures TLS 1.3 (required for QUIC)
   - Enables server name indication (SNI)

2. **QUIC Connection**
   - Creates MQTT QUIC socket
   - Applies TLS configuration
   - Connects to broker with username/password

3. **Message Publishing**
   - Publishes to topic: `devices/{device_id}/telemetry`
   - JSON payload with timestamp and sensor data
   - QoS 1 (at least once delivery)

4. **Event Callbacks**
   - `connect_cb` - Connection established/lost
   - `message_cb` - Message received (for subscriptions)

---

## NanoSDK MQTT over QUIC API

### Key Functions

**Socket Creation:**
```c
int nng_mqtt_quic_client_open(nng_socket *sock);
```

**TLS Configuration:**
```c
int nng_tls_config_alloc(nng_tls_config **cfg, nng_tls_mode mode);
int nng_tls_config_ca_chain(nng_tls_config *cfg, const char *ca, const char *crl);
int nng_tls_config_auth_mode(nng_tls_config *cfg, nng_tls_auth_mode mode);
int nng_tls_config_server_name(nng_tls_config *cfg, const char *name);
```

**Connection:**
```c
int nng_dialer_create(nng_dialer *d, nng_socket s, const char *url);
int nng_dialer_start(nng_dialer d, int flags);
```

**MQTT Messages:**
```c
int nng_mqtt_msg_alloc(nng_msg **msgp, size_t sz);
int nng_mqtt_msg_set_packet_type(nng_msg *msg, nng_mqtt_packet_type type);
int nng_mqtt_msg_set_publish_topic(nng_msg *msg, const char *topic);
int nng_mqtt_msg_set_publish_payload(nng_msg *msg, uint8_t *payload, uint32_t len);
```

---

## Troubleshooting

### Build Error: nng/nng.h not found

**Solution:** Install NanoSDK as shown in Requirements section

### Link Error: cannot find -lnng

**Solution:**
```bash
# Update library cache
sudo ldconfig

# OR specify library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Runtime Error: Failed to create MQTT QUIC socket

**Solution:** Ensure NanoSDK was built with QUIC support:
```bash
cmake -DBUILD_SHARED_LIBS=ON -DNNG_ENABLE_QUIC=ON ..
```

### Connection timeout

**Solution:**
1. Check firewall allows UDP port 14567 outbound
2. Verify `ca-chain.pem` exists and is readable
3. Test network connectivity:
   ```bash
   ping tesaiot.com
   ```

### Failed to set CA certificate

**Solution:**
1. Verify CA certificate format:
   ```bash
   openssl x509 -in ca-chain.pem -text -noout
   ```
2. Ensure file contains valid PEM certificates
3. Check file permissions (must be readable)

---

## Performance Testing

### Connection Time Measurement

Add timing code:
```c
#include <sys/time.h>

struct timeval start, end;
gettimeofday(&start, NULL);

// ... connection code ...

gettimeofday(&end, NULL);
long ms = (end.tv_sec - start.tv_sec) * 1000 +
          (end.tv_usec - start.tv_usec) / 1000;
printf("Connection time: %ld ms\n", ms);
```

### Throughput Testing

Publish 1000 messages and measure time:
```bash
time ./mqtt_quic_client <device_id> <password>
```

---

## Advanced Usage

### Subscribe to Commands

```c
// Subscribe after connection
nng_msg *submsg;
nng_mqtt_msg_alloc(&submsg, 0);
nng_mqtt_msg_set_packet_type(submsg, NNG_MQTT_SUBSCRIBE);

const char *topic = "devices/your-device-id/commands/#";
nng_mqtt_msg_set_subscribe_topic(submsg, topic, 1); // QoS 1

nng_sendmsg(sock, submsg, 0);

// Handle incoming messages in message_cb callback
```

### Custom Keep-Alive

```c
nng_mqtt_msg_set_connect_keep_alive(connmsg, 120); // 120 seconds
```

### Last Will and Testament

```c
const char *lwt_topic = "devices/your-device-id/status";
const char *lwt_payload = "{\"status\":\"offline\"}";

nng_mqtt_msg_set_connect_will_topic(connmsg, lwt_topic);
nng_mqtt_msg_set_connect_will_msg(connmsg, (uint8_t *)lwt_payload, strlen(lwt_payload));
nng_mqtt_msg_set_connect_will_qos(connmsg, 1);
nng_mqtt_msg_set_connect_will_retain(connmsg, true);
```

---

## Debugging

### Enable NNG Debug Logging

```bash
export NNG_DEBUG=1
./mqtt_quic_client <device_id> <password>
```

### Verbose Output

Add to code:
```c
nng_socket_set_int(sock, NNG_OPT_RECVMAXSZ, 0);  // Unlimited
nng_socket_set_ms(sock, NNG_OPT_RECVTIMEO, 5000); // 5s timeout
```

### Packet Capture

```bash
# Capture QUIC traffic
sudo tcpdump -i any -n udp port 14567 -w quic.pcap

# Analyze with Wireshark
wireshark quic.pcap
```

---

## References

- **NanoSDK GitHub:** https://github.com/nanomq/NanoSDK
- **NanoSDK Documentation:** https://nanomq.io/docs/en/latest/
- **MQTT 3.1.1 Spec:** https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/
- **QUIC Protocol:** https://www.rfc-editor.org/rfc/rfc9000.html
- **NNG Library:** https://nng.nanomsg.org/

---

## License

Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)

Licensed under Apache License 2.0
