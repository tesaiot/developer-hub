#!/usr/bin/env python3
"""
Tests for TESAIoT MQTT WSS Client

Run with: pytest tests/test_mqtt_client.py -v
"""

import json
import os
import sys
import ssl
from datetime import datetime
from unittest.mock import MagicMock, Mock, patch, call

import pytest

# Add parent directory to path to import main module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from main import TESAIoTMQTTClient


class TestConfigLoading:
    """Tests for configuration loading from environment variables."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
        "MQTT_BROKER_URL": "wss://mqtt.tesaiot.com:8085/mqtt",
        "MQTT_TOPIC": "device/+/telemetry/#",
        "MQTT_CLIENT_ID": "test-client-123",
        "CA_CERT_PATH": "/path/to/ca.crt"
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_config_loaded_from_environment(self, mock_client_class):
        """Test that all configuration values are loaded from environment variables."""
        client = TESAIoTMQTTClient()

        assert client.token == "tesa_mqtt_testorg_1234567890123456789012345678"
        assert client.broker_url == "wss://mqtt.tesaiot.com:8085/mqtt"
        assert client.topic == "device/+/telemetry/#"
        assert client.client_id == "test-client-123"

    @patch.dict(os.environ, {}, clear=True)
    @patch("main.mqtt.Client")
    def test_config_defaults(self, mock_client_class):
        """Test that default values are used when environment variables are not set."""
        # We need to provide a token to avoid validation error
        with patch.object(TESAIoTMQTTClient, "_validate_config"):
            client = TESAIoTMQTTClient()

            assert client.broker_url == "wss://mqtt.tesaiot.com:8085/mqtt"
            assert client.topic == "device/+/telemetry/#"
            assert client.client_id.startswith("tesaiot-python-")

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_token_from_environment(self, mock_client_class):
        """Test that token is correctly loaded from environment."""
        client = TESAIoTMQTTClient()
        assert client.token == "tesa_mqtt_testorg_1234567890123456789012345678"


class TestURLParsing:
    """Tests for WSS URL parsing functionality."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_standard_wss_url(self, mock_client_class):
        """Test parsing a standard WSS URL with host, port, and path."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com:8085/mqtt")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085
        assert path == "/mqtt"

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_ws_url(self, mock_client_class):
        """Test parsing a WS (non-secure) URL."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("ws://localhost:8083/mqtt")

        assert host == "localhost"
        assert port == 8083
        assert path == "/mqtt"

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_url_without_port(self, mock_client_class):
        """Test parsing URL without explicit port uses default."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com/mqtt")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085  # Default WSS port
        assert path == "/mqtt"

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_url_without_path(self, mock_client_class):
        """Test parsing URL without explicit path uses default."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com:8085")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085
        assert path == "/mqtt"  # Default path

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_url_with_nested_path(self, mock_client_class):
        """Test parsing URL with nested path."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("wss://broker.example.com:9001/ws/mqtt")

        assert host == "broker.example.com"
        assert port == 9001
        assert path == "/ws/mqtt"

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_parse_url_with_ip_address(self, mock_client_class):
        """Test parsing URL with IP address."""
        client = TESAIoTMQTTClient()

        host, port, path = client._parse_broker_url("wss://192.168.1.100:8085/mqtt")

        assert host == "192.168.1.100"
        assert port == 8085
        assert path == "/mqtt"


class TestTokenValidation:
    """Tests for token validation functionality."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_valid_token_format(self, mock_client_class):
        """Test that valid token passes validation."""
        # Should not raise any exception
        client = TESAIoTMQTTClient()
        assert client.token.startswith("tesa_mqtt_")

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_missing_token_raises_error(self, mock_client_class):
        """Test that missing token causes system exit."""
        with pytest.raises(SystemExit) as exc_info:
            TESAIoTMQTTClient()
        assert exc_info.value.code == 1

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "invalid_token_format",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_invalid_token_format_raises_error(self, mock_client_class):
        """Test that invalid token format causes system exit."""
        with pytest.raises(SystemExit) as exc_info:
            TESAIoTMQTTClient()
        assert exc_info.value.code == 1

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "TESA_MQTT_UPPERCASE_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_token_case_sensitive(self, mock_client_class):
        """Test that token prefix is case sensitive."""
        with pytest.raises(SystemExit) as exc_info:
            TESAIoTMQTTClient()
        assert exc_info.value.code == 1


class TestMessageProcessing:
    """Tests for message processing functionality."""

    @pytest.fixture
    def mock_mqtt_client(self):
        """Create a mock MQTT client."""
        return MagicMock()

    @pytest.fixture
    def tesaiot_client(self, mock_mqtt_client):
        """Create a TESAIoTMQTTClient with mocked MQTT client."""
        with patch.dict(os.environ, {
            "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
        }, clear=True):
            with patch("main.mqtt.Client", return_value=mock_mqtt_client):
                client = TESAIoTMQTTClient()
                return client

    def test_on_connect_success(self, tesaiot_client, mock_mqtt_client):
        """Test successful connection handler."""
        # Mock the flags and reason code
        mock_flags = MagicMock()

        # Test with paho 2.x style reason code
        mock_reason_code = MagicMock()
        mock_reason_code.value = 0

        # Mock subscribe to return (result, mid) tuple as expected by paho-mqtt
        mock_mqtt_client.subscribe.return_value = (0, 1)  # MQTT_ERR_SUCCESS, message_id

        tesaiot_client._on_connect(mock_mqtt_client, None, mock_flags, mock_reason_code)

        # Should subscribe to the topic
        mock_mqtt_client.subscribe.assert_called_once_with("device/+/telemetry/#", qos=1)

    def test_on_connect_failure(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test connection failure handler."""
        mock_flags = MagicMock()

        # Simulate connection refused (code 5)
        mock_reason_code = MagicMock()
        mock_reason_code.value = 5

        tesaiot_client._on_connect(mock_mqtt_client, None, mock_flags, mock_reason_code)

        # Should not subscribe on failure
        mock_mqtt_client.subscribe.assert_not_called()

        captured = capsys.readouterr()
        assert "Connection failed" in captured.out

    def test_on_connect_paho_v1_style(self, tesaiot_client, mock_mqtt_client):
        """Test connection handler with paho v1 style integer reason code."""
        mock_flags = MagicMock()

        # Mock subscribe to return (result, mid) tuple as expected by paho-mqtt
        mock_mqtt_client.subscribe.return_value = (0, 1)  # MQTT_ERR_SUCCESS, message_id

        # paho 1.x passes integer directly
        tesaiot_client._on_connect(mock_mqtt_client, None, mock_flags, 0)

        mock_mqtt_client.subscribe.assert_called_once_with("device/+/telemetry/#", qos=1)

    def test_on_message_json_payload(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test message handler with JSON payload."""
        # Create a mock message
        mock_msg = MagicMock()
        mock_msg.topic = "device/abc-123/telemetry/temperature"
        mock_msg.payload = json.dumps({"value": 25.5, "unit": "celsius"}).encode()

        tesaiot_client._on_message(mock_mqtt_client, None, mock_msg)

        captured = capsys.readouterr()
        assert "abc-123" in captured.out
        assert "temperature" in captured.out
        assert "25.5" in captured.out

    def test_on_message_non_json_payload(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test message handler with non-JSON payload."""
        mock_msg = MagicMock()
        mock_msg.topic = "device/abc-123/telemetry/status"
        mock_msg.payload = b"online"

        tesaiot_client._on_message(mock_mqtt_client, None, mock_msg)

        captured = capsys.readouterr()
        assert "abc-123" in captured.out
        assert "online" in captured.out

    def test_on_message_topic_parsing(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test correct extraction of device ID and sensor type from topic."""
        mock_msg = MagicMock()
        mock_msg.topic = "device/my-device-456/telemetry/humidity/relative"
        mock_msg.payload = json.dumps({"value": 60}).encode()

        tesaiot_client._on_message(mock_mqtt_client, None, mock_msg)

        captured = capsys.readouterr()
        assert "my-device-456" in captured.out
        assert "humidity/relative" in captured.out

    def test_on_message_short_topic(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test message handler with short topic format."""
        mock_msg = MagicMock()
        mock_msg.topic = "device/short-topic"
        mock_msg.payload = json.dumps({"data": "test"}).encode()

        tesaiot_client._on_message(mock_mqtt_client, None, mock_msg)

        captured = capsys.readouterr()
        assert "short-topic" in captured.out

    def test_process_message_called(self, tesaiot_client, mock_mqtt_client):
        """Test that process_message is called with correct arguments."""
        mock_msg = MagicMock()
        mock_msg.topic = "device/test-device/telemetry/pressure"
        mock_msg.payload = json.dumps({"value": 1013}).encode()

        with patch.object(tesaiot_client, "process_message") as mock_process:
            tesaiot_client._on_message(mock_mqtt_client, None, mock_msg)

            mock_process.assert_called_once()
            args = mock_process.call_args[0]
            assert args[0] == "test-device"
            assert args[1] == "pressure"
            assert args[2] == {"value": 1013}

    def test_on_subscribe(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test subscribe confirmation handler."""
        tesaiot_client._on_subscribe(mock_mqtt_client, None, 1, [1])

        captured = capsys.readouterr()
        assert "Subscribed to" in captured.out

    def test_on_disconnect_clean(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test clean disconnect handler."""
        tesaiot_client.running = False

        mock_reason_code = MagicMock()
        mock_reason_code.value = 0

        tesaiot_client._on_disconnect(mock_mqtt_client, None, mock_reason_code)

        captured = capsys.readouterr()
        # Clean disconnect should not print reconnection message
        assert "Attempting to reconnect" not in captured.out

    def test_on_disconnect_unclean(self, tesaiot_client, mock_mqtt_client, capsys):
        """Test unclean disconnect handler."""
        tesaiot_client.running = True

        mock_reason_code = MagicMock()
        mock_reason_code.value = 1

        tesaiot_client._on_disconnect(mock_mqtt_client, None, mock_reason_code)

        captured = capsys.readouterr()
        assert "Disconnected" in captured.out
        assert "Attempting to reconnect" in captured.out


class TestMQTTClientSetup:
    """Tests for MQTT client initialization and setup."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_client_created_with_websockets(self, mock_client_class):
        """Test that client is created with WebSocket transport."""
        TESAIoTMQTTClient()

        mock_client_class.assert_called_once()
        call_kwargs = mock_client_class.call_args.kwargs
        assert call_kwargs.get("transport") == "websockets"

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_websocket_path_set(self, mock_client_class):
        """Test that WebSocket path is configured."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        TESAIoTMQTTClient()

        mock_client.ws_set_options.assert_called_once_with(path="/mqtt")

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_credentials_set(self, mock_client_class):
        """Test that credentials are set with token as both username and password."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        TESAIoTMQTTClient()

        mock_client.username_pw_set.assert_called_once_with(
            username="tesa_mqtt_testorg_1234567890123456789012345678",
            password="tesa_mqtt_testorg_1234567890123456789012345678"
        )

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
        "CA_CERT_PATH": "/path/to/ca.crt"
    }, clear=True)
    @patch("os.path.exists", return_value=True)
    @patch("main.mqtt.Client")
    def test_tls_with_ca_cert(self, mock_client_class, mock_exists):
        """Test TLS configuration with CA certificate."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        TESAIoTMQTTClient()

        mock_client.tls_set.assert_called_once_with(
            ca_certs="/path/to/ca.crt",
            cert_reqs=ssl.CERT_REQUIRED
        )

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_tls_without_ca_cert(self, mock_client_class):
        """Test TLS configuration without CA certificate (insecure mode)."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        TESAIoTMQTTClient()

        mock_client.tls_set.assert_called_once_with(cert_reqs=ssl.CERT_NONE)
        mock_client.tls_insecure_set.assert_called_once_with(True)

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_callbacks_registered(self, mock_client_class):
        """Test that all MQTT callbacks are registered."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        client = TESAIoTMQTTClient()

        assert mock_client.on_connect == client._on_connect
        assert mock_client.on_message == client._on_message
        assert mock_client.on_disconnect == client._on_disconnect
        assert mock_client.on_subscribe == client._on_subscribe


class TestProcessMessage:
    """Tests for the process_message method."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_process_message_accepts_parameters(self, mock_client_class):
        """Test that process_message accepts the expected parameters."""
        client = TESAIoTMQTTClient()

        # Should not raise any exception
        client.process_message(
            device_id="test-device",
            sensor_type="temperature",
            data={"value": 25.5, "unit": "C"}
        )

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_process_message_with_empty_data(self, mock_client_class):
        """Test that process_message handles empty data."""
        client = TESAIoTMQTTClient()

        # Should not raise any exception
        client.process_message(
            device_id="test-device",
            sensor_type="status",
            data={}
        )

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_process_message_with_nested_data(self, mock_client_class):
        """Test that process_message handles nested data structures."""
        client = TESAIoTMQTTClient()

        nested_data = {
            "sensors": {
                "temperature": {"value": 25.5, "unit": "C"},
                "humidity": {"value": 60, "unit": "%"}
            },
            "timestamp": "2024-01-01T00:00:00Z"
        }

        # Should not raise any exception
        client.process_message(
            device_id="test-device",
            sensor_type="multi",
            data=nested_data
        )


class TestClientRun:
    """Tests for the client run method."""

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_run_connects_to_broker(self, mock_client_class):
        """Test that run method connects to the broker."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        client = TESAIoTMQTTClient()

        with patch.object(client, "_display_banner"):
            with patch.object(client, "_setup_signal_handlers"):
                # Simulate KeyboardInterrupt to exit loop_forever
                mock_client.loop_forever.side_effect = KeyboardInterrupt()
                client.run()

        mock_client.connect.assert_called_once_with("mqtt.tesaiot.com", 8085)
        mock_client.loop_forever.assert_called_once()

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_run_disconnects_on_exit(self, mock_client_class):
        """Test that run method disconnects on exit."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        client = TESAIoTMQTTClient()

        with patch.object(client, "_display_banner"):
            with patch.object(client, "_setup_signal_handlers"):
                mock_client.loop_forever.side_effect = KeyboardInterrupt()
                client.run()

        mock_client.disconnect.assert_called_once()

    @patch.dict(os.environ, {
        "MQTT_API_TOKEN": "tesa_mqtt_testorg_1234567890123456789012345678",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_signal_handler(self, mock_client_class):
        """Test signal handler sets running to false and disconnects."""
        mock_client = MagicMock()
        mock_client_class.return_value = mock_client

        client = TESAIoTMQTTClient()
        client.running = True

        with patch("signal.signal") as mock_signal:
            client._setup_signal_handlers()

            # Get the handler function that was registered
            handler_calls = mock_signal.call_args_list
            sigint_handler = None
            for call_args in handler_calls:
                if call_args[0][0] == __import__("signal").SIGINT:
                    sigint_handler = call_args[0][1]
                    break

            # Simulate signal
            if sigint_handler:
                sigint_handler(__import__("signal").SIGINT, None)

        assert client.running is False


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
