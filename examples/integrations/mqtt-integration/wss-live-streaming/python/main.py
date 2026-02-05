#!/usr/bin/env python3
"""
TESAIoT WSS Live Streaming - Python Client

Subscribe to real-time device telemetry via WebSocket Secure MQTT.

Prerequisites:
1. Generate an MQTT API Token from TESAIoT Admin UI
   (Organization Settings > MQTT API Tokens > Generate New Token)
2. Copy your token and save it in .env file

Usage:
    pip install -r requirements.txt
    cp .env.example .env
    # Edit .env with your token
    python main.py

See: https://github.com/tesaiot/developer-hub
"""

import json
import os
import signal
import ssl
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
        """Initialize the MQTT client with configuration from environment."""
        # Configuration from environment variables
        self.token = os.getenv("MQTT_API_TOKEN", "")
        self.broker_url = os.getenv(
            "MQTT_BROKER_URL", "wss://mqtt.tesaiot.com:8085/mqtt"
        )
        self.topic = os.getenv("MQTT_TOPIC", "device/+/telemetry/#")
        self.client_id = os.getenv(
            "MQTT_CLIENT_ID", f"tesaiot-python-{int(datetime.now().timestamp())}"
        )

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
        ca_cert = os.getenv("CA_CERT_PATH", None)
        if ca_cert and os.path.exists(ca_cert):
            self.client.tls_set(ca_certs=ca_cert, cert_reqs=ssl.CERT_REQUIRED)
        else:
            # Use system CA certs with relaxed verification for testing
            self.client.tls_set(cert_reqs=ssl.CERT_NONE)
            self.client.tls_insecure_set(True)

        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.client.on_subscribe = self._on_subscribe

        # Running flag for graceful shutdown
        self.running = True

    def _parse_broker_url(self, url: str) -> tuple[str, int, str]:
        """
        Parse WSS URL into host, port, and path components.

        Args:
            url: WSS URL (e.g., wss://mqtt.tesaiot.com:8085/mqtt)

        Returns:
            Tuple of (host, port, path)
        """
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

    def _validate_config(self) -> None:
        """Validate configuration and exit if invalid."""
        if not self.token:
            print("❌ ERROR: MQTT_API_TOKEN is required")
            print("")
            print("Please set MQTT_API_TOKEN in .env file or environment variable")
            print("Generate a token from TESAIoT Admin UI:")
            print("  1. Login to https://admin.tesaiot.com")
            print("  2. Go to Organization Settings > MQTT API Tokens")
            print('  3. Click "Generate New Token"')
            sys.exit(1)

        if not self.token.startswith("tesa_mqtt_"):
            print("❌ ERROR: Invalid token format")
            print("")
            print('Token should start with "tesa_mqtt_"')
            print(f"Current token: {self.token[:20]}...")
            sys.exit(1)

    def _on_connect(
        self,
        client: mqtt.Client,
        userdata: Any,
        flags: Any,
        reason_code: Any,
        properties: Optional[Any] = None,
    ) -> None:
        """Handle connection to broker."""
        # paho 1.x passes rc as int, paho 2.x passes reason_code object
        rc = (
            reason_code
            if isinstance(reason_code, int)
            else reason_code.value if hasattr(reason_code, "value") else 0
        )

        if rc == 0:
            print("✅ Connected to TESAIoT MQTT Broker!")

            # Subscribe to telemetry topic
            result, mid = client.subscribe(self.topic, qos=1)
            if result == mqtt.MQTT_ERR_SUCCESS:
                print(f"📡 Subscribing to: {self.topic}")
            else:
                print(f"❌ Subscription failed with error: {result}")
        else:
            print(f"❌ Connection failed with reason code: {rc}")
            if rc == 5:  # Not authorized
                print("   Check that your token is valid and not expired")

    def _on_subscribe(
        self,
        client: mqtt.Client,
        userdata: Any,
        mid: int,
        reason_codes: Any,
        properties: Optional[Any] = None,
    ) -> None:
        """Handle subscription confirmation."""
        print(f"📡 Subscribed to: {self.topic} (QoS 1)")
        print("")
        print("Waiting for telemetry messages...")
        print("─" * 50)
        print("")

    def _on_message(
        self, client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage
    ) -> None:
        """Handle incoming message."""
        # Parse topic to extract device ID
        # Topic format: device/<device_id>/telemetry/<sensor_type>
        topic_parts = msg.topic.split("/")
        device_id = topic_parts[1] if len(topic_parts) > 1 else "unknown"
        sensor_type = "/".join(topic_parts[3:]) if len(topic_parts) > 3 else "default"

        # Parse payload (assuming JSON format)
        try:
            data = json.loads(msg.payload.decode())
        except json.JSONDecodeError:
            data = {"raw": msg.payload.decode()}

        # Log received telemetry
        timestamp = datetime.now().isoformat()
        print(f"[{timestamp}] {msg.topic}")
        print(f"  Device: {device_id}")
        print(f"  Sensor: {sensor_type}")
        print(f"  Data: {json.dumps(data, indent=2)}")
        print("")

        # Process telemetry data
        self.process_message(device_id, sensor_type, data)

    def _on_disconnect(
        self,
        client: mqtt.Client,
        userdata: Any,
        reason_code: Any,
        properties: Optional[Any] = None,
    ) -> None:
        """Handle disconnection from broker."""
        rc = (
            reason_code
            if isinstance(reason_code, int)
            else reason_code.value if hasattr(reason_code, "value") else 0
        )
        if rc != 0:
            print(f"🔌 Disconnected from broker (rc={rc})")
            if self.running:
                print("   Attempting to reconnect...")

    def process_message(
        self, device_id: str, sensor_type: str, data: dict[str, Any]
    ) -> None:
        """
        Process received telemetry message.

        Customize this method to handle telemetry data:
        - Store in database
        - Forward to webhook
        - Trigger alerts
        - Update dashboard

        Args:
            device_id: Device UUID
            sensor_type: Sensor type (e.g., 'temperature', 'humidity')
            data: Parsed JSON payload
        """
        # Example: Add your custom processing logic here
        # - Store in PostgreSQL/MongoDB
        # - Send to Redis for real-time dashboard
        # - Trigger webhook for external systems
        # - Check thresholds and send alerts
        pass

    def run(self) -> None:
        """Connect to broker and start message loop."""
        self._display_banner()
        self._setup_signal_handlers()

        print("Connecting to TESAIoT MQTT Broker via WSS...")
        print(f"  Broker: {self.host}:{self.port}")
        print(f"  Path: {self.path}")
        print(f"  Client ID: {self.client_id}")
        print(f"  Token: {self.token[:20]}...{self.token[-4:]}")
        print("")

        try:
            self.client.connect(self.host, self.port)
            self.client.loop_forever()
        except KeyboardInterrupt:
            pass
        finally:
            self.client.disconnect()
            print("✅ Disconnected. Goodbye!")

    def _display_banner(self) -> None:
        """Display application banner."""
        print("")
        print("┌─────────────────────────────────────────────────┐")
        print("│  TESAIoT WSS Live Streaming - Python Client     │")
        print("└─────────────────────────────────────────────────┘")
        print("")

    def _setup_signal_handlers(self) -> None:
        """Setup graceful shutdown handlers."""

        def handler(signum: int, frame: Any) -> None:
            print("")
            print("Received shutdown signal. Disconnecting...")
            self.running = False
            self.client.disconnect()

        signal.signal(signal.SIGINT, handler)
        signal.signal(signal.SIGTERM, handler)


def main() -> None:
    """Main entry point."""
    client = TESAIoTMQTTClient()
    client.run()


if __name__ == "__main__":
    main()
