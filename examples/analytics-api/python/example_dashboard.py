"""
Complete Dashboard Example for TESAIoT Analytics API

This example demonstrates building a comprehensive IoT analytics
dashboard that combines all four analytics domains:
- Anomaly Detection
- Pattern Recognition
- AI Insights
- Connectivity Metrics

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
import json
from datetime import datetime
from dataclasses import dataclass
from typing import Dict, Any, List, Optional
from analytics_client import AnalyticsClient, TimeRange


@dataclass
class DashboardData:
    """Container for all dashboard data"""
    timestamp: str
    fleet_health: Dict[str, Any]
    anomalies: Dict[str, Any]
    clusters: Dict[str, Any]
    insights: List[Dict[str, Any]]
    connectivity: Dict[str, Any]
    alerts: List[Dict[str, Any]]


async def collect_dashboard_data() -> DashboardData:
    """Collect all data needed for the dashboard"""
    async with AnalyticsClient() as client:
        # Collect all data in parallel for efficiency
        anomalies_task = client.get_anomalies(limit=100, severity_filter=["critical", "high", "medium"])
        timeline_task = client.get_anomaly_timeline(days=30)
        clusters_task = client.get_clusters(metric_name="temperature", n_clusters=5)
        insights_task = client.get_insights(days=7)
        connectivity_task = client.get_connectivity_status()
        latency_task = client.get_latency_stats(hours=24)
        throughput_task = client.get_throughput_stats(hours=24)
        quality_task = client.get_connection_quality()

        # Await all tasks
        anomalies, timeline, clusters, insights, connectivity, latency, throughput, quality = await asyncio.gather(
            anomalies_task, timeline_task, clusters_task, insights_task,
            connectivity_task, latency_task, throughput_task, quality_task
        )

        # Process and structure the data
        alerts = generate_alerts(anomalies, connectivity, latency, quality)

        fleet_health = calculate_fleet_health(
            anomalies, connectivity, insights, latency
        )

        return DashboardData(
            timestamp=datetime.utcnow().isoformat() + "Z",
            fleet_health=fleet_health,
            anomalies={
                "summary": anomalies.get("summary", {}),
                "recent": anomalies.get("anomalies", [])[:10],
                "timeline": timeline.get("timeline", []),
                "trend": timeline.get("trend", {})
            },
            clusters={
                "clusters": clusters.get("clusters", []),
                "silhouette_score": clusters.get("silhouette_score", 0),
                "outliers": clusters.get("outliers", [])[:5]
            },
            insights=insights.get("insights", []),
            connectivity={
                "status": connectivity.get("summary", {}),
                "devices": connectivity.get("devices", [])[:20],
                "latency": latency.get("summary", {}),
                "throughput": throughput.get("summary", {}),
                "quality": quality.get("summary", {})
            },
            alerts=alerts
        )


def calculate_fleet_health(anomalies, connectivity, insights, latency) -> Dict[str, Any]:
    """Calculate overall fleet health score"""
    scores = {}

    # Anomaly score (0-100, lower anomaly rate = higher score)
    anomaly_summary = anomalies.get("summary", {})
    total_devices = connectivity.get("summary", {}).get("total", 100)
    anomaly_count = anomaly_summary.get("total", 0)
    anomaly_rate = anomaly_count / max(total_devices, 1)
    scores["anomaly"] = max(0, 100 - (anomaly_rate * 1000))

    # Connectivity score (percentage online)
    conn_summary = connectivity.get("summary", {})
    online = conn_summary.get("online", 0)
    total = conn_summary.get("total", 1)
    scores["connectivity"] = (online / max(total, 1)) * 100

    # Latency score (0-100, lower latency = higher score)
    p95_latency = latency.get("summary", {}).get("p95_latency_ms", 0)
    scores["latency"] = max(0, 100 - (p95_latency / 10))

    # Insights severity score
    insights_list = insights.get("insights", [])
    critical_count = sum(1 for i in insights_list if i.get("severity") == "critical")
    warning_count = sum(1 for i in insights_list if i.get("severity") == "warning")
    scores["insights"] = max(0, 100 - (critical_count * 20) - (warning_count * 5))

    # Calculate weighted overall score
    overall = (
        scores["anomaly"] * 0.3 +
        scores["connectivity"] * 0.3 +
        scores["latency"] * 0.2 +
        scores["insights"] * 0.2
    )

    return {
        "overall_score": round(overall, 1),
        "component_scores": scores,
        "status": get_health_status(overall)
    }


def get_health_status(score: float) -> str:
    """Convert health score to status label"""
    if score >= 90:
        return "EXCELLENT"
    elif score >= 70:
        return "GOOD"
    elif score >= 50:
        return "FAIR"
    elif score >= 30:
        return "POOR"
    else:
        return "CRITICAL"


def generate_alerts(anomalies, connectivity, latency, quality) -> List[Dict[str, Any]]:
    """Generate alerts based on current data"""
    alerts = []

    # Critical anomaly alerts
    anomaly_summary = anomalies.get("summary", {})
    critical_count = anomaly_summary.get("by_severity", {}).get("critical", 0)
    if critical_count > 0:
        alerts.append({
            "level": "critical",
            "type": "anomaly",
            "title": f"{critical_count} Critical Anomalies Detected",
            "description": "Immediate investigation recommended"
        })

    # Offline device alerts
    conn_summary = connectivity.get("summary", {})
    offline = conn_summary.get("offline", 0)
    total = conn_summary.get("total", 1)
    offline_pct = (offline / max(total, 1)) * 100

    if offline_pct > 20:
        alerts.append({
            "level": "critical",
            "type": "connectivity",
            "title": f"{offline} Devices Offline ({offline_pct:.0f}%)",
            "description": "Network connectivity issue detected"
        })
    elif offline > 0:
        alerts.append({
            "level": "warning",
            "type": "connectivity",
            "title": f"{offline} Device(s) Offline",
            "description": "Some devices are not responding"
        })

    # Latency alerts
    p95 = latency.get("summary", {}).get("p95_latency_ms", 0)
    if p95 > 1000:
        alerts.append({
            "level": "critical",
            "type": "latency",
            "title": f"High Latency: {p95:.0f}ms P95",
            "description": "Network performance severely degraded"
        })
    elif p95 > 500:
        alerts.append({
            "level": "warning",
            "type": "latency",
            "title": f"Elevated Latency: {p95:.0f}ms P95",
            "description": "Network performance degraded"
        })

    # Quality alerts
    poor_devices = quality.get("summary", {}).get("distribution", {}).get("poor", 0)
    if poor_devices > 5:
        alerts.append({
            "level": "warning",
            "type": "quality",
            "title": f"{poor_devices} Devices with Poor Connection Quality",
            "description": "Review device connections and network path"
        })

    return alerts


def render_dashboard(data: DashboardData):
    """Render the dashboard to console"""
    print("\n" + "=" * 80)
    print(" " * 25 + "TESAIoT ANALYTICS DASHBOARD")
    print("=" * 80)
    print(f"Generated: {data.timestamp}")

    # Fleet Health Overview
    print("\n" + "-" * 80)
    print(" FLEET HEALTH")
    print("-" * 80)

    health = data.fleet_health
    score = health["overall_score"]
    status = health["status"]

    # Visual health bar
    bar_len = int(score / 5)
    bar = "" * bar_len + "" * (20 - bar_len)
    print(f"\n  Overall: [{bar}] {score:.0f}/100 ({status})")

    print(f"\n  Component Scores:")
    for component, comp_score in health["component_scores"].items():
        print(f"    {component.capitalize():15} {comp_score:.0f}/100")

    # Alerts Section
    if data.alerts:
        print("\n" + "-" * 80)
        print(" ACTIVE ALERTS")
        print("-" * 80)

        for alert in data.alerts:
            icon = "!" if alert["level"] == "critical" else "!" if alert["level"] == "warning" else "i"
            print(f"\n  [{icon}] {alert['title']}")
            print(f"      {alert['description']}")

    # Anomaly Detection Section
    print("\n" + "-" * 80)
    print(" ANOMALY DETECTION")
    print("-" * 80)

    anom = data.anomalies
    summary = anom["summary"]
    trend = anom.get("trend", {})

    print(f"\n  Total Anomalies (7 days): {summary.get('total', 0)}")
    print(f"  By Severity:")
    for sev, count in summary.get("by_severity", {}).items():
        print(f"    {sev.capitalize():10} {count}")

    trend_dir = trend.get("direction", "stable")
    trend_pct = trend.get("percent_change", 0)
    trend_icon = "" if trend_dir == "decreasing" else "" if trend_dir == "increasing" else ""
    print(f"\n  Trend: {trend_icon} {trend_dir} ({trend_pct:+.1f}%)")

    # Pattern Recognition Section
    print("\n" + "-" * 80)
    print(" PATTERN RECOGNITION")
    print("-" * 80)

    clusters = data.clusters
    print(f"\n  Clusters: {len(clusters['clusters'])}")
    print(f"  Silhouette Score: {clusters['silhouette_score']:.3f}")

    for cluster in clusters["clusters"]:
        print(f"\n    Cluster {cluster['cluster_id']}: {cluster['device_count']} devices")
        chars = cluster.get("characteristics", {})
        if chars:
            print(f"      Mean: {chars.get('mean', 0):.2f}, StdDev: {chars.get('std_dev', 0):.2f}")

    if clusters["outliers"]:
        print(f"\n  Outliers: {len(clusters['outliers'])} devices")

    # AI Insights Section
    print("\n" + "-" * 80)
    print(" AI INSIGHTS")
    print("-" * 80)

    insights = data.insights
    print(f"\n  Total Insights: {len(insights)}")

    critical_insights = [i for i in insights if i.get("severity") == "critical"]
    if critical_insights:
        print(f"\n  Critical Insights:")
        for i in critical_insights[:3]:
            print(f"    ! {i['title']}")

    actionable = [i for i in insights if i.get("actionable")]
    if actionable:
        print(f"\n  Actionable Recommendations ({len(actionable)}):")
        for i in actionable[:3]:
            print(f"    - {i['title']}")

    # Connectivity Section
    print("\n" + "-" * 80)
    print(" CONNECTIVITY")
    print("-" * 80)

    conn = data.connectivity
    status = conn["status"]

    print(f"\n  Devices: {status.get('online', 0)}/{status.get('total', 0)} online")

    latency = conn["latency"]
    print(f"\n  Latency:")
    print(f"    Average: {latency.get('avg_latency_ms', 0):.1f} ms")
    print(f"    P95: {latency.get('p95_latency_ms', 0):.1f} ms")

    throughput = conn["throughput"]
    print(f"\n  Throughput (24h):")
    print(f"    Total Messages: {throughput.get('total_messages', 0):,}")
    print(f"    Avg/Hour: {throughput.get('avg_per_hour', 0):,.0f}")

    quality = conn["quality"]
    print(f"\n  Connection Quality: {quality.get('average_quality_score', 0):.0f}/100")

    # Footer
    print("\n" + "=" * 80)
    print(" " * 30 + "END OF REPORT")
    print("=" * 80)


async def dashboard_refresh_loop(interval_seconds: int = 60, max_iterations: int = 5):
    """Continuously refresh dashboard"""
    print("Starting dashboard refresh loop...")
    print(f"Refresh interval: {interval_seconds} seconds")
    print("Press Ctrl+C to stop\n")

    iteration = 0
    while iteration < max_iterations:
        iteration += 1

        try:
            # Collect fresh data
            data = await collect_dashboard_data()

            # Clear screen (simulated with newlines in this demo)
            print("\n" * 2)

            # Render dashboard
            render_dashboard(data)

            print(f"\n[Refresh {iteration}/{max_iterations} - "
                  f"Next refresh in {interval_seconds} seconds]")

            # Wait for next refresh
            await asyncio.sleep(interval_seconds)

        except KeyboardInterrupt:
            print("\n\nDashboard stopped by user.")
            break
        except Exception as e:
            print(f"\nError refreshing dashboard: {e}")
            await asyncio.sleep(10)

    print("\nDashboard loop completed.")


async def export_dashboard_json(output_file: str = "dashboard_data.json"):
    """Export dashboard data to JSON file"""
    print(f"Exporting dashboard data to {output_file}...")

    data = await collect_dashboard_data()

    # Convert dataclass to dict for JSON serialization
    export_data = {
        "timestamp": data.timestamp,
        "fleet_health": data.fleet_health,
        "anomalies": data.anomalies,
        "clusters": data.clusters,
        "insights": data.insights,
        "connectivity": data.connectivity,
        "alerts": data.alerts
    }

    with open(output_file, "w") as f:
        json.dump(export_data, f, indent=2)

    print(f"Dashboard data exported to {output_file}")
    print(f"File size: {os.path.getsize(output_file)} bytes")


async def main():
    """Main dashboard entry point"""
    import sys

    if len(sys.argv) > 1:
        if sys.argv[1] == "--loop":
            interval = int(sys.argv[2]) if len(sys.argv) > 2 else 60
            await dashboard_refresh_loop(interval_seconds=interval)
        elif sys.argv[1] == "--export":
            output = sys.argv[2] if len(sys.argv) > 2 else "dashboard_data.json"
            await export_dashboard_json(output)
        elif sys.argv[1] == "--help":
            print("TESAIoT Analytics Dashboard")
            print("\nUsage:")
            print("  python example_dashboard.py          # Single refresh")
            print("  python example_dashboard.py --loop   # Continuous refresh (60s)")
            print("  python example_dashboard.py --loop 30  # Custom interval")
            print("  python example_dashboard.py --export  # Export to JSON")
            return
    else:
        # Single dashboard render
        print("Collecting dashboard data...")
        data = await collect_dashboard_data()
        render_dashboard(data)


if __name__ == "__main__":
    asyncio.run(main())
