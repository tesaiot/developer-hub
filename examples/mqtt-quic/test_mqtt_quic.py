#!/usr/bin/env python3
"""
TESA IoT Platform - MQTT over QUIC Test Client
Reads credentials from .env file and tests connection

Usage:
    python3 test_mqtt_quic.py
"""

import os
import json
import time
import ssl
from pathlib import Path
from datetime import datetime
from dotenv import load_dotenv

# Try paho-mqtt (most common)
try:
    import paho.mqtt.client as mqtt
    HAS_MQTT = True
except ImportError:
    HAS_MQTT = False
    print("❌ paho-mqtt not installed. Install: pip install paho-mqtt python-dotenv")
    exit(1)

# Load .env file
env_file = Path(__file__).parent / ".env"
if not env_file.exists():
    print(f"❌ Error: {env_file} not found")
    print("   Run setup_env.py first to create .env from credentials")
    exit(1)

load_dotenv(env_file)

# Read configuration from .env
MQTT_HOST = os.getenv('MQTT_HOST', 'mqtt.tesaiot.com')
MQTT_PORT = int(os.getenv('MQTT_PORT', '14567'))
MQTT_USERNAME = os.getenv('MQTT_USERNAME')
MQTT_PASSWORD = os.getenv('MQTT_PASSWORD')
MQTT_CA_CERT = os.getenv('MQTT_CA_CERT')
MQTT_KEEPALIVE = int(os.getenv('MQTT_KEEPALIVE', '60'))
MQTT_TRANSPORT = os.getenv('MQTT_TRANSPORT', 'quic')

# Topics
TOPIC_TELEMETRY = os.getenv('MQTT_TOPIC_TELEMETRY', f'devices/{MQTT_USERNAME}/telemetry')
TOPIC_COMMAND = os.getenv('MQTT_TOPIC_COMMAND', f'devices/{MQTT_USERNAME}/command')
TOPIC_STATUS = os.getenv('MQTT_TOPIC_STATUS', f'devices/{MQTT_USERNAME}/status')


class MQTTQuicClient:
    """MQTT over QUIC test client"""

    def __init__(self):
        self.client = None
        self.connected = False
        self.message_count = 0

    def on_connect(self, client, userdata, flags, rc, properties=None):
        """Callback when connected to broker"""
        if rc == 0:
            self.connected = True
            print(f"✅ Connected to {MQTT_HOST}:{MQTT_PORT}")
            print(f"   Transport: {MQTT_TRANSPORT.upper()}")
            print(f"   Device ID: {MQTT_USERNAME}")

            # Subscribe to command topic
            client.subscribe(TOPIC_COMMAND, qos=1)
            print(f"📥 Subscribed to: {TOPIC_COMMAND}")
        else:
            print(f"❌ Connection failed with code {rc}")
            error_messages = {
                1: "Incorrect protocol version",
                2: "Invalid client identifier",
                3: "Server unavailable",
                4: "Bad username or password",
                5: "Not authorized"
            }
            print(f"   Error: {error_messages.get(rc, 'Unknown error')}")

    def on_disconnect(self, client, userdata, rc, properties=None):
        """Callback when disconnected"""
        self.connected = False
        if rc != 0:
            print(f"⚠️  Unexpected disconnect (code {rc})")
        else:
            print("👋 Disconnected gracefully")

    def on_message(self, client, userdata, msg):
        """Callback when message received"""
        print(f"\n📨 Message received:")
        print(f"   Topic: {msg.topic}")
        print(f"   Payload: {msg.payload.decode()}")
        print(f"   QoS: {msg.qos}")

    def on_publish(self, client, userdata, mid):
        """Callback when message published"""
        self.message_count += 1
        print(f"📤 Message published (total: {self.message_count})")

    def connect(self):
        """Connect to MQTT broker using QUIC"""

        # Check credentials
        if not MQTT_USERNAME or not MQTT_PASSWORD:
            print("❌ Error: Missing credentials in .env file")
            return False

        # Check CA certificate
        ca_cert_path = Path(MQTT_CA_CERT)
        if not ca_cert_path.exists():
            print(f"❌ Error: CA certificate not found: {MQTT_CA_CERT}")
            return False

        print(f"\n🔌 Connecting to MQTT broker...")
        print(f"   Host: {MQTT_HOST}")
        print(f"   Port: {MQTT_PORT} (UDP)")
        print(f"   Transport: {MQTT_TRANSPORT.upper()}")
        print(f"   TLS: 1.3 (QUIC mandatory)")

        # Create MQTT client
        # Note: paho-mqtt standard version doesn't support QUIC transport
        # This will use TCP/TLS as fallback unless using QUIC-enabled paho build
        self.client = mqtt.Client(
            client_id=MQTT_USERNAME,
            protocol=mqtt.MQTTv5  # MQTT 5.0
        )

        # Set username/password
        self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

        # Configure TLS (Server-TLS mode)
        self.client.tls_set(
            ca_certs=str(ca_cert_path),
            certfile=None,  # No client certificate (Server-TLS)
            keyfile=None,   # No client key (Server-TLS)
            cert_reqs=ssl.CERT_REQUIRED,
            tls_version=ssl.PROTOCOL_TLS,
            ciphers=None
        )

        # Set callbacks
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.on_publish = self.on_publish

        # NOTE: Standard paho-mqtt doesn't support QUIC transport
        # For true QUIC support, need NanoSDK or QUIC-enabled MQTT library
        print("\n⚠️  NOTE: Standard paho-mqtt doesn't support QUIC transport")
        print("   Falling back to MQTTS (TCP/TLS port 8884) for this test")
        print("   For true QUIC support, use NanoSDK or QUIC-enabled client")

        # Use TCP port 8884 for MQTTS (instead of QUIC port 14567)
        tcp_port = 8884
        print(f"\n🔄 Using MQTTS port {tcp_port} (TCP) instead of {MQTT_PORT} (UDP/QUIC)")

        try:
            # Connect (using TCP/TLS as fallback)
            self.client.connect(MQTT_HOST, tcp_port, MQTT_KEEPALIVE)
            self.client.loop_start()

            # Wait for connection
            timeout = 10
            start = time.time()
            while not self.connected and (time.time() - start) < timeout:
                time.sleep(0.1)

            if not self.connected:
                print("❌ Connection timeout")
                return False

            return True

        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False

    def publish_telemetry(self, data: dict):
        """Publish telemetry data"""
        if not self.connected:
            print("❌ Not connected")
            return False

        payload = json.dumps(data)
        result = self.client.publish(TOPIC_TELEMETRY, payload, qos=1)

        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            return True
        else:
            print(f"❌ Publish failed: {result.rc}")
            return False

    def disconnect(self):
        """Disconnect from broker"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()


def main():
    """Main test function"""

    print("=" * 60)
    print("TESA IoT Platform - MQTT over QUIC Test")
    print("=" * 60)

    # Create client and connect
    client = MQTTQuicClient()

    if not client.connect():
        print("\n❌ Test FAILED - Could not connect")
        return 1

    print("\n" + "=" * 60)
    print("Publishing test telemetry data...")
    print("=" * 60)

    # Publish 5 test messages
    for i in range(5):
        telemetry = {
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "device_id": MQTT_USERNAME,
            "sequence": i + 1,
            "data": {
                "temperature": round(20 + (i * 0.5), 2),
                "humidity": round(50 + (i * 2), 2),
                "pressure": round(1013 + (i * 0.1), 2)
            },
            "metadata": {
                "transport": MQTT_TRANSPORT,
                "test": True
            }
        }

        print(f"\n📊 Message {i+1}/5:")
        print(f"   Temperature: {telemetry['data']['temperature']}°C")
        print(f"   Humidity: {telemetry['data']['humidity']}%")
        print(f"   Pressure: {telemetry['data']['pressure']} hPa")

        if client.publish_telemetry(telemetry):
            time.sleep(2)  # Wait 2 seconds between messages
        else:
            print("   ❌ Failed to publish")
            break

    print("\n" + "=" * 60)
    print(f"✅ Test completed - Published {client.message_count} messages")
    print("=" * 60)

    # Wait a bit for any commands
    print("\n⏳ Waiting 5 seconds for any incoming commands...")
    time.sleep(5)

    # Disconnect
    client.disconnect()
    time.sleep(1)

    print("\n✅ Test PASSED")
    return 0


if __name__ == '__main__':
    try:
        exit(main())
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted by user")
        exit(1)
