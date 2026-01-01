# Copyright (c) 2025 TESAIoT Platform (TESA)
# Licensed under Apache License 2.0

"""Unit tests for python-cli application."""

import os
import pytest
from datetime import datetime, timedelta, timezone
from unittest.mock import patch, MagicMock


class TestDeriveRealtimeWsUrl:
    """Test WebSocket URL derivation from HTTP base URL."""

    def test_https_to_wss(self):
        """HTTPS base URL should become WSS."""
        from app import _derive_realtime_ws_url
        result = _derive_realtime_ws_url("https://admin.tesaiot.com/api/v1")
        assert result == "wss://admin.tesaiot.com/ws/telemetry"

    def test_http_to_ws(self):
        """HTTP base URL should become WS."""
        from app import _derive_realtime_ws_url
        result = _derive_realtime_ws_url("http://localhost:8080/api/v1")
        assert result == "ws://localhost:8080/ws/telemetry"


class TestTimeParser:
    """Test ISO8601 timestamp parsing."""

    def test_parse_iso8601_with_z(self):
        """Parse timestamp with Z suffix."""
        from app import _parse_iso8601
        result = _parse_iso8601("2025-12-11T10:30:00Z")
        assert result.year == 2025
        assert result.month == 12
        assert result.day == 11
        assert result.hour == 10
        assert result.minute == 30
        assert result.tzinfo == timezone.utc

    def test_parse_iso8601_with_offset(self):
        """Parse timestamp with timezone offset."""
        from app import _parse_iso8601
        result = _parse_iso8601("2025-12-11T17:30:00+07:00")
        # Should be converted to UTC
        assert result.tzinfo == timezone.utc
        assert result.hour == 10  # 17:30 +07:00 = 10:30 UTC

    def test_parse_iso8601_invalid(self):
        """Invalid timestamp should raise TESAIoTClientError."""
        from app import _parse_iso8601, TESAIoTClientError
        with pytest.raises(TESAIoTClientError) as exc_info:
            _parse_iso8601("not-a-date")
        assert "Invalid ISO 8601" in str(exc_info.value)


class TestRelativeOffset:
    """Test relative duration parsing."""

    def test_parse_minutes(self):
        """Parse minute duration."""
        from app import _parse_relative_offset
        result = _parse_relative_offset("15m")
        assert result == timedelta(minutes=15)

    def test_parse_hours(self):
        """Parse hour duration."""
        from app import _parse_relative_offset
        result = _parse_relative_offset("2h")
        assert result == timedelta(hours=2)

    def test_parse_days(self):
        """Parse day duration."""
        from app import _parse_relative_offset
        result = _parse_relative_offset("1d")
        assert result == timedelta(days=1)

    def test_parse_seconds(self):
        """Parse second duration."""
        from app import _parse_relative_offset
        result = _parse_relative_offset("30s")
        assert result == timedelta(seconds=30)

    def test_parse_invalid(self):
        """Invalid duration should raise error."""
        from app import _parse_relative_offset, TESAIoTClientError
        with pytest.raises(TESAIoTClientError):
            _parse_relative_offset("invalid")


class TestResolveTimeBounds:
    """Test time boundary resolution."""

    def test_since_overrides_start_end(self):
        """Using --since should compute relative window."""
        from app import _resolve_time_bounds
        start, end = _resolve_time_bounds(None, None, "1h")
        # Both should be ISO strings
        assert start is not None
        assert end is not None
        assert "T" in start
        assert "Z" in start

    def test_explicit_start_end(self):
        """Explicit start/end should be preserved."""
        from app import _resolve_time_bounds
        start, end = _resolve_time_bounds(
            "2025-12-11T00:00:00Z",
            "2025-12-11T23:59:59Z",
            None
        )
        assert start == "2025-12-11T00:00:00Z"
        assert end == "2025-12-11T23:59:59Z"

    def test_start_after_end_error(self):
        """Start after end should raise error."""
        from app import _resolve_time_bounds, TESAIoTClientError
        with pytest.raises(TESAIoTClientError) as exc_info:
            _resolve_time_bounds(
                "2025-12-11T23:59:59Z",
                "2025-12-11T00:00:00Z",
                None
            )
        assert "earlier than end" in str(exc_info.value)


class TestTESAIoTClient:
    """Test API client URL building."""

    @patch.dict(os.environ, {"TESAIOT_API_KEY": "test-key"})
    @patch("app.Path.exists", return_value=True)
    @patch("builtins.open", create=True)
    @patch("yaml.safe_load")
    def test_build_url_with_leading_slash(self, mock_yaml, mock_open, mock_exists):
        """Path with leading slash should work."""
        mock_yaml.return_value = {
            "api_gateway": {"base_url": "https://admin.tesaiot.com/api/v1"}
        }
        from app import Settings, TESAIoTClient
        settings = Settings(
            base_url="https://admin.tesaiot.com/api/v1",
            timeout=15,
            verify_tls=True,
            api_key="test-key",
            default_schema="test",
            realtime_ws_url="wss://admin.tesaiot.com/ws/telemetry",
            realtime_subscribe_timeout=5
        )
        client = TESAIoTClient(settings)
        url = client._build_url("/devices")
        assert url == "https://admin.tesaiot.com/api/v1/devices"

    @patch.dict(os.environ, {"TESAIOT_API_KEY": "test-key"})
    def test_build_url_without_leading_slash(self):
        """Path without leading slash should add one."""
        from app import Settings, TESAIoTClient
        settings = Settings(
            base_url="https://admin.tesaiot.com/api/v1",
            timeout=15,
            verify_tls=True,
            api_key="test-key",
            default_schema="test",
            realtime_ws_url="wss://admin.tesaiot.com/ws/telemetry",
            realtime_subscribe_timeout=5
        )
        client = TESAIoTClient(settings)
        url = client._build_url("devices")
        assert url == "https://admin.tesaiot.com/api/v1/devices"


class TestSettings:
    """Test Settings dataclass."""

    def test_settings_creation(self):
        """Settings should store all values."""
        from app import Settings
        settings = Settings(
            base_url="https://test.com",
            timeout=30,
            verify_tls=False,
            api_key="my-key",
            default_schema="industrial",
            realtime_ws_url="wss://test.com/ws",
            realtime_subscribe_timeout=10,
            organization_id="org-123",
            user_email="user@test.com"
        )
        assert settings.base_url == "https://test.com"
        assert settings.timeout == 30
        assert settings.verify_tls is False
        assert settings.api_key == "my-key"
        assert settings.organization_id == "org-123"
        assert settings.user_email == "user@test.com"

    def test_settings_optional_fields(self):
        """Optional fields should default to None."""
        from app import Settings
        settings = Settings(
            base_url="https://test.com",
            timeout=15,
            verify_tls=True,
            api_key="key",
            default_schema="test",
            realtime_ws_url="wss://test.com/ws",
            realtime_subscribe_timeout=5
        )
        assert settings.organization_id is None
        assert settings.user_email is None
