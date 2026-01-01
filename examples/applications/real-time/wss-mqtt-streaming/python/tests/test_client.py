# Copyright (c) 2025 TESAIoT Platform (TESA)
# Licensed under Apache License 2.0

"""Unit tests for WSS MQTT client."""

import pytest
from unittest.mock import patch, MagicMock
import os


class TestTESAIoTMQTTClientParsing:
    """Test URL parsing functionality."""

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_parse_broker_url_full(self, mock_client):
        """Full WSS URL should be parsed correctly."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com:8085/mqtt")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085
        assert path == "/mqtt"

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_parse_broker_url_no_path(self, mock_client):
        """URL without path should default to /mqtt."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com:8085")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085
        assert path == "/mqtt"

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_parse_broker_url_no_port(self, mock_client):
        """URL without port should default to 8085."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        host, port, path = client._parse_broker_url("wss://mqtt.tesaiot.com/mqtt")

        assert host == "mqtt.tesaiot.com"
        assert port == 8085
        assert path == "/mqtt"

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_parse_broker_url_ws_protocol(self, mock_client):
        """WS protocol should also work."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        host, port, path = client._parse_broker_url("ws://localhost:8080/ws")

        assert host == "localhost"
        assert port == 8080
        assert path == "/ws"


class TestTESAIoTMQTTClientValidation:
    """Test configuration validation."""

    @patch.dict(os.environ, {"MQTT_API_TOKEN": ""}, clear=True)
    @patch("paho.mqtt.client.Client")
    def test_missing_token_exits(self, mock_client):
        """Missing token should exit."""
        from main import TESAIoTMQTTClient

        with pytest.raises(SystemExit) as exc_info:
            TESAIoTMQTTClient()
        assert exc_info.value.code == 1

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "invalid_token"}, clear=True)
    @patch("paho.mqtt.client.Client")
    def test_invalid_token_format_exits(self, mock_client):
        """Token without correct prefix should exit."""
        from main import TESAIoTMQTTClient

        with pytest.raises(SystemExit) as exc_info:
            TESAIoTMQTTClient()
        assert exc_info.value.code == 1


class TestTESAIoTMQTTClientCallbacks:
    """Test MQTT callback handlers."""

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_on_connect_success(self, mock_client):
        """Successful connection should subscribe to topic."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()

        # Create mock MQTT client
        mock_mqtt = MagicMock()
        mock_mqtt.subscribe.return_value = (0, 1)  # MQTT_ERR_SUCCESS, mid

        # Call on_connect with success code
        client._on_connect(mock_mqtt, None, None, 0, None)

        # Should subscribe to topic
        mock_mqtt.subscribe.assert_called_once()

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_on_connect_failure(self, mock_client, capsys):
        """Failed connection should print error."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        mock_mqtt = MagicMock()

        # Call on_connect with auth failure code
        client._on_connect(mock_mqtt, None, None, 5, None)

        captured = capsys.readouterr()
        assert "Authentication failed" in captured.out

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_on_message_json_payload(self, mock_client, capsys):
        """JSON message should be parsed and printed."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        mock_mqtt = MagicMock()

        # Create mock message
        mock_msg = MagicMock()
        mock_msg.topic = "device/test-device-001/telemetry/sensor"
        mock_msg.payload = b'{"temperature": 25.5}'

        client._on_message(mock_mqtt, None, mock_msg)

        captured = capsys.readouterr()
        assert "test-device-001" in captured.out
        assert "temperature" in captured.out

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_on_message_non_json_payload(self, mock_client, capsys):
        """Non-JSON message should still be handled."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()
        mock_mqtt = MagicMock()

        # Create mock message with plain text
        mock_msg = MagicMock()
        mock_msg.topic = "device/test-device/telemetry/raw"
        mock_msg.payload = b"plain text data"

        client._on_message(mock_mqtt, None, mock_msg)

        captured = capsys.readouterr()
        assert "test-device" in captured.out
        assert "plain text data" in captured.out


class TestTESAIoTMQTTClientMethods:
    """Test client methods."""

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_disconnect(self, mock_client):
        """Disconnect should set running to False."""
        from main import TESAIoTMQTTClient

        mock_instance = mock_client.return_value
        client = TESAIoTMQTTClient()

        assert client.running is True

        client.disconnect()

        assert client.running is False
        mock_instance.disconnect.assert_called_once()

    @patch.dict(os.environ, {"MQTT_API_TOKEN": "tesa_mqtt_test_token_12345"})
    @patch("paho.mqtt.client.Client")
    def test_process_telemetry_hook(self, mock_client):
        """process_telemetry should be callable."""
        from main import TESAIoTMQTTClient

        client = TESAIoTMQTTClient()

        # Should not raise any errors
        client.process_telemetry("device-001", "temperature", {"value": 25.5})
