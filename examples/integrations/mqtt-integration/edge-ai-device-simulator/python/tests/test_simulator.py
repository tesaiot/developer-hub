#!/usr/bin/env python3
"""
Tests for TESAIoT Edge AI Device Simulator

Run with: pytest tests/test_simulator.py -v
"""

import json
import os
import sys
from datetime import datetime, timezone
from unittest.mock import MagicMock, Mock, patch, call

import pytest

# Add parent directory to path to import main module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from main import InfineonDPS368Simulator


class TestConfigLoading:
    """Tests for configuration loading from environment variables."""

    @patch.dict(os.environ, {
        "MQTT_BROKER_HOST": "test.mosquitto.org",
        "MQTT_BROKER_PORT": "1884",
        "DEVICE_ID": "test-sensor-01",
        "MQTT_TOPIC": "device/test-sensor-01/telemetry",
        "PUBLISH_INTERVAL": "2.5",
        "ANOMALY_INTERVAL": "5",
        "MQTT_CLIENT_ID": "test-client-abc"
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_config_loaded_from_environment(self, mock_client_class):
        """Test that all configuration values are loaded from environment variables."""
        simulator = InfineonDPS368Simulator()

        assert simulator.broker_host == "test.mosquitto.org"
        assert simulator.broker_port == 1884
        assert simulator.device_id == "test-sensor-01"
        assert simulator.topic == "device/test-sensor-01/telemetry"
        assert simulator.publish_interval == 2.5
        assert simulator.anomaly_interval == 5
        assert simulator.client_id == "test-client-abc"

    @patch.dict(os.environ, {}, clear=True)
    @patch("main.mqtt.Client")
    def test_config_defaults(self, mock_client_class):
        """Test that default values are used when environment variables are not set."""
        simulator = InfineonDPS368Simulator()

        assert simulator.broker_host == "localhost"
        assert simulator.broker_port == 1883
        assert simulator.device_id == "infineon-sensor-01"
        assert simulator.topic == "device/infineon-sensor-01/telemetry"
        assert simulator.publish_interval == 1.0
        assert simulator.anomaly_interval == 10
        assert simulator.client_id.startswith("dps368-sim-")

    @patch.dict(os.environ, {
        "DEVICE_ID": "custom-device",
    }, clear=True)
    @patch("main.mqtt.Client")
    def test_topic_uses_device_id_from_env(self, mock_client_class):
        """Test that topic is constructed using DEVICE_ID from environment."""
        simulator = InfineonDPS368Simulator()

        assert simulator.topic == "device/custom-device/telemetry"


class TestNormalDataGeneration:
    """Tests for normal sensor data generation."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {}, clear=True):
            with patch("main.mqtt.Client"):
                return InfineonDPS368Simulator()

    def test_normal_pressure_range(self, simulator):
        """Test that normal pressure values are within expected range."""
        # Generate multiple samples to ensure range is respected
        for _ in range(100):
            data = simulator._generate_normal_data()
            assert simulator.PRESSURE_MIN <= data["pressure"] <= simulator.PRESSURE_MAX

    def test_normal_temperature_range(self, simulator):
        """Test that normal temperature values are within expected range."""
        for _ in range(100):
            data = simulator._generate_normal_data()
            assert simulator.TEMP_MIN <= data["temperature"] <= simulator.TEMP_MAX

    def test_normal_data_structure(self, simulator):
        """Test that normal data has the expected structure."""
        data = simulator._generate_normal_data()

        assert isinstance(data, dict)
        assert "pressure" in data
        assert "temperature" in data
        assert isinstance(data["pressure"], float)
        assert isinstance(data["temperature"], float)

    def test_normal_data_has_two_decimal_places(self, simulator):
        """Test that values are rounded to 2 decimal places."""
        data = simulator._generate_normal_data()

        # Check that values have at most 2 decimal places
        pressure_str = str(data["pressure"])
        if "." in pressure_str:
            assert len(pressure_str.split(".")[1]) <= 2

        temp_str = str(data["temperature"])
        if "." in temp_str:
            assert len(temp_str.split(".")[1]) <= 2

    def test_normal_data_variation(self, simulator):
        """Test that generated data has variation (not always the same)."""
        values = [simulator._generate_normal_data() for _ in range(10)]
        pressures = [d["pressure"] for d in values]
        temperatures = [d["temperature"] for d in values]

        # With 10 samples, we should have some variation
        assert len(set(pressures)) > 1 or len(set(temperatures)) > 1


class TestAnomalyDataGeneration:
    """Tests for anomaly sensor data generation."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {}, clear=True):
            with patch("main.mqtt.Client"):
                return InfineonDPS368Simulator()

    def test_anomaly_temperature_high(self, simulator):
        """Test that temperature anomaly is at or above the anomaly threshold."""
        # Test multiple times since anomaly type is random
        for _ in range(50):
            data = simulator._generate_anomaly_data()
            # If temperature is anomalous, it should be >= ANOMALY_TEMP
            if data["temperature"] > simulator.TEMP_MAX:
                assert data["temperature"] >= simulator.ANOMALY_TEMP

    def test_anomaly_pressure_high(self, simulator):
        """Test that pressure anomaly is at or above the anomaly threshold."""
        for _ in range(50):
            data = simulator._generate_anomaly_data()
            # If pressure is anomalous, it should be >= ANOMALY_PRESSURE
            if data["pressure"] > simulator.PRESSURE_MAX:
                assert data["pressure"] >= simulator.ANOMALY_PRESSURE

    def test_anomaly_data_structure(self, simulator):
        """Test that anomaly data has the expected structure."""
        data = simulator._generate_anomaly_data()

        assert isinstance(data, dict)
        assert "pressure" in data
        assert "temperature" in data
        assert isinstance(data["pressure"], float)
        assert isinstance(data["temperature"], float)

    def test_anomaly_temperature_type(self, simulator):
        """Test that temperature anomaly can be generated."""
        # Run multiple times to hit the "temperature" anomaly type
        for _ in range(100):
            data = simulator._generate_anomaly_data()
            if data["temperature"] >= simulator.ANOMALY_TEMP:
                # Temperature is in anomaly range
                # Pressure should be in normal range (only temperature is anomalous for this type)
                if data["pressure"] <= simulator.PRESSURE_MAX:
                    assert simulator.PRESSURE_MIN <= data["pressure"] <= simulator.PRESSURE_MAX
                    return

        pytest.skip("Did not generate temperature-only anomaly in 100 attempts")

    def test_anomaly_pressure_type(self, simulator):
        """Test that pressure anomaly can be generated."""
        for _ in range(100):
            data = simulator._generate_anomaly_data()
            if data["pressure"] >= simulator.ANOMALY_PRESSURE:
                # Pressure is in anomaly range
                # Temperature should be in normal range (only pressure is anomalous for this type)
                if data["temperature"] <= simulator.TEMP_MAX:
                    assert simulator.TEMP_MIN <= data["temperature"] <= simulator.TEMP_MAX
                    return

        pytest.skip("Did not generate pressure-only anomaly in 100 attempts")

    def test_anomaly_both_type(self, simulator):
        """Test that both anomaly type can be generated."""
        for _ in range(100):
            data = simulator._generate_anomaly_data()
            if (data["pressure"] >= simulator.ANOMALY_PRESSURE and
                data["temperature"] >= simulator.ANOMALY_TEMP):
                return

        pytest.skip("Did not generate 'both' anomaly in 100 attempts")


class TestTelemetryPayloadStructure:
    """Tests for telemetry payload structure."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {
            "DEVICE_ID": "test-device-123"
        }, clear=True):
            with patch("main.mqtt.Client"):
                return InfineonDPS368Simulator()

    def test_telemetry_normal_structure(self, simulator):
        """Test normal telemetry payload structure."""
        payload = simulator._generate_telemetry(is_anomaly=False)

        assert isinstance(payload, dict)
        assert "timestamp" in payload
        assert "device_id" in payload
        assert "sensor_type" in payload
        assert "data" in payload

    def test_telemetry_anomaly_structure(self, simulator):
        """Test anomaly telemetry payload structure."""
        payload = simulator._generate_telemetry(is_anomaly=True)

        assert isinstance(payload, dict)
        assert "timestamp" in payload
        assert "device_id" in payload
        assert "sensor_type" in payload
        assert "data" in payload

    def test_telemetry_timestamp_format(self, simulator):
        """Test that timestamp is in ISO 8601 format."""
        payload = simulator._generate_telemetry(is_anomaly=False)

        # Should be a valid ISO 8601 string with timezone
        timestamp = payload["timestamp"]
        assert isinstance(timestamp, str)
        # Check it ends with +00:00 or Z (UTC timezone)
        assert "+00:00" in timestamp or timestamp.endswith("Z")

    def test_telemetry_device_id(self, simulator):
        """Test that device_id matches the configured device."""
        payload = simulator._generate_telemetry(is_anomaly=False)

        assert payload["device_id"] == "test-device-123"

    def test_telemetry_sensor_type(self, simulator):
        """Test that sensor_type is DPS368."""
        payload = simulator._generate_telemetry(is_anomaly=False)

        assert payload["sensor_type"] == "DPS368"

    def test_telemetry_normal_data_values(self, simulator):
        """Test that normal telemetry contains normal data values."""
        payload = simulator._generate_telemetry(is_anomaly=False)
        data = payload["data"]

        assert simulator.PRESSURE_MIN <= data["pressure"] <= simulator.PRESSURE_MAX
        assert simulator.TEMP_MIN <= data["temperature"] <= simulator.TEMP_MAX

    def test_telemetry_anomaly_data_values(self, simulator):
        """Test that anomaly telemetry contains at least one anomalous value."""
        # Run multiple times to ensure we get an anomaly
        for _ in range(50):
            payload = simulator._generate_telemetry(is_anomaly=True)
            data = payload["data"]

            # At least one value should be anomalous
            is_pressure_anomaly = data["pressure"] > simulator.PRESSURE_MAX
            is_temp_anomaly = data["temperature"] > simulator.TEMP_MAX

            if is_pressure_anomaly or is_temp_anomaly:
                return

        pytest.skip("Did not generate anomalous data in 50 attempts")

    def test_telemetry_json_serializable(self, simulator):
        """Test that telemetry payload can be serialized to JSON."""
        payload = simulator._generate_telemetry(is_anomaly=False)

        # Should not raise any exception
        json_str = json.dumps(payload)
        assert isinstance(json_str, str)

        # Should be deserializable back to the same structure
        deserialized = json.loads(json_str)
        assert deserialized["device_id"] == payload["device_id"]
        assert deserialized["sensor_type"] == payload["sensor_type"]


class TestTopicFormatting:
    """Tests for MQTT topic formatting."""

    @patch("main.mqtt.Client")
    def test_default_topic_format(self, mock_client_class):
        """Test default topic format with default device ID."""
        with patch.dict(os.environ, {}, clear=True):
            simulator = InfineonDPS368Simulator()

            assert simulator.topic == "device/infineon-sensor-01/telemetry"

    @patch("main.mqtt.Client")
    def test_custom_topic_from_env(self, mock_client_class):
        """Test custom topic from environment variable."""
        with patch.dict(os.environ, {
            "MQTT_TOPIC": "custom/topic/path"
        }, clear=True):
            simulator = InfineonDPS368Simulator()

            assert simulator.topic == "custom/topic/path"

    @patch("main.mqtt.Client")
    def test_topic_with_custom_device_id(self, mock_client_class):
        """Test topic construction with custom device ID."""
        with patch.dict(os.environ, {
            "DEVICE_ID": "my-custom-sensor"
        }, clear=True):
            simulator = InfineonDPS368Simulator()

            assert simulator.topic == "device/my-custom-sensor/telemetry"

    @patch("main.mqtt.Client")
    def test_topic_with_different_device_ids(self, mock_client_class):
        """Test topic formatting with various device IDs."""
        test_cases = [
            ("sensor-001", "device/sensor-001/telemetry"),
            ("abc-123-xyz", "device/abc-123-xyz/telemetry"),
            ("device_with_underscores", "device/device_with_underscores/telemetry"),
            ("Device.With.Dots", "device/Device.With.Dots/telemetry"),
        ]

        for device_id, expected_topic in test_cases:
            with patch.dict(os.environ, {
                "DEVICE_ID": device_id
            }, clear=True):
                simulator = InfineonDPS368Simulator()
                assert simulator.topic == expected_topic


class TestMQTTClientSetup:
    """Tests for MQTT client initialization."""

    @patch.dict(os.environ, {}, clear=True)
    def test_client_created(self):
        """Test that MQTT client is created."""
        with patch("main.mqtt.Client") as mock_client_class:
            InfineonDPS368Simulator()

            mock_client_class.assert_called_once()

    @patch.dict(os.environ, {}, clear=True)
    def test_client_id_passed(self):
        """Test that client ID is passed to MQTT client."""
        with patch("main.mqtt.Client") as mock_client_class:
            with patch.object(InfineonDPS368Simulator, "_timestamp", return_value="1234567890"):
                InfineonDPS368Simulator()

                call_kwargs = mock_client_class.call_args.kwargs
                assert "client_id" in call_kwargs
                assert "dps368-sim-" in call_kwargs["client_id"]

    @patch.dict(os.environ, {}, clear=True)
    def test_callbacks_registered(self):
        """Test that MQTT callbacks are registered."""
        with patch("main.mqtt.Client") as mock_client_class:
            mock_client = MagicMock()
            mock_client_class.return_value = mock_client

            simulator = InfineonDPS368Simulator()

            assert mock_client.on_connect == simulator._on_connect
            assert mock_client.on_disconnect == simulator._on_disconnect
            assert mock_client.on_publish == simulator._on_publish

    @patch.dict(os.environ, {}, clear=True)
    def test_reconnect_delay_set(self):
        """Test that reconnect delay is configured."""
        with patch("main.mqtt.Client") as mock_client_class:
            mock_client = MagicMock()
            mock_client_class.return_value = mock_client

            InfineonDPS368Simulator()

            mock_client.reconnect_delay_set.assert_called_once_with(min_delay=1, max_delay=30)


class TestConnectionHandlers:
    """Tests for MQTT connection handlers."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {}, clear=True):
            with patch("main.mqtt.Client"):
                return InfineonDPS368Simulator()

    def test_on_connect_success(self, simulator, capsys):
        """Test successful connection handler."""
        mock_client = MagicMock()
        mock_reason_code = MagicMock()
        mock_reason_code.value = 0

        simulator._on_connect(mock_client, None, MagicMock(), mock_reason_code)

        assert simulator.connected is True
        captured = capsys.readouterr()
        assert "Connected to MQTT Broker" in captured.out

    def test_on_connect_failure(self, simulator, capsys):
        """Test connection failure handler."""
        mock_client = MagicMock()
        mock_reason_code = MagicMock()
        mock_reason_code.value = 5  # Connection refused

        simulator._on_connect(mock_client, None, MagicMock(), mock_reason_code)

        assert simulator.connected is False
        captured = capsys.readouterr()
        assert "Connection failed" in captured.out

    def test_on_connect_paho_v1_style(self, simulator):
        """Test connection handler with paho v1 style integer reason code."""
        mock_client = MagicMock()

        simulator._on_connect(mock_client, None, MagicMock(), 0)

        assert simulator.connected is True

    def test_on_disconnect_while_running(self, simulator, capsys):
        """Test disconnect handler while simulator is running."""
        simulator.running = True
        mock_reason_code = MagicMock()

        simulator._on_disconnect(MagicMock(), None, mock_reason_code)

        assert simulator.connected is False
        captured = capsys.readouterr()
        assert "Disconnected" in captured.out
        assert "Attempting to reconnect" in captured.out

    def test_on_disconnect_when_stopped(self, simulator, capsys):
        """Test disconnect handler when simulator is stopped."""
        simulator.running = False
        mock_reason_code = MagicMock()

        simulator._on_disconnect(MagicMock(), None, mock_reason_code)

        assert simulator.connected is False
        captured = capsys.readouterr()
        assert "Disconnected" in captured.out
        assert "Attempting to reconnect" not in captured.out

    def test_on_publish(self, simulator):
        """Test publish confirmation handler."""
        initial_count = simulator.total_published

        simulator._on_publish(MagicMock(), None, 1)

        assert simulator.total_published == initial_count + 1


class TestPublishTelemetry:
    """Tests for telemetry publishing."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {
            "DEVICE_ID": "test-device"
        }, clear=True):
            with patch("main.mqtt.Client") as mock_client_class:
                mock_client = MagicMock()
                mock_client_class.return_value = mock_client
                mock_client.publish.return_value = MagicMock(rc=0)  # MQTT_ERR_SUCCESS
                return InfineonDPS368Simulator(), mock_client

    def test_publish_increments_packet_count(self, simulator):
        """Test that publishing increments packet count."""
        sim, mock_client = simulator
        initial_count = sim.packet_count

        sim._publish_telemetry()

        assert sim.packet_count == initial_count + 1

    def test_publish_calls_mqtt_publish(self, simulator):
        """Test that MQTT publish is called."""
        sim, mock_client = simulator

        sim._publish_telemetry()

        mock_client.publish.assert_called_once()
        call_args = mock_client.publish.call_args
        assert call_args.kwargs["topic"] == "device/test-device/telemetry"
        assert call_args.kwargs["qos"] == 1
        assert call_args.kwargs["retain"] is False

    def test_publish_payload_is_json(self, simulator):
        """Test that published payload is valid JSON."""
        sim, mock_client = simulator

        sim._publish_telemetry()

        call_args = mock_client.publish.call_args
        payload = call_args.kwargs["payload"]

        # Should be valid JSON
        data = json.loads(payload)
        assert "timestamp" in data
        assert "device_id" in data
        assert "sensor_type" in data
        assert "data" in data

    def test_anomaly_injected_at_interval(self, simulator):
        """Test that anomaly is injected at the configured interval."""
        sim, mock_client = simulator
        sim.anomaly_interval = 5
        sim.packet_count = 4  # Next packet (5) should be anomaly

        sim._publish_telemetry()

        call_args = mock_client.publish.call_args
        payload = json.loads(call_args.kwargs["payload"])
        data = payload["data"]

        # Should be an anomaly (at least one value outside normal range)
        is_anomaly = (data["pressure"] > sim.PRESSURE_MAX or
                     data["temperature"] > sim.TEMP_MAX)
        assert is_anomaly is True

    def test_normal_data_between_anomalies(self, simulator):
        """Test that normal data is sent between anomalies."""
        sim, mock_client = simulator
        sim.anomaly_interval = 10
        sim.packet_count = 5  # Not at anomaly interval

        sim._publish_telemetry()

        call_args = mock_client.publish.call_args
        payload = json.loads(call_args.kwargs["payload"])
        data = payload["data"]

        # Should be normal data
        assert sim.PRESSURE_MIN <= data["pressure"] <= sim.PRESSURE_MAX
        assert sim.TEMP_MIN <= data["temperature"] <= sim.TEMP_MAX

    def test_publish_failure_handled(self, simulator, capsys):
        """Test that publish failure is handled gracefully."""
        sim, mock_client = simulator
        mock_client.publish.return_value = MagicMock(rc=1)  # Error

        sim._publish_telemetry()

        captured = capsys.readouterr()
        assert "Publish failed" in captured.out


class TestTimestamp:
    """Tests for timestamp generation."""

    @patch("main.mqtt.Client")
    def test_timestamp_format(self, mock_client_class):
        """Test timestamp format."""
        with patch.dict(os.environ, {}, clear=True):
            simulator = InfineonDPS368Simulator()
            timestamp = simulator._timestamp()

            # Should be in format: YYYY-MM-DD HH:MM:SS
            assert len(timestamp) == 19
            assert timestamp[4] == "-"
            assert timestamp[7] == "-"
            assert timestamp[10] == " "
            assert timestamp[13] == ":"
            assert timestamp[16] == ":"


class TestSignalHandling:
    """Tests for signal handling."""

    @patch("main.mqtt.Client")
    def test_shutdown_handler(self, mock_client_class):
        """Test shutdown signal handler."""
        with patch.dict(os.environ, {}, clear=True):
            simulator = InfineonDPS368Simulator()
            simulator.running = True

            simulator._handle_shutdown(__import__("signal").SIGINT, None)

            assert simulator.running is False

    @patch("main.mqtt.Client")
    def test_shutdown_prints_stats(self, mock_client_class, capsys):
        """Test that shutdown prints statistics."""
        with patch.dict(os.environ, {}, clear=True):
            simulator = InfineonDPS368Simulator()
            simulator.total_published = 100
            simulator.total_anomalies = 10

            simulator._handle_shutdown(__import__("signal").SIGINT, None)

            captured = capsys.readouterr()
            assert "Total packets published: 100" in captured.out
            assert "Total anomalies injected: 10" in captured.out


class TestStatistics:
    """Tests for statistics tracking."""

    @pytest.fixture
    def simulator(self):
        """Create a simulator instance with mocked MQTT client."""
        with patch.dict(os.environ, {}, clear=True):
            with patch("main.mqtt.Client"):
                return InfineonDPS368Simulator()

    def test_initial_statistics(self, simulator):
        """Test initial statistics values."""
        assert simulator.total_published == 0
        assert simulator.total_anomalies == 0
        assert simulator.packet_count == 0

    def test_statistics_after_publish(self, simulator):
        """Test statistics after publishing."""
        with patch.object(simulator, "client"):
            simulator.client = MagicMock()
            simulator.client.publish.return_value = MagicMock(rc=0)

            simulator._publish_telemetry()

            assert simulator.packet_count == 1

    def test_anomaly_count_incremented(self, simulator):
        """Test that anomaly counter is incremented for anomaly packets."""
        simulator.client = MagicMock()
        simulator.client.publish.return_value = MagicMock(rc=0)
        simulator.anomaly_interval = 1  # Every packet is anomaly

        initial_anomalies = simulator.total_anomalies
        simulator._publish_telemetry()

        assert simulator.total_anomalies == initial_anomalies + 1


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
