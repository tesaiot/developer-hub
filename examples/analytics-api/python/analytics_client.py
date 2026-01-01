"""
TESAIoT BDH AI Analytics API Client

A Python client library for accessing the TESAIoT Analytics API.

Usage:
    from analytics_client import AnalyticsClient

    client = AnalyticsClient(
        base_url="https://admin.tesaiot.com/api/v1/bdh-ai",
        api_token="your_jwt_token"
    )

    # Get anomalies
    anomalies = await client.get_anomalies()

Author: TESAIoT Team
License: Apache 2.0
"""

import os
import asyncio
from datetime import datetime, timedelta
from typing import Optional, List, Dict, Any
from dataclasses import dataclass, field
import httpx
from pydantic import BaseModel


# ============================================================
# Data Models
# ============================================================

@dataclass
class TimeRange:
    """Time range for queries"""
    start: str
    end: str

    @classmethod
    def last_days(cls, days: int) -> "TimeRange":
        """Create time range for last N days"""
        end = datetime.utcnow()
        start = end - timedelta(days=days)
        return cls(
            start=start.isoformat() + "Z",
            end=end.isoformat() + "Z"
        )


@dataclass
class Anomaly:
    """Anomaly data"""
    id: str
    device_id: str
    device_name: str
    metric: str
    value: float
    severity: str
    score: float
    timestamp: str
    acknowledged: bool = False
    resolved: bool = False


@dataclass
class Cluster:
    """Device cluster data"""
    cluster_id: int
    cluster_name: str
    device_count: int
    characteristics: Dict[str, Any]
    devices: List[str]


@dataclass
class Insight:
    """Generated insight"""
    id: str
    type: str
    severity: str
    title: str
    description: str
    confidence: float
    actionable: bool
    recommended_actions: List[str] = field(default_factory=list)


@dataclass
class DeviceStatus:
    """Device connectivity status"""
    device_id: str
    device_name: str
    status: str  # online/offline
    last_seen: str
    uptime_percent: float


# ============================================================
# Analytics Client
# ============================================================

class AnalyticsClient:
    """
    TESAIoT BDH AI Analytics API Client

    Example:
        client = AnalyticsClient(
            base_url="https://admin.tesaiot.com/api/v1/bdh-ai",
            api_token="your_token"
        )

        anomalies = await client.get_anomalies()
    """

    def __init__(
        self,
        base_url: Optional[str] = None,
        api_token: Optional[str] = None,
        timeout: float = 30.0
    ):
        """
        Initialize the Analytics client.

        Args:
            base_url: API base URL (default: from TESAIOT_API_URL env)
            api_token: JWT authentication token (default: from TESAIOT_API_TOKEN env)
            timeout: Request timeout in seconds
        """
        self.base_url = base_url or os.getenv(
            "TESAIOT_API_URL",
            "https://admin.tesaiot.com/api/v1/bdh-ai"
        )
        self.api_token = api_token or os.getenv("TESAIOT_API_TOKEN")
        self.timeout = timeout

        if not self.api_token:
            raise ValueError("API token required. Set TESAIOT_API_TOKEN or pass api_token")

        self._client = httpx.AsyncClient(
            base_url=self.base_url,
            headers={
                "X-API-KEY": self.api_token,
                "Content-Type": "application/json"
            },
            timeout=timeout,
            verify=False  # Skip TLS verification for testing
        )

    async def close(self):
        """Close the HTTP client"""
        await self._client.aclose()

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()

    # --------------------------------------------------------
    # Anomaly Detection APIs
    # --------------------------------------------------------

    async def get_anomalies(
        self,
        start_time: Optional[str] = None,
        end_time: Optional[str] = None,
        severity_filter: Optional[List[str]] = None,
        device_ids: Optional[List[str]] = None,
        limit: int = 100,
        offset: int = 0
    ) -> Dict[str, Any]:
        """
        Get aggregated anomalies.

        Args:
            start_time: Start of time range (ISO 8601)
            end_time: End of time range (ISO 8601)
            severity_filter: Filter by severity levels
            device_ids: Filter by specific devices
            limit: Maximum results to return
            offset: Pagination offset

        Returns:
            Dict with anomalies data and summary
        """
        # Default to last 7 days
        if not start_time or not end_time:
            time_range = TimeRange.last_days(7)
            start_time = start_time or time_range.start
            end_time = end_time or time_range.end

        payload = {
            "time_range": {"start": start_time, "end": end_time},
            "limit": limit,
            "offset": offset
        }

        if severity_filter:
            payload["severity_filter"] = severity_filter
        if device_ids:
            payload["device_ids"] = device_ids

        response = await self._client.get("/analytics/anomalies", params=payload)
        response.raise_for_status()
        return response.json()

    async def get_anomaly_timeline(
        self,
        days: int = 30,
        group_by: str = "severity"
    ) -> Dict[str, Any]:
        """
        Get anomaly timeline for trend visualization.

        Args:
            days: Number of days to include
            group_by: Grouping field (severity, device, metric)

        Returns:
            Dict with timeline data and trend info
        """
        payload = {"days": days, "group_by": group_by}
        response = await self._client.post("/analytics/anomalies/timeline", json=payload)
        response.raise_for_status()
        return response.json()

    async def get_anomaly_heatmap(
        self,
        start_time: str,
        end_time: str,
        resolution: str = "hour",
        max_devices: int = 50
    ) -> Dict[str, Any]:
        """
        Get device × time heatmap data.

        Args:
            start_time: Start of time range
            end_time: End of time range
            resolution: Time resolution (hour, day)
            max_devices: Maximum devices to include

        Returns:
            Dict with heatmap matrix data
        """
        payload = {
            "time_range": {"start": start_time, "end": end_time},
            "resolution": resolution,
            "max_devices": max_devices
        }
        response = await self._client.post("/analytics/anomalies/heatmap", json=payload)
        response.raise_for_status()
        return response.json()

    async def acknowledge_anomaly(
        self,
        anomaly_id: str,
        notes: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Mark an anomaly as acknowledged.

        Args:
            anomaly_id: The anomaly ID
            notes: Optional notes

        Returns:
            Updated anomaly data
        """
        payload = {"acknowledged": True}
        if notes:
            payload["notes"] = notes

        response = await self._client.put(
            f"/analytics/anomalies/{anomaly_id}/acknowledge",
            json=payload
        )
        response.raise_for_status()
        return response.json()

    async def resolve_anomaly(
        self,
        anomaly_id: str,
        resolution: str = "resolved",
        notes: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Mark an anomaly as resolved.

        Args:
            anomaly_id: The anomaly ID
            resolution: Resolution type (resolved, false_positive, deferred)
            notes: Optional notes

        Returns:
            Updated anomaly data
        """
        payload = {"resolved": True, "resolution": resolution}
        if notes:
            payload["notes"] = notes

        response = await self._client.put(
            f"/analytics/anomalies/{anomaly_id}/resolve",
            json=payload
        )
        response.raise_for_status()
        return response.json()

    # --------------------------------------------------------
    # Pattern Recognition APIs
    # --------------------------------------------------------

    async def get_clusters(
        self,
        metric_name: str,
        n_clusters: int = 5,
        start_time: Optional[str] = None,
        end_time: Optional[str] = None,
        include_outliers: bool = True
    ) -> Dict[str, Any]:
        """
        Get K-means clustering of device behavior patterns.

        Args:
            metric_name: Metric to cluster (e.g., temperature)
            n_clusters: Number of clusters
            start_time: Start of analysis period
            end_time: End of analysis period
            include_outliers: Include outlier detection

        Returns:
            Dict with clusters, silhouette score, and outliers
        """
        if not start_time or not end_time:
            time_range = TimeRange.last_days(7)
            start_time = start_time or time_range.start
            end_time = end_time or time_range.end

        payload = {
            "metric_name": metric_name,
            "n_clusters": n_clusters,
            "time_range": {"start": start_time, "end": end_time},
            "include_outliers": include_outliers
        }

        response = await self._client.post("/patterns/clusters", json=payload)
        response.raise_for_status()
        return response.json()

    async def get_outliers(
        self,
        metric_name: str,
        threshold: float = 0.7
    ) -> Dict[str, Any]:
        """
        Get outlier devices that don't fit patterns.

        Args:
            metric_name: Metric to analyze
            threshold: Outlier score threshold

        Returns:
            Dict with outlier devices
        """
        payload = {"metric_name": metric_name, "threshold": threshold}
        response = await self._client.post("/patterns/outliers", json=payload)
        response.raise_for_status()
        return response.json()

    async def find_similar_devices(
        self,
        device_id: str,
        metric_name: str,
        top_k: int = 10
    ) -> Dict[str, Any]:
        """
        Find devices with similar behavior patterns.

        Args:
            device_id: Source device ID
            metric_name: Metric to compare
            top_k: Number of similar devices to return

        Returns:
            Dict with source device features and similar devices
        """
        payload = {
            "device_id": device_id,
            "metric_name": metric_name,
            "top_k": top_k
        }
        response = await self._client.post("/patterns/similar", json=payload)
        response.raise_for_status()
        return response.json()

    # --------------------------------------------------------
    # Insights APIs
    # --------------------------------------------------------

    async def get_insights(
        self,
        days: int = 7,
        insight_types: Optional[List[str]] = None,
        min_confidence: float = 0.7
    ) -> Dict[str, Any]:
        """
        Generate AI-powered insights.

        Args:
            days: Analysis period in days
            insight_types: Filter by type (anomaly_pattern, trend, correlation, recommendation)
            min_confidence: Minimum confidence score

        Returns:
            Dict with fleet summary, insights, and recommendations
        """
        payload = {
            "analysis_period_days": days,
            "min_confidence": min_confidence
        }

        if insight_types:
            payload["insight_types"] = insight_types

        response = await self._client.post("/insights", json=payload)
        response.raise_for_status()
        return response.json()

    async def get_recommendations(
        self,
        priority: Optional[str] = None,
        limit: int = 10
    ) -> Dict[str, Any]:
        """
        Get actionable recommendations.

        Args:
            priority: Filter by priority (high, medium, low)
            limit: Maximum recommendations

        Returns:
            Dict with recommendations list
        """
        params = {"limit": limit}
        if priority:
            params["priority"] = priority

        response = await self._client.get("/insights/recommendations", params=params)
        response.raise_for_status()
        return response.json()

    # --------------------------------------------------------
    # Connectivity APIs
    # --------------------------------------------------------

    async def get_connectivity_status(
        self,
        status_filter: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Get real-time device connectivity status.

        Args:
            status_filter: Filter by status (online, offline)

        Returns:
            Dict with device status list and summary
        """
        params = {}
        if status_filter:
            params["status"] = status_filter

        response = await self._client.get("/connectivity/status", params=params)
        response.raise_for_status()
        return response.json()

    async def get_latency_stats(
        self,
        hours: int = 24
    ) -> Dict[str, Any]:
        """
        Get network latency statistics.

        Args:
            hours: Time range in hours

        Returns:
            Dict with latency statistics and heatmap
        """
        params = {"hours": hours}
        response = await self._client.get("/connectivity/latency", params=params)
        response.raise_for_status()
        return response.json()

    async def get_throughput_stats(
        self,
        hours: int = 24
    ) -> Dict[str, Any]:
        """
        Get message throughput statistics.

        Args:
            hours: Time range in hours

        Returns:
            Dict with throughput timeline and by-topic breakdown
        """
        params = {"hours": hours}
        response = await self._client.get("/connectivity/throughput", params=params)
        response.raise_for_status()
        return response.json()

    async def get_connection_quality(self) -> Dict[str, Any]:
        """
        Get connection quality scores for all devices.

        Returns:
            Dict with quality scores and issues
        """
        response = await self._client.get("/connectivity/quality")
        response.raise_for_status()
        return response.json()

    # --------------------------------------------------------
    # Fleet Analytics APIs (NEW - 2025-12-30)
    # --------------------------------------------------------

    async def get_fleet_anomalies(
        self,
        days: int = 7,
        severity: Optional[str] = None,
        limit: int = 100
    ) -> Dict[str, Any]:
        """
        Get fleet-wide anomaly overview.

        Args:
            days: Number of days to analyze (default: 7)
            severity: Filter by severity (critical, high, medium, low)
            limit: Maximum number of anomalies to return

        Returns:
            Dict with anomalies, summary, and groupings
        """
        params = {"days": days, "limit": limit}
        if severity:
            params["severity"] = severity

        response = await self._client.get("/analytics/anomalies", params=params)
        response.raise_for_status()
        return response.json()


# ============================================================
# Synchronous Wrapper
# ============================================================

class AnalyticsClientSync:
    """
    Synchronous wrapper for AnalyticsClient.

    Use this if you don't need async/await.
    """

    def __init__(self, *args, **kwargs):
        self._async_client = AnalyticsClient(*args, **kwargs)

    def _run(self, coro):
        return asyncio.get_event_loop().run_until_complete(coro)

    def get_anomalies(self, **kwargs):
        return self._run(self._async_client.get_anomalies(**kwargs))

    def get_anomaly_timeline(self, **kwargs):
        return self._run(self._async_client.get_anomaly_timeline(**kwargs))

    def get_clusters(self, **kwargs):
        return self._run(self._async_client.get_clusters(**kwargs))

    def get_insights(self, **kwargs):
        return self._run(self._async_client.get_insights(**kwargs))

    def get_connectivity_status(self, **kwargs):
        return self._run(self._async_client.get_connectivity_status(**kwargs))

    def get_latency_stats(self, **kwargs):
        return self._run(self._async_client.get_latency_stats(**kwargs))

    def get_throughput_stats(self, **kwargs):
        return self._run(self._async_client.get_throughput_stats(**kwargs))

    def get_fleet_anomalies(self, **kwargs):
        return self._run(self._async_client.get_fleet_anomalies(**kwargs))

    def close(self):
        self._run(self._async_client.close())


# ============================================================
# CLI Usage
# ============================================================

if __name__ == "__main__":
    import json

    async def main():
        async with AnalyticsClient() as client:
            # Get anomalies
            print("Fetching anomalies...")
            anomalies = await client.get_anomalies(limit=5)
            print(json.dumps(anomalies, indent=2))

    asyncio.run(main())
