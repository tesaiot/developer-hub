#!/usr/bin/env python3
"""
TESA IoT Platform - MQTT over QUIC Client Example (Python)
Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)

This example demonstrates connecting to the TESA IoT Platform
using MQTT over QUIC with Server-TLS authentication.

Requirements:
- NanoSDK Python bindings OR paho-mqtt with QUIC support
- CA certificate chain (ca-chain.pem from bundle)
- Device credentials (username/password)

Usage:
    python3 mqtt_quic_client.py <device_id> <password>

Example:
    python3 mqtt_quic_client.py 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123

License: Apache 2.0
"""

import sys
import json
import time
import ssl
import random
from datetime import datetime
from pathlib import Path

# Try importing NanoSDK first (recommended for QUIC)
try:
    import nng
    USING_NANOSDK = True
    print("[INFO] Using NanoSDK for MQTT over QUIC")
except ImportError:
    USING_NANOSDK = False
    # Fallback to paho-mqtt (Note: requires QUIC-enabled build)
    try:
        import paho.mqtt.client as mqtt
        print("[INFO] Using paho-mqtt (Note: QUIC support may be limited)")
    except ImportError:
        print("[ERROR] ❌ No MQTT library found!")
        print("[ERROR] Install NanoSDK Python bindings or paho-mqtt:")
        print("[ERROR]   pip install nng paho-mqtt")
        sys.exit(1)

# Configuration
MQTT_BROKER_HOST = "tesaiot.com"
MQTT_BROKER_PORT = 14567
MQTT_QOS = 1
MQTT_KEEPALIVE = 60
CA_CERT_PATH = "ca-chain.pem"
CONFIG_PATH = "mqtt-quic-config.json"


class MQTTQUICClient:
    """MQTT over QUIC client using NanoSDK or paho-mqtt"""

    def __init__(self, device_id: str, password: str):
        self.device_id = device_id
        self.password = password
        self.connected = False
        self.telemetry_topic = f"devices/{device_id}/telemetry"

        # Load configuration if available
        self.config = self._load_config()

    def _load_config(self) -> dict:
        """Load MQTT-QUIC configuration from bundle"""
        config_file = Path(CONFIG_PATH)
        if config_file.exists():
            try:
                with open(config_file, 'r') as f:
                    config = json.load(f)
                    print(f"[INFO] ✅ Loaded configuration from {CONFIG_PATH}")
                    return config
            except Exception as e:
                print(f"[WARNING] ⚠️  Failed to load config: {e}")

        # Default configuration
        return {
            "protocol": "mqtts",
            "host": MQTT_BROKER_HOST,
            "port": MQTT_BROKER_PORT,
            "transport": "quic",
            "tls": {
                "enabled": True,
                "version": "1.3",
                "ca_file": CA_CERT_PATH,
                "verify_server": True,
                "verify_mode": "server-only"
            },
            "auth": {
                "method": "username_password",
                "username": self.device_id,
                "password": self.password
            },
            "connection": {
                "keepalive": MQTT_KEEPALIVE,
                "clean_session": True,
                "connect_timeout": 10
            }
        }

    def _create_ssl_context(self) -> ssl.SSLContext:
        """Create SSL context for server verification"""
        ca_cert = Path(self.config['tls']['ca_file'])
        if not ca_cert.exists():
            raise FileNotFoundError(f"❌ CA certificate not found: {ca_cert}")

        # Create SSL context for TLS 1.3 (QUIC requires TLS 1.3)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.minimum_version = ssl.TLSVersion.TLSv1_3
        context.maximum_version = ssl.TLSVersion.TLSv1_3

        # Load CA certificate for server verification
        context.load_verify_locations(str(ca_cert))

        # Require server certificate verification
        context.check_hostname = True
        context.verify_mode = ssl.CERT_REQUIRED

        print(f"[INFO] ✅ SSL context created with TLS 1.3")
        print(f"[INFO]    CA certificate: {ca_cert}")
        print(f"[INFO]    Verify server: {self.config['tls']['verify_server']}")

        return context

    def on_connect(self, client, userdata, flags, rc):
        """Callback when connected to broker"""
        if rc == 0:
            self.connected = True
            print("[INFO] ✅ Connected to MQTT broker via QUIC")
            print(f"[INFO]    Client ID: {self.device_id}")
            print(f"[INFO]    Transport: QUIC (UDP)")
            print(f"[INFO]    TLS Version: 1.3")
        else:
            print(f"[ERROR] ❌ Connection failed with code: {rc}")
            error_messages = {
                1: "Incorrect protocol version",
                2: "Invalid client identifier",
                3: "Server unavailable",
                4: "Bad username or password",
                5: "Not authorized"
            }
            print(f"[ERROR]    Reason: {error_messages.get(rc, 'Unknown error')}")

    def on_disconnect(self, client, userdata, rc):
        """Callback when disconnected from broker"""
        self.connected = False
        if rc == 0:
            print("[INFO] 🛑 Disconnected gracefully")
        else:
            print(f"[WARNING] ⚠️  Unexpected disconnection (code: {rc})")

    def on_message(self, client, userdata, msg):
        """Callback when message received"""
        print(f"[INFO] 📩 Received message on topic: {msg.topic}")
        print(f"[INFO]    Payload: {msg.payload.decode()}")

    def on_publish(self, client, userdata, mid):
        """Callback when message published"""
        print(f"[INFO] ✅ Message published (mid: {mid})")

    def connect(self):
        """Connect to MQTT broker using QUIC"""
        print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        print("  TESA IoT Platform - MQTT over QUIC Client")
        print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

        if USING_NANOSDK:
            return self._connect_nanosdk()
        else:
            return self._connect_paho()

    def _connect_nanosdk(self):
        """Connect using NanoSDK (preferred for QUIC)"""
        print("[INFO] 🔌 Connecting via NanoSDK...")
        print(f"[INFO]    Endpoint: mqtts://{self.config['host']}:{self.config['port']}")
        print(f"[INFO]    Username: {self.device_id}")

        # Note: NanoSDK QUIC implementation would go here
        # This is a placeholder - actual NanoSDK QUIC API may differ
        print("[WARNING] ⚠️  NanoSDK QUIC implementation pending")
        print("[INFO] Falling back to paho-mqtt...")
        return self._connect_paho()

    def _connect_paho(self):
        """Connect using paho-mqtt"""
        print("[INFO] 🔌 Connecting via paho-mqtt...")
        print(f"[INFO]    Endpoint: mqtts://{self.config['host']}:{self.config['port']}")
        print(f"[INFO]    Username: {self.device_id}")
        print(f"[INFO]    Transport: {'QUIC (UDP)' if self.config['transport'] == 'quic' else 'TCP'}")

        # Create MQTT client
        self.client = mqtt.Client(
            client_id=self.device_id,
            clean_session=self.config['connection']['clean_session'],
            protocol=mqtt.MQTTv311
        )

        # Set authentication
        self.client.username_pw_set(
            self.device_id,
            self.password
        )

        # Set callbacks
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.on_publish = self.on_publish

        # Configure TLS/SSL
        ssl_context = self._create_ssl_context()
        self.client.tls_set_context(ssl_context)

        # Note: paho-mqtt may not support QUIC transport directly
        # This will use TLS over TCP instead of QUIC
        if self.config['transport'] == 'quic':
            print("[WARNING] ⚠️  paho-mqtt uses TCP+TLS, not QUIC")
            print("[WARNING]    For true QUIC support, use NanoSDK or QUIC-enabled client")

        # Connect to broker
        try:
            self.client.connect(
                self.config['host'],
                self.config['port'],
                self.config['connection']['keepalive']
            )

            # Start network loop
            self.client.loop_start()

            # Wait for connection
            timeout = self.config['connection']['connect_timeout']
            start_time = time.time()
            while not self.connected and (time.time() - start_time) < timeout:
                time.sleep(0.1)

            if not self.connected:
                raise ConnectionError("Connection timeout - no CONNACK received")

            return True

        except Exception as e:
            print(f"[ERROR] ❌ Connection failed: {e}")
            return False

    def publish_telemetry(self, temperature: float, humidity: float):
        """Publish telemetry data to broker"""
        timestamp = datetime.utcnow().isoformat() + 'Z'

        payload = {
            "timestamp": timestamp,
            "data": {
                "temperature": round(temperature, 2),
                "humidity": round(humidity, 2)
            }
        }

        payload_json = json.dumps(payload)

        print(f"[INFO] 📤 Publishing telemetry to {self.telemetry_topic}")
        print(f"[INFO]    Payload: {payload_json}")

        try:
            result = self.client.publish(
                self.telemetry_topic,
                payload_json,
                qos=MQTT_QOS
            )

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                return True
            else:
                print(f"[ERROR] ❌ Publish failed with code: {result.rc}")
                return False

        except Exception as e:
            print(f"[ERROR] ❌ Publish error: {e}")
            return False

    def disconnect(self):
        """Disconnect from broker"""
        print("\n[INFO] 🛑 Disconnecting from broker...")
        if hasattr(self, 'client'):
            self.client.loop_stop()
            self.client.disconnect()
        print("[INFO] ✅ Disconnected successfully")


def main():
    """Main function"""
    if len(sys.argv) != 3:
        print("Usage: python3 mqtt_quic_client.py <device_id> <password>")
        print("\nExample:")
        print("  python3 mqtt_quic_client.py 05f8968a-b400-4727-9678-b53cb0889fce MySecurePassword123")
        sys.exit(1)

    device_id = sys.argv[1]
    password = sys.argv[2]

    # Create and connect client
    client = MQTTQUICClient(device_id, password)

    if not client.connect():
        sys.exit(1)

    print("\n[INFO] 🚀 Starting telemetry loop (Ctrl+C to stop)...\n")

    # Main loop - publish telemetry every 10 seconds
    count = 0
    try:
        while True:
            # Simulate sensor readings
            temperature = 20.0 + random.uniform(0, 10)  # 20.0 - 30.0°C
            humidity = 40.0 + random.uniform(0, 30)     # 40.0 - 70.0%

            if client.publish_telemetry(temperature, humidity):
                count += 1
                print(f"[INFO] ✅ Telemetry #{count} sent successfully\n")

            # Sleep for 10 seconds
            time.sleep(10)

    except KeyboardInterrupt:
        print("\n[INFO] 🛑 Received Ctrl+C, shutting down...")
    finally:
        client.disconnect()

    print("[INFO] ✅ Shutdown complete")


if __name__ == "__main__":
    main()
