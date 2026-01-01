"""
Basic Usage Example for TESAIoT Analytics API

This example demonstrates:
- Client initialization
- Making API requests
- Handling responses
- Error handling

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
from datetime import datetime, timedelta
from analytics_client import AnalyticsClient, AnalyticsClientSync, TimeRange


async def async_example():
    """Async example using context manager"""
    print("=" * 60)
    print("TESAIoT Analytics API - Basic Example (Async)")
    print("=" * 60)

    # Initialize client (uses env vars or explicit values)
    async with AnalyticsClient(
        base_url=os.getenv("TESAIOT_API_URL", "https://admin.tesaiot.com/api/v1/bdh-ai"),
        api_token=os.getenv("TESAIOT_API_TOKEN")
    ) as client:

        # 1. Get recent anomalies
        print("\n1. Fetching recent anomalies...")
        try:
            anomalies = await client.get_anomalies(
                limit=5,
                severity_filter=["critical", "high"]
            )
            print(f"   Found {anomalies.get('summary', {}).get('total', 0)} anomalies")

            for a in anomalies.get("anomalies", [])[:3]:
                print(f"   - {a['device_name']}: {a['metric']} ({a['severity']})")
        except Exception as e:
            print(f"   Error: {e}")

        # 2. Get device clusters
        print("\n2. Fetching device clusters...")
        try:
            clusters = await client.get_clusters(
                metric_name="temperature",
                n_clusters=3
            )
            print(f"   Found {len(clusters.get('clusters', []))} clusters")
            print(f"   Silhouette score: {clusters.get('silhouette_score', 'N/A')}")
        except Exception as e:
            print(f"   Error: {e}")

        # 3. Get AI insights
        print("\n3. Fetching AI insights...")
        try:
            insights = await client.get_insights(days=7)
            print(f"   Generated {len(insights.get('insights', []))} insights")

            for i in insights.get("insights", [])[:2]:
                print(f"   - [{i['severity'].upper()}] {i['title']}")
        except Exception as e:
            print(f"   Error: {e}")

        # 4. Get connectivity status
        print("\n4. Fetching connectivity status...")
        try:
            status = await client.get_connectivity_status()
            summary = status.get("summary", {})
            print(f"   Total devices: {summary.get('total', 0)}")
            print(f"   Online: {summary.get('online', 0)}")
            print(f"   Offline: {summary.get('offline', 0)}")
        except Exception as e:
            print(f"   Error: {e}")

    print("\n" + "=" * 60)
    print("Done!")


def sync_example():
    """Synchronous example for simpler use cases"""
    print("=" * 60)
    print("TESAIoT Analytics API - Basic Example (Sync)")
    print("=" * 60)

    # Initialize sync client
    client = AnalyticsClientSync(
        base_url=os.getenv("TESAIOT_API_URL", "https://admin.tesaiot.com/api/v1/bdh-ai"),
        api_token=os.getenv("TESAIOT_API_TOKEN")
    )

    try:
        # Get anomalies
        print("\nFetching anomalies...")
        anomalies = client.get_anomalies(limit=5)
        print(f"Found {anomalies.get('summary', {}).get('total', 0)} anomalies")

        # Get connectivity
        print("\nFetching connectivity...")
        status = client.get_connectivity_status()
        print(f"Online devices: {status.get('summary', {}).get('online', 0)}")

    finally:
        client.close()

    print("\nDone!")


def example_with_time_range():
    """Example showing time range handling"""
    print("=" * 60)
    print("Time Range Examples")
    print("=" * 60)

    # Using TimeRange helper
    last_7_days = TimeRange.last_days(7)
    print(f"\nLast 7 days:")
    print(f"  Start: {last_7_days.start}")
    print(f"  End: {last_7_days.end}")

    last_30_days = TimeRange.last_days(30)
    print(f"\nLast 30 days:")
    print(f"  Start: {last_30_days.start}")
    print(f"  End: {last_30_days.end}")

    # Custom time range
    custom_start = datetime(2025, 12, 1, 0, 0, 0)
    custom_end = datetime(2025, 12, 29, 23, 59, 59)
    print(f"\nCustom range:")
    print(f"  Start: {custom_start.isoformat()}Z")
    print(f"  End: {custom_end.isoformat()}Z")


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1 and sys.argv[1] == "--sync":
        sync_example()
    elif len(sys.argv) > 1 and sys.argv[1] == "--time":
        example_with_time_range()
    else:
        # Default: run async example
        asyncio.run(async_example())
