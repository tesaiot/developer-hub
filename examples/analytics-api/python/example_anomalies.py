"""
Anomaly Detection Examples for TESAIoT Analytics API

This example demonstrates:
- Querying aggregated anomalies
- Filtering by severity, device, time range
- Anomaly timeline visualization
- Heatmap data retrieval
- Acknowledging and resolving anomalies

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
import json
from datetime import datetime, timedelta
from analytics_client import AnalyticsClient, TimeRange


async def get_critical_anomalies():
    """Get only critical and high severity anomalies"""
    print("\n" + "=" * 60)
    print("Critical & High Severity Anomalies")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_anomalies(
            severity_filter=["critical", "high"],
            limit=10
        )

        summary = result.get("summary", {})
        print(f"\nSummary:")
        print(f"  Total: {summary.get('total', 0)}")
        print(f"  Critical: {summary.get('by_severity', {}).get('critical', 0)}")
        print(f"  High: {summary.get('by_severity', {}).get('high', 0)}")

        print(f"\nRecent Critical Anomalies:")
        for anomaly in result.get("anomalies", []):
            print(f"  [{anomaly['severity'].upper()}] {anomaly['device_name']}")
            print(f"    Metric: {anomaly['metric']}")
            print(f"    Value: {anomaly['value']} (score: {anomaly['score']:.2f})")
            print(f"    Time: {anomaly['timestamp']}")
            print()


async def get_anomalies_for_devices():
    """Get anomalies for specific devices"""
    print("\n" + "=" * 60)
    print("Anomalies for Specific Devices")
    print("=" * 60)

    # Example device IDs (replace with actual device IDs)
    device_ids = [
        "device-001",
        "device-002",
        "device-003"
    ]

    async with AnalyticsClient() as client:
        result = await client.get_anomalies(
            device_ids=device_ids,
            limit=20
        )

        print(f"\nFound {result.get('summary', {}).get('total', 0)} anomalies")
        print(f"for devices: {', '.join(device_ids)}")

        # Group by device
        by_device = {}
        for anomaly in result.get("anomalies", []):
            device = anomaly.get("device_name", "Unknown")
            if device not in by_device:
                by_device[device] = []
            by_device[device].append(anomaly)

        for device, anomalies in by_device.items():
            print(f"\n{device}: {len(anomalies)} anomalies")


async def get_anomaly_timeline():
    """Get anomaly timeline for trend visualization"""
    print("\n" + "=" * 60)
    print("30-Day Anomaly Timeline")
    print("=" * 60)

    async with AnalyticsClient() as client:
        # Get timeline grouped by severity
        result = await client.get_anomaly_timeline(
            days=30,
            group_by="severity"
        )

        print(f"\nTimeline data points: {len(result.get('timeline', []))}")
        print(f"Trend: {result.get('trend', {}).get('direction', 'N/A')}")
        print(f"Change: {result.get('trend', {}).get('percent_change', 0):.1f}%")

        # Show recent days
        print("\nRecent 7 days:")
        for point in result.get("timeline", [])[-7:]:
            date = point.get("date", "")
            counts = point.get("counts", {})
            total = sum(counts.values())
            print(f"  {date}: {total} anomalies", end="")
            if counts.get("critical", 0) > 0:
                print(f" (critical: {counts['critical']})", end="")
            print()


async def get_anomaly_heatmap():
    """Get device × time heatmap data"""
    print("\n" + "=" * 60)
    print("Anomaly Heatmap (Device × Time)")
    print("=" * 60)

    time_range = TimeRange.last_days(7)

    async with AnalyticsClient() as client:
        result = await client.get_anomaly_heatmap(
            start_time=time_range.start,
            end_time=time_range.end,
            resolution="day",
            max_devices=10
        )

        devices = result.get("devices", [])
        time_buckets = result.get("time_buckets", [])
        matrix = result.get("matrix", [])

        print(f"\nDevices: {len(devices)}")
        print(f"Time buckets: {len(time_buckets)}")

        # Show top devices by anomaly count
        device_totals = []
        for i, device in enumerate(devices):
            total = sum(matrix[i]) if i < len(matrix) else 0
            device_totals.append((device, total))

        device_totals.sort(key=lambda x: x[1], reverse=True)

        print("\nTop devices by anomaly count:")
        for device, count in device_totals[:5]:
            print(f"  {device}: {count} anomalies")


async def acknowledge_anomaly_example():
    """Example of acknowledging an anomaly"""
    print("\n" + "=" * 60)
    print("Acknowledge Anomaly Example")
    print("=" * 60)

    async with AnalyticsClient() as client:
        # First, get an unacknowledged anomaly
        result = await client.get_anomalies(
            severity_filter=["critical", "high"],
            limit=1
        )

        anomalies = result.get("anomalies", [])
        if not anomalies:
            print("No anomalies to acknowledge")
            return

        anomaly = anomalies[0]
        anomaly_id = anomaly.get("id")

        print(f"\nAnomaly to acknowledge:")
        print(f"  ID: {anomaly_id}")
        print(f"  Device: {anomaly['device_name']}")
        print(f"  Severity: {anomaly['severity']}")

        # Acknowledge it (commented out to prevent accidental changes)
        # result = await client.acknowledge_anomaly(
        #     anomaly_id=anomaly_id,
        #     notes="Reviewed by monitoring team"
        # )
        # print(f"\nAcknowledged: {result.get('acknowledged', False)}")

        print("\n(Acknowledge call commented out to prevent accidental changes)")


async def resolve_anomaly_example():
    """Example of resolving an anomaly"""
    print("\n" + "=" * 60)
    print("Resolve Anomaly Example")
    print("=" * 60)

    async with AnalyticsClient() as client:
        # Get an acknowledged but unresolved anomaly
        result = await client.get_anomalies(limit=1)

        anomalies = result.get("anomalies", [])
        if not anomalies:
            print("No anomalies to resolve")
            return

        anomaly = anomalies[0]
        anomaly_id = anomaly.get("id")

        print(f"\nAnomaly to resolve:")
        print(f"  ID: {anomaly_id}")
        print(f"  Device: {anomaly['device_name']}")

        # Resolve options:
        # - "resolved": Issue has been fixed
        # - "false_positive": Not a real anomaly
        # - "deferred": Will address later

        # Resolve it (commented out to prevent accidental changes)
        # result = await client.resolve_anomaly(
        #     anomaly_id=anomaly_id,
        #     resolution="resolved",
        #     notes="Sensor recalibrated, values now normal"
        # )
        # print(f"\nResolved: {result.get('resolved', False)}")

        print("\n(Resolve call commented out to prevent accidental changes)")


async def anomaly_analysis_report():
    """Generate a comprehensive anomaly analysis report"""
    print("\n" + "=" * 60)
    print("Comprehensive Anomaly Analysis Report")
    print("=" * 60)
    print(f"Generated: {datetime.utcnow().isoformat()}Z")

    async with AnalyticsClient() as client:
        # 1. Summary statistics
        print("\n--- Summary Statistics ---")
        result = await client.get_anomalies(limit=1000)
        summary = result.get("summary", {})

        print(f"Total anomalies (last 7 days): {summary.get('total', 0)}")
        print(f"\nBy severity:")
        for sev, count in summary.get("by_severity", {}).items():
            print(f"  {sev.capitalize()}: {count}")

        print(f"\nBy metric:")
        for metric, count in list(summary.get("by_metric", {}).items())[:5]:
            print(f"  {metric}: {count}")

        # 2. Trend analysis
        print("\n--- Trend Analysis ---")
        timeline = await client.get_anomaly_timeline(days=30)
        trend = timeline.get("trend", {})

        direction = trend.get("direction", "stable")
        change = trend.get("percent_change", 0)

        if direction == "increasing":
            print(f"WARNING: Anomalies increasing by {change:.1f}%")
        elif direction == "decreasing":
            print(f"GOOD: Anomalies decreasing by {abs(change):.1f}%")
        else:
            print(f"Anomaly rate is stable")

        # 3. Top affected devices
        print("\n--- Top Affected Devices ---")
        anomalies = result.get("anomalies", [])
        device_counts = {}
        for a in anomalies:
            device = a.get("device_name", "Unknown")
            device_counts[device] = device_counts.get(device, 0) + 1

        top_devices = sorted(device_counts.items(), key=lambda x: x[1], reverse=True)[:5]
        for device, count in top_devices:
            print(f"  {device}: {count} anomalies")

        # 4. Recommendations
        print("\n--- Recommendations ---")
        if summary.get("by_severity", {}).get("critical", 0) > 5:
            print("  ! Review critical anomalies immediately")
        if direction == "increasing":
            print("  ! Investigate root cause of increasing anomalies")
        if top_devices and top_devices[0][1] > 10:
            print(f"  ! Focus on device: {top_devices[0][0]}")


async def main():
    """Run all anomaly detection examples"""
    print("=" * 60)
    print("TESAIoT Analytics API - Anomaly Detection Examples")
    print("=" * 60)

    await get_critical_anomalies()
    await get_anomalies_for_devices()
    await get_anomaly_timeline()
    await get_anomaly_heatmap()
    await acknowledge_anomaly_example()
    await resolve_anomaly_example()
    await anomaly_analysis_report()

    print("\n" + "=" * 60)
    print("All examples completed!")


if __name__ == "__main__":
    asyncio.run(main())
