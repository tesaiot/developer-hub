#!/usr/bin/env python3
"""
TESAIoT Platform - WSS MQTT Third-Party Integration Example (Python)

This example demonstrates how to connect to the TESAIoT MQTT broker
via WebSocket Secure (WSS) and subscribe to device telemetry data.

Prerequisites:
1. Generate an MQTT API Token from TESAIoT Admin UI
   (Organization Settings > MQTT API Tokens > Generate New Token)
2. Copy your token and save it in .env file

Usage:
    pip install -r requirements.txt
    cp .env.example .env
    # Edit .env with your token
    python main.py
"""

import os
import json
import signal
import sys
from datetime import datetime
from typing import Any, Optional

from dotenv import load_dotenv
import paho.mqtt.client as mqtt

# Load environment variables from .env file
load_dotenv()


class TESAIoTMQTTClient:
    """MQTT client for connecting to TESAIoT platform via WSS."""

    def __init__(self):
        # Configuration from environment variables
        self.token = os.getenv("MQTT_API_TOKEN", "")
        self.broker_url = os.getenv("MQTT_BROKER_URL", "wss://mqtt.tesaiot.com:8085/mqtt")
        self.subscribe_topic = os.getenv("MQTT_SUBSCRIBE_TOPIC", "device/+/telemetry/#")
        self.client_id = os.getenv("MQTT_CLIENT_ID", f"tesaiot-python-{datetime.now().timestamp():.0f}")

        # Parse broker URL
        self.host, self.port, self.path = self._parse_broker_url(self.broker_url)

        # Validate configuration
        self._validate_config()

        # Create MQTT client with WebSocket transport
        # Support both paho-mqtt 1.x and 2.x
        try:
            # paho-mqtt 2.x style
            self.client = mqtt.Client(
                client_id=self.client_id,
                transport="websockets",
                protocol=mqtt.MQTTv311,
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            )
            self._paho_v2 = True
        except (AttributeError, TypeError):
            # paho-mqtt 1.x fallback
            self.client = mqtt.Client(
                client_id=self.client_id,
                transport="websockets",
                protocol=mqtt.MQTTv311,
            )
            self._paho_v2 = False

        # Configure WebSocket path
        self.client.ws_set_options(path=self.path)

        # Set credentials (token is used for both username and password)
        self.client.username_pw_set(username=self.token, password=self.token)

        # Enable TLS for WSS
        # Load CA certificate from environment or use system default
        import ssl
        ca_cert = os.getenv("CA_CERT_PATH", None)
        if ca_cert and os.path.exists(ca_cert):
            self.client.tls_set(ca_certs=ca_cert, cert_reqs=ssl.CERT_REQUIRED)
        else:
            # Use system CA certs and allow unverified for self-signed certs
            self.client.tls_set(cert_reqs=ssl.CERT_NONE)
            self.client.tls_insecure_set(True)  # For testing with self-signed certs

        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.client.on_subscribe = self._on_subscribe

        # Running flag for graceful shutdown
        self.running = True

    def _parse_broker_url(self, url: str) -> tuple[str, int, str]:
        """Parse WSS URL into host, port, and path components."""
        # Remove protocol prefix
        if url.startswith("wss://"):
            url = url[6:]
        elif url.startswith("ws://"):
            url = url[5:]

        # Split path
        if "/" in url:
            host_port, path = url.split("/", 1)
            path = "/" + path
        else:
            host_port = url
            path = "/mqtt"

        # Split host and port
        if ":" in host_port:
            host, port_str = host_port.split(":")
            port = int(port_str)
        else:
            host = host_port
            port = 8085  # Default WSS port

        return host, port, path

    def _validate_config(self):
        """Validate configuration and exit if invalid."""
        if not self.token:
            print("ERROR: MQTT_API_TOKEN is required")
            print("Please set MQTT_API_TOKEN in .env file or environment variable")
            print("Generate a token from TESAIoT Admin UI > Organization Settings > MQTT API Tokens")
            sys.exit(1)

        if not self.token.startswith("tesa_mqtt_"):
            print("ERROR: Invalid token format")
            print('Token should start with "tesa_mqtt_"')
            sys.exit(1)

    def _on_connect(self, client, userdata, flags, reason_code, properties=None):
        """Callback when connected to broker."""
        # paho 1.x passes rc as int, paho 2.x passes reason_code object
        rc = reason_code if isinstance(reason_code, int) else reason_code.value if hasattr(reason_code, 'value') else 0
        if rc == 0:
            print("Connected to TESAIoT MQTT Broker!")

            # Subscribe to telemetry topic
            result, mid = client.subscribe(self.subscribe_topic, qos=1)
            if result == mqtt.MQTT_ERR_SUCCESS:
                print(f"Subscribing to: {self.subscribe_topic}")
            else:
                print(f"Subscription failed with error: {result}")
        else:
            print(f"Connection failed with reason code: {rc}")
            if rc == 5:  # Not authorized
                print("Authentication failed. Check your MQTT API Token.")
                print("Token may be expired, revoked, or invalid.")

    def _on_subscribe(self, client, userdata, mid, reason_codes=None, properties=None):
        """Callback when subscription is confirmed."""
        print(f"Subscribed successfully!")
        print(f"\nWaiting for telemetry messages...\n")

    def _on_message(self, client, userdata, msg):
        """Callback when message is received."""
        # Parse topic to extract device ID and sensor type
        # Topic format: device/<device_id>/telemetry/<sensor_type>
        topic_parts = msg.topic.split("/")
        device_id = topic_parts[1] if len(topic_parts) > 1 else "unknown"
        sensor_type = "/".join(topic_parts[3:]) if len(topic_parts) > 3 else "default"

        # Parse payload (assuming JSON format)
        try:
            data = json.loads(msg.payload.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            data = msg.payload.decode("utf-8", errors="replace")

        # Log received telemetry
        timestamp = datetime.now().isoformat()
        print(f"[{timestamp}] Device: {device_id}")
        print(f"  Sensor: {sensor_type}")
        print(f"  Data: {json.dumps(data, indent=2) if isinstance(data, dict) else data}")
        print()

        # Process telemetry data
        self.process_telemetry(device_id, sensor_type, data)

    def _on_disconnect(self, client, userdata, flags_or_rc, reason_code=None, properties=None):
        """Callback when disconnected from broker."""
        # paho 1.x: (client, userdata, rc)
        # paho 2.x: (client, userdata, flags, reason_code, properties)
        if reason_code is not None:
            rc = reason_code if isinstance(reason_code, int) else getattr(reason_code, 'value', 0)
        else:
            rc = flags_or_rc if isinstance(flags_or_rc, int) else 0
        print(f"Disconnected from MQTT broker (reason: {rc})")
        if self.running:
            print("Attempting to reconnect...")

    def process_telemetry(self, device_id: str, sensor_type: str, data: Any):
        """
        Process received telemetry data.
        Override this method to implement your business logic.

        Examples:
        - Store in time-series database (InfluxDB, TimescaleDB)
        - Send to analytics service
        - Check thresholds and trigger alerts
        - Update real-time dashboard via WebSocket
        - Forward to cloud services (AWS IoT, Azure IoT Hub, etc.)
        """
        pass  # Implement your logic here

    def connect(self):
        """Connect to MQTT broker."""
        print("Connecting to TESAIoT MQTT Broker via WSS...")
        print(f"  Broker: {self.host}:{self.port}{self.path}")
        print(f"  Client ID: {self.client_id}")
        print(f"  Token: {self.token[:20]}...{self.token[-4:]}")

        try:
            self.client.connect(self.host, self.port, keepalive=60)
        except Exception as e:
            print(f"Connection error: {e}")
            sys.exit(1)

    def run_forever(self):
        """Run the MQTT client loop."""
        self.client.loop_forever()

    def disconnect(self):
        """Disconnect from MQTT broker."""
        self.running = False
        self.client.disconnect()
        print("Disconnected from MQTT broker")


def main():
    """Main entry point."""
    # Create client
    client = TESAIoTMQTTClient()

    # Setup graceful shutdown handlers
    def signal_handler(signum, frame):
        print("\nShutting down...")
        client.disconnect()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Connect and run
    client.connect()

    print("Press Ctrl+C to exit\n")
    client.run_forever()


if __name__ == "__main__":
    main()
