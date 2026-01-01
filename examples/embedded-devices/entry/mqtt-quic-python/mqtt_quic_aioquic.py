#!/usr/bin/env python3
"""
TESA IoT Platform - MQTT over QUIC Client using aioquic
Copyright (c) 2024-2025 Thai Embedded Systems Association (TESA)

This example demonstrates connecting to the TESA IoT Platform
using MQTT over QUIC with Server-TLS authentication.

Requirements:
- aioquic library: pip install aioquic
- Device credentials (device_id/password)

Usage:
    python3 mqtt_quic_aioquic.py <device_id> <password>

Example:
    python3 mqtt_quic_aioquic.py 95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9 MyPassword123

License: Apache 2.0
"""

import sys
import json
import struct
import asyncio
import ssl
import random
from datetime import datetime
from typing import Optional

try:
    from aioquic.asyncio import connect
    from aioquic.asyncio.protocol import QuicConnectionProtocol
    from aioquic.quic.configuration import QuicConfiguration
    from aioquic.quic.events import StreamDataReceived, ConnectionTerminated
except ImportError:
    print("[ERROR] aioquic not installed. Run: pip install aioquic")
    sys.exit(1)

# Configuration
MQTT_BROKER_HOST = "mqtt.tesaiot.com"
MQTT_BROKER_PORT = 14567
MQTT_KEEPALIVE = 60


class MQTTQuicProtocol(QuicConnectionProtocol):
    """MQTT over QUIC Protocol Handler"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.connected = False
        self.stream_id: Optional[int] = None
        self.received_data = bytearray()
        self.packet_id = 1

    def quic_event_received(self, event):
        if isinstance(event, StreamDataReceived):
            self.received_data.extend(event.data)
            self._process_mqtt_packets()
        elif isinstance(event, ConnectionTerminated):
            print(f"[WARNING] Connection terminated: {event.error_code}")
            self.connected = False

    def _process_mqtt_packets(self):
        """Process received MQTT packets"""
        while len(self.received_data) >= 2:
            packet_type = self.received_data[0] & 0xF0

            # Parse remaining length
            idx = 1
            remaining_length = 0
            multiplier = 1
            while idx < len(self.received_data):
                byte = self.received_data[idx]
                remaining_length += (byte & 0x7F) * multiplier
                multiplier *= 128
                idx += 1
                if (byte & 0x80) == 0:
                    break

            total_length = idx + remaining_length
            if len(self.received_data) < total_length:
                break

            packet = bytes(self.received_data[:total_length])
            self.received_data = self.received_data[total_length:]
            self._handle_mqtt_packet(packet_type, packet)

    def _handle_mqtt_packet(self, packet_type: int, packet: bytes):
        """Handle received MQTT packet"""
        if packet_type == 0x20:  # CONNACK
            if len(packet) >= 4:
                return_code = packet[3]
                if return_code == 0:
                    self.connected = True
                    print("[INFO] ✅ MQTT CONNACK - Connected successfully!")
                else:
                    errors = {1: "Protocol error", 2: "ID rejected", 3: "Server unavailable",
                              4: "Bad credentials", 5: "Not authorized"}
                    print(f"[ERROR] ❌ CONNACK rejected: {errors.get(return_code, 'Unknown')}")
        elif packet_type == 0x40:  # PUBACK
            print("[INFO] ✅ PUBACK received - Message delivered")
        elif packet_type == 0xD0:  # PINGRESP
            print("[DEBUG] PINGRESP received")

    async def mqtt_connect(self, client_id: str, username: str, password: str):
        """Send MQTT CONNECT packet"""
        self.stream_id = self._quic.get_next_available_stream_id()

        # Build MQTT CONNECT packet (MQTT 3.1.1)
        # Variable header: protocol name + level + flags + keepalive
        var_header = b'\x00\x04MQTT'  # Protocol name
        var_header += bytes([0x04])    # Protocol level (MQTT 3.1.1)
        var_header += bytes([0xC2])    # Connect flags: username, password, clean session
        var_header += struct.pack('>H', MQTT_KEEPALIVE)

        # Payload
        payload = struct.pack('>H', len(client_id)) + client_id.encode('utf-8')
        payload += struct.pack('>H', len(username)) + username.encode('utf-8')
        payload += struct.pack('>H', len(password)) + password.encode('utf-8')

        # Fixed header
        remaining = var_header + payload
        packet = bytes([0x10, len(remaining)]) + remaining

        self._quic.send_stream_data(self.stream_id, packet)
        self.transmit()

        # Wait for CONNACK
        for _ in range(50):
            await asyncio.sleep(0.1)
            if self.connected:
                return True
        return False

    async def mqtt_publish(self, topic: str, payload: str, qos: int = 1):
        """Publish MQTT message"""
        if not self.connected or self.stream_id is None:
            print("[ERROR] Not connected!")
            return False

        # Build PUBLISH packet
        topic_bytes = topic.encode('utf-8')
        payload_bytes = payload.encode('utf-8')

        var_header = struct.pack('>H', len(topic_bytes)) + topic_bytes
        if qos > 0:
            var_header += struct.pack('>H', self.packet_id)
            self.packet_id = (self.packet_id % 65535) + 1

        remaining = var_header + payload_bytes
        fixed_byte = 0x30 | (qos << 1)
        packet = bytes([fixed_byte, len(remaining)]) + remaining

        self._quic.send_stream_data(self.stream_id, packet)
        self.transmit()
        return True

    async def mqtt_disconnect(self):
        """Send MQTT DISCONNECT"""
        if self.stream_id is not None:
            self._quic.send_stream_data(self.stream_id, bytes([0xE0, 0x00]))
            self.transmit()


async def run_mqtt_quic_client(device_id: str, password: str):
    """Run MQTT over QUIC client"""
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  TESA IoT Platform - MQTT over QUIC Client (aioquic)")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

    # Configure QUIC with TLS 1.3
    configuration = QuicConfiguration(
        alpn_protocols=["mqtt"],  # EMQX MQTT over QUIC uses "mqtt" as ALPN
        is_client=True,
        verify_mode=ssl.CERT_NONE,  # Allow self-signed certs
    )

    print(f"[INFO] 🔌 Connecting to {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT} via QUIC...")
    print(f"[INFO]    Device ID: {device_id}")
    print(f"[INFO]    Transport: QUIC (UDP)")
    print(f"[INFO]    TLS Version: 1.3")

    try:
        async with connect(
            MQTT_BROKER_HOST,
            MQTT_BROKER_PORT,
            configuration=configuration,
            create_protocol=MQTTQuicProtocol,
        ) as protocol:
            print("[INFO] ✅ QUIC connection established!")

            # MQTT Connect
            print(f"\n[INFO] Sending MQTT CONNECT...")
            if not await protocol.mqtt_connect(device_id, device_id, password):
                print("[ERROR] ❌ MQTT connection failed - timeout!")
                return False

            print(f"\n[INFO] 🚀 Starting telemetry loop (Ctrl+C to stop)...\n")

            # Telemetry loop
            topic = f"device/{device_id}/telemetry/sensors"
            count = 0
            try:
                while True:
                    temperature = 20.0 + random.uniform(0, 10)
                    humidity = 40.0 + random.uniform(0, 30)

                    payload = json.dumps({
                        "timestamp": datetime.now().isoformat() + 'Z',
                        "data": {
                            "temperature": round(temperature, 2),
                            "humidity": round(humidity, 2)
                        }
                    })

                    print(f"[INFO] 📤 Publishing to {topic}")
                    print(f"[INFO]    Payload: {payload}")

                    if await protocol.mqtt_publish(topic, payload, qos=1):
                        count += 1
                        print(f"[INFO] ✅ Telemetry #{count} sent\n")

                    await asyncio.sleep(10)

            except asyncio.CancelledError:
                pass
            finally:
                print("\n[INFO] 🛑 Disconnecting...")
                await protocol.mqtt_disconnect()
                return True

    except Exception as e:
        print(f"[ERROR] ❌ Connection failed: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 mqtt_quic_aioquic.py <device_id> <password>")
        print("\nExample:")
        print("  python3 mqtt_quic_aioquic.py 95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9 MyPassword123")
        print("\nRequirements:")
        print("  pip install aioquic")
        sys.exit(1)

    device_id = sys.argv[1]
    password = sys.argv[2]

    try:
        asyncio.run(run_mqtt_quic_client(device_id, password))
    except KeyboardInterrupt:
        print("\n[INFO] ✅ Shutdown complete")


if __name__ == "__main__":
    main()
