"""
IoT Connectivity Examples for TESAIoT Analytics API

This example demonstrates:
- Real-time device connectivity status
- Latency monitoring and analysis
- Throughput metrics
- Connection quality scores
- Network health monitoring

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
import json
from datetime import datetime
from analytics_client import AnalyticsClient


async def get_connectivity_overview():
    """Get overview of device connectivity"""
    print("\n" + "=" * 60)
    print("Device Connectivity Overview")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_connectivity_status()

        summary = result.get("summary", {})
        print(f"\n--- Summary ---")
        print(f"Total Devices: {summary.get('total', 0)}")
        print(f"Online: {summary.get('online', 0)}")
        print(f"Offline: {summary.get('offline', 0)}")

        online_pct = (summary.get('online', 0) / summary.get('total', 1)) * 100
        print(f"Online Percentage: {online_pct:.1f}%")

        # Visual indicator
        if online_pct >= 95:
            print("Status: EXCELLENT")
        elif online_pct >= 80:
            print("Status: GOOD")
        elif online_pct >= 50:
            print("Status: WARNING")
        else:
            print("Status: CRITICAL")


async def get_online_devices():
    """Get list of online devices"""
    print("\n" + "=" * 60)
    print("Online Devices")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_connectivity_status(status_filter="online")

        devices = result.get("devices", [])
        print(f"\n{len(devices)} devices online")

        for device in devices[:10]:
            uptime = device.get("uptime_percent", 0)
            last_seen = device.get("last_seen", "Unknown")
            print(f"\n  {device.get('device_name', 'Unknown')}")
            print(f"    ID: {device.get('device_id', 'N/A')}")
            print(f"    Last Seen: {last_seen}")
            print(f"    Uptime (30d): {uptime:.1f}%")


async def get_offline_devices():
    """Get list of offline devices"""
    print("\n" + "=" * 60)
    print("Offline Devices (Attention Needed)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_connectivity_status(status_filter="offline")

        devices = result.get("devices", [])
        print(f"\n{len(devices)} devices offline")

        for device in devices[:10]:
            last_seen = device.get("last_seen", "Unknown")
            print(f"\n  ! {device.get('device_name', 'Unknown')}")
            print(f"    ID: {device.get('device_id', 'N/A')}")
            print(f"    Last Seen: {last_seen}")

            # Calculate offline duration if possible
            try:
                last_dt = datetime.fromisoformat(last_seen.replace("Z", "+00:00"))
                offline_duration = datetime.utcnow().replace(tzinfo=last_dt.tzinfo) - last_dt
                hours = offline_duration.total_seconds() / 3600
                print(f"    Offline For: {hours:.1f} hours")
            except:
                pass


async def get_latency_analysis():
    """Analyze network latency"""
    print("\n" + "=" * 60)
    print("Network Latency Analysis (24 Hours)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_latency_stats(hours=24)

        summary = result.get("summary", {})
        print(f"\n--- Overall Statistics ---")
        print(f"Average Latency: {summary.get('avg_latency_ms', 0):.1f} ms")
        print(f"Median Latency: {summary.get('median_latency_ms', 0):.1f} ms")
        print(f"P95 Latency: {summary.get('p95_latency_ms', 0):.1f} ms")
        print(f"P99 Latency: {summary.get('p99_latency_ms', 0):.1f} ms")

        # Latency health indicator
        p95 = summary.get('p95_latency_ms', 0)
        if p95 < 100:
            print("Status: EXCELLENT (< 100ms)")
        elif p95 < 500:
            print("Status: GOOD (< 500ms)")
        elif p95 < 1000:
            print("Status: WARNING (< 1s)")
        else:
            print("Status: CRITICAL (> 1s)")

        # Per-device latency
        devices = result.get("devices", [])
        if devices:
            print(f"\n--- Per-Device Latency ---")

            # Sort by latency (highest first)
            devices.sort(key=lambda x: x.get("avg_latency_ms", 0), reverse=True)

            print("\nHighest Latency Devices:")
            for device in devices[:5]:
                print(f"  {device.get('device_name', 'Unknown')}: "
                      f"{device.get('avg_latency_ms', 0):.1f} ms "
                      f"(P95: {device.get('p95_latency_ms', 0):.1f} ms)")


async def get_throughput_analysis():
    """Analyze message throughput"""
    print("\n" + "=" * 60)
    print("Message Throughput Analysis (24 Hours)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_throughput_stats(hours=24)

        summary = result.get("summary", {})
        print(f"\n--- Overall Statistics ---")
        print(f"Total Messages: {summary.get('total_messages', 0):,}")
        print(f"Avg/Hour: {summary.get('avg_per_hour', 0):,.0f}")
        print(f"Peak/Hour: {summary.get('peak_per_hour', 0):,.0f}")
        print(f"Peak Time: {summary.get('peak_time', 'N/A')}")

        # Throughput by topic
        by_topic = result.get("by_topic", {})
        if by_topic:
            print(f"\n--- By Topic ---")
            for topic, count in sorted(by_topic.items(), key=lambda x: x[1], reverse=True)[:5]:
                print(f"  {topic}: {count:,} messages")

        # Timeline (hourly)
        timeline = result.get("timeline", [])
        if timeline:
            print(f"\n--- Hourly Timeline (Last 12 Hours) ---")
            for point in timeline[-12:]:
                hour = point.get("hour", "")
                count = point.get("messages", 0)
                # Simple bar chart
                bar_len = min(int(count / 1000), 40)
                bar = "" * bar_len
                print(f"  {hour}: {bar} {count:,}")


async def get_connection_quality():
    """Get connection quality scores"""
    print("\n" + "=" * 60)
    print("Connection Quality Scores")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_connection_quality()

        summary = result.get("summary", {})
        print(f"\n--- Overall Quality ---")
        avg_score = summary.get("average_quality_score", 0)
        print(f"Average Score: {avg_score:.0f}/100")

        # Quality distribution
        distribution = summary.get("distribution", {})
        print(f"\nQuality Distribution:")
        print(f"  Excellent (90-100): {distribution.get('excellent', 0)} devices")
        print(f"  Good (70-89): {distribution.get('good', 0)} devices")
        print(f"  Fair (50-69): {distribution.get('fair', 0)} devices")
        print(f"  Poor (< 50): {distribution.get('poor', 0)} devices")

        # Devices with issues
        issues = result.get("issues", [])
        if issues:
            print(f"\n--- Devices with Quality Issues ---")
            for issue in issues[:5]:
                print(f"\n  {issue.get('device_name', 'Unknown')}")
                print(f"    Score: {issue.get('quality_score', 0)}/100")
                print(f"    Issue: {issue.get('issue_type', 'Unknown')}")
                print(f"    Details: {issue.get('details', 'N/A')}")


async def connectivity_monitoring_loop():
    """Example of continuous connectivity monitoring"""
    print("\n" + "=" * 60)
    print("Continuous Connectivity Monitoring")
    print("(Press Ctrl+C to stop)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        iteration = 0
        while iteration < 3:  # Limited iterations for demo
            iteration += 1
            print(f"\n--- Check #{iteration} at {datetime.utcnow().isoformat()}Z ---")

            try:
                # Quick connectivity check
                status = await client.get_connectivity_status()
                summary = status.get("summary", {})

                online = summary.get("online", 0)
                offline = summary.get("offline", 0)
                total = summary.get("total", 0)

                print(f"Devices: {online}/{total} online, {offline} offline")

                # Alert on high offline count
                if offline > 0:
                    print(f"ALERT: {offline} device(s) offline!")

                    # Get offline device details
                    offline_status = await client.get_connectivity_status(status_filter="offline")
                    for device in offline_status.get("devices", [])[:3]:
                        print(f"  - {device.get('device_name', 'Unknown')}")

                # Quick latency check
                latency = await client.get_latency_stats(hours=1)
                p95 = latency.get("summary", {}).get("p95_latency_ms", 0)
                print(f"P95 Latency: {p95:.0f}ms")

                if p95 > 500:
                    print("WARNING: High latency detected!")

            except Exception as e:
                print(f"Error: {e}")

            # Wait before next check (5 seconds for demo, use longer in production)
            await asyncio.sleep(5)

    print("\nMonitoring stopped.")


async def network_health_report():
    """Generate comprehensive network health report"""
    print("\n" + "=" * 60)
    print("Network Health Report")
    print("=" * 60)
    print(f"Generated: {datetime.utcnow().isoformat()}Z")

    async with AnalyticsClient() as client:
        # Connectivity status
        status = await client.get_connectivity_status()
        summary = status.get("summary", {})

        # Latency stats
        latency = await client.get_latency_stats(hours=24)
        latency_summary = latency.get("summary", {})

        # Throughput stats
        throughput = await client.get_throughput_stats(hours=24)
        throughput_summary = throughput.get("summary", {})

        # Quality scores
        quality = await client.get_connection_quality()
        quality_summary = quality.get("summary", {})

        # Calculate overall health score
        online_pct = (summary.get("online", 0) / max(summary.get("total", 1), 1)) * 100
        quality_score = quality_summary.get("average_quality_score", 0)
        latency_score = max(0, 100 - (latency_summary.get("p95_latency_ms", 0) / 10))

        health_score = (online_pct * 0.4 + quality_score * 0.4 + latency_score * 0.2)

        print(f"\n{'=' * 40}")
        print(f" OVERALL HEALTH SCORE: {health_score:.0f}/100")
        print(f"{'=' * 40}")

        # Connectivity section
        print(f"\n--- CONNECTIVITY ---")
        print(f"Total Devices: {summary.get('total', 0)}")
        print(f"Online: {summary.get('online', 0)} ({online_pct:.1f}%)")
        print(f"Offline: {summary.get('offline', 0)}")

        # Latency section
        print(f"\n--- LATENCY ---")
        print(f"Average: {latency_summary.get('avg_latency_ms', 0):.1f} ms")
        print(f"P95: {latency_summary.get('p95_latency_ms', 0):.1f} ms")
        print(f"P99: {latency_summary.get('p99_latency_ms', 0):.1f} ms")

        # Throughput section
        print(f"\n--- THROUGHPUT ---")
        print(f"Total Messages (24h): {throughput_summary.get('total_messages', 0):,}")
        print(f"Average/Hour: {throughput_summary.get('avg_per_hour', 0):,.0f}")
        print(f"Peak/Hour: {throughput_summary.get('peak_per_hour', 0):,.0f}")

        # Quality section
        print(f"\n--- CONNECTION QUALITY ---")
        print(f"Average Score: {quality_score:.0f}/100")
        dist = quality_summary.get("distribution", {})
        print(f"Excellent: {dist.get('excellent', 0)}")
        print(f"Good: {dist.get('good', 0)}")
        print(f"Fair: {dist.get('fair', 0)}")
        print(f"Poor: {dist.get('poor', 0)}")

        # Issues section
        print(f"\n--- ISSUES & ALERTS ---")

        if summary.get("offline", 0) > 0:
            print(f"! {summary.get('offline', 0)} device(s) are offline")

        if latency_summary.get("p95_latency_ms", 0) > 500:
            print(f"! High latency detected (P95 > 500ms)")

        issues = quality.get("issues", [])
        if issues:
            print(f"! {len(issues)} device(s) with connection quality issues")

        if not (summary.get("offline", 0) or
                latency_summary.get("p95_latency_ms", 0) > 500 or
                issues):
            print(" No issues detected")

        # Recommendations
        print(f"\n--- RECOMMENDATIONS ---")
        recs = []

        if summary.get("offline", 0) > summary.get("total", 0) * 0.1:
            recs.append("Investigate offline devices - more than 10% are down")

        if latency_summary.get("p95_latency_ms", 0) > 1000:
            recs.append("Network optimization needed - latency is very high")

        if dist.get("poor", 0) > 5:
            recs.append("Review devices with poor connection quality")

        if not recs:
            recs.append("Network is healthy - continue monitoring")

        for i, rec in enumerate(recs, 1):
            print(f"{i}. {rec}")


async def main():
    """Run all connectivity examples"""
    print("=" * 60)
    print("TESAIoT Analytics API - Connectivity Examples")
    print("=" * 60)

    await get_connectivity_overview()
    await get_online_devices()
    await get_offline_devices()
    await get_latency_analysis()
    await get_throughput_analysis()
    await get_connection_quality()
    await connectivity_monitoring_loop()
    await network_health_report()

    print("\n" + "=" * 60)
    print("All examples completed!")


if __name__ == "__main__":
    asyncio.run(main())
