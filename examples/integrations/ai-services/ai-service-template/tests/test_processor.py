# Copyright (c) 2025 TESAIoT Platform (TESA)
# Licensed under Apache License 2.0

"""Unit tests for AI processor."""

import pytest
from unittest.mock import patch, MagicMock
from datetime import datetime


class TestAIProcessor:
    """Test AI inference processor."""

    @patch("core.processor.settings")
    def test_initialization_without_model(self, mock_settings):
        """Processor should initialize without model path."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        assert processor._model is None
        assert processor.model_version == "1.0.0"
        assert processor.initialized_at is not None

    @patch("core.processor.settings")
    def test_initialization_with_model_path(self, mock_settings):
        """Processor should attempt to load model from path."""
        mock_settings.model_path = "/models/test.pkl"

        from core.processor import AIProcessor

        processor = AIProcessor(model_path="/models/test.pkl")
        assert processor.model_path == "/models/test.pkl"

    @patch("core.processor.settings")
    def test_process_empty_data_error(self, mock_settings):
        """Empty input should raise ValueError."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        with pytest.raises(ValueError) as exc_info:
            processor.process({})
        assert "cannot be empty" in str(exc_info.value)

    @patch("core.processor.settings")
    def test_process_returns_required_fields(self, mock_settings):
        """Result should contain prediction, confidence, version, timestamp."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 25.0})

        assert "prediction" in result
        assert "confidence" in result
        assert "model_version" in result
        assert "timestamp" in result

    @patch("core.processor.settings")
    def test_rule_based_high_temperature(self, mock_settings):
        """High temperature (>40) should predict 'high'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 45.0})

        assert result["prediction"] == "high"
        assert result["confidence"] == 0.95

    @patch("core.processor.settings")
    def test_rule_based_warning_temperature(self, mock_settings):
        """Warning temperature (35-40) should predict 'warning'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 37.0})

        assert result["prediction"] == "warning"
        assert result["confidence"] == 0.85

    @patch("core.processor.settings")
    def test_rule_based_low_temperature(self, mock_settings):
        """Low temperature (5-10) should predict 'low'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 7.0})

        assert result["prediction"] == "low"
        assert result["confidence"] == 0.90

    @patch("core.processor.settings")
    def test_rule_based_critical_low_temperature(self, mock_settings):
        """Critical low temperature (<5) should predict 'critical_low'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 2.0})

        assert result["prediction"] == "critical_low"
        assert result["confidence"] == 0.95

    @patch("core.processor.settings")
    def test_rule_based_normal_temperature(self, mock_settings):
        """Normal temperature (10-35) should predict 'normal'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 25.0})

        assert result["prediction"] == "normal"
        assert result["confidence"] == 0.90

    @patch("core.processor.settings")
    def test_rule_based_no_temperature(self, mock_settings):
        """Missing temperature should predict 'unknown'."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"humidity": 60.0})

        assert result["prediction"] == "unknown"
        assert result["confidence"] == 0.5

    @patch("core.processor.settings")
    def test_extract_features(self, mock_settings):
        """Feature extraction should return list of floats."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        features = processor._extract_features({
            "temperature": 25.5,
            "humidity": 60.0,
            "pressure": 1013.25
        })

        assert isinstance(features, list)
        assert len(features) == 3
        assert features[0] == 25.5
        assert features[1] == 60.0
        assert features[2] == 1013.25

    @patch("core.processor.settings")
    def test_extract_features_partial(self, mock_settings):
        """Partial data should extract available features."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        features = processor._extract_features({
            "temperature": 30.0
        })

        assert len(features) == 1
        assert features[0] == 30.0

    @patch("core.processor.settings")
    def test_timestamp_format(self, mock_settings):
        """Timestamp should be ISO format."""
        mock_settings.model_path = None

        from core.processor import AIProcessor

        processor = AIProcessor()
        result = processor.process({"temperature": 25.0})

        # Should be parseable as ISO timestamp
        timestamp = result["timestamp"]
        parsed = datetime.fromisoformat(timestamp)
        assert parsed is not None


class TestModelIntegration:
    """Test model loading integration (mocked)."""

    @patch("core.processor.settings")
    @patch("core.processor.logger")
    def test_model_loading_logs(self, mock_logger, mock_settings):
        """Model loading should log info messages."""
        mock_settings.model_path = "/models/test.pkl"

        from core.processor import AIProcessor

        processor = AIProcessor(model_path="/models/test.pkl")

        # Check that logging was called
        mock_logger.info.assert_called()
