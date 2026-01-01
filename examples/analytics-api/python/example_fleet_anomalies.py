#!/usr/bin/env python3
"""
TESAIoT BDH AI Analytics - Fleet Anomalies Example

This example demonstrates how to:
1. Get fleet-wide anomaly overview
2. Group anomalies by device, severity, and type
3. Generate anomaly summary reports

API Endpoint: GET /api/v1/bdh-ai/analytics/anomalies

Author: TESAIoT Team
License: Apache 2.0
"""

import os
import asyncio
import httpx
from dotenv import load_dotenv
from datetime import datetime, timedelta

# Load environment
load_dotenv()

# Configuration
API_BASE_URL = os.getenv("TESAIOT_API_URL", "https://admin.tesaiot.com/api/v1/bdh-ai")
API_TOKEN = os.getenv("TESAIOT_API_TOKEN", "")


async def get_fleet_anomalies(
    days: int = 7,
    severity: str = None,
    limit: int = 100
) -> dict:
    """
    Get fleet-wide anomaly overview.

    Args:
        days: Number of days to analyze (default: 7)
        severity: Filter by severity (critical, high, medium, low)
        limit: Maximum number of anomalies to return

    Returns:
        dict with anomalies, summary, and groupings
    """
    async with httpx.AsyncClient(verify=False, timeout=30.0) as client:
        params = {
            "days": days,
            "limit": limit
        }
        if severity:
            params["severity"] = severity

        response = await client.get(
            f"{API_BASE_URL}/analytics/anomalies",
            params=params,
            headers={"X-API-KEY": API_TOKEN}
        )
        response.raise_for_status()
        return response.json()


def severity_icon(severity: str) -> str:
    """Get icon for severity level"""
    icons = {
        "critical": "🔴",
        "high": "🟠",
        "medium": "🟡",
        "low": "🟢"
    }
    return icons.get(severity.lower(), "⚪")


def print_fleet_anomalies_report(data: dict):
    """Print formatted fleet anomalies report"""
    print("\n" + "=" * 70)
    print("  TESAIoT BDH AI - Fleet Anomalies Report")
    print("=" * 70)

    # Summary statistics
    if "summary" in data:
        summary = data["summary"]
        print("\n📊 Fleet Anomaly Summary:")
        print(f"   • Total Anomalies: {summary.get('total_anomalies', 0)}")
        print(f"   • Affected Devices: {summary.get('affected_devices', 0)}")
        print(f"   • Analysis Period: {summary.get('analysis_period_days', 7)} days")

        # By severity breakdown
        if "by_severity" in summary:
            print("\n   Breakdown by Severity:")
            for sev, count in summary["by_severity"].items():
                icon = severity_icon(sev)
                print(f"   {icon} {sev.capitalize()}: {count}")

        # Trend indicator
        if "trend" in summary:
            trend = summary["trend"]
            trend_icon = "📈" if trend > 0 else "📉" if trend < 0 else "➡️"
            print(f"\n   {trend_icon} Trend: {trend:+.1f}% vs previous period")

    # By device grouping
    if "by_device" in data and data["by_device"]:
        print("\n📱 Anomalies by Device (Top 10):")
        print("-" * 70)
        print(f"{'Device':<42} {'Total':<8} {'Critical':<10} {'High':<8}")
        print("-" * 70)

        for device in data["by_device"][:10]:
            device_id = device.get("device_id", "Unknown")[:38]
            total = device.get("total_anomalies", 0)
            critical = device.get("critical_count", 0)
            high = device.get("high_count", 0)

            # Add alert indicator for devices with critical anomalies
            indicator = "⚠️ " if critical > 0 else "   "
            print(f"{indicator}{device_id:<40} {total:<8} {critical:<10} {high:<8}")

    # By type grouping
    if "by_type" in data and data["by_type"]:
        print("\n📋 Anomalies by Type:")
        print("-" * 50)
        for anomaly_type in data["by_type"]:
            type_name = anomaly_type.get("type", "Unknown")
            count = anomaly_type.get("count", 0)
            percentage = anomaly_type.get("percentage", 0)
            bar = "█" * int(percentage / 5)
            print(f"   {type_name:<20} {count:>5} ({percentage:>5.1f}%) {bar}")

    # Recent critical anomalies
    if "recent_critical" in data and data["recent_critical"]:
        print("\n🚨 Recent Critical Anomalies:")
        print("-" * 70)
        for anomaly in data["recent_critical"][:5]:
            timestamp = anomaly.get("timestamp", "")[:16]
            device_id = anomaly.get("device_id", "")[:20]
            metric = anomaly.get("metric", "Unknown")
            value = anomaly.get("value", 0)
            print(f"   {timestamp} | {device_id}... | {metric}: {value:.2f}")

    # Recent anomalies list (if no critical)
    if "anomalies" in data and data["anomalies"] and "recent_critical" not in data:
        print("\n📝 Recent Anomalies:")
        print("-" * 70)
        print(f"{'Time':<18} {'Device':<22} {'Metric':<12} {'Severity':<10}")
        print("-" * 70)

        for anomaly in data["anomalies"][:10]:
            timestamp = anomaly.get("timestamp", "")[:16]
            device_id = anomaly.get("device_id", "")[:18]
            metric = anomaly.get("metric", "")[:10]
            severity = anomaly.get("severity", "")
            icon = severity_icon(severity)
            print(f"   {timestamp} {device_id}... {metric:<12} {icon} {severity:<8}")

    # Recommendations
    if "recommendations" in data and data["recommendations"]:
        print("\n💡 Recommendations:")
        for i, rec in enumerate(data["recommendations"][:3], 1):
            print(f"   {i}. {rec}")

    print("\n" + "=" * 70)
    print(f"Generated at: {datetime.utcnow().isoformat()}Z")
    print("=" * 70 + "\n")


async def main():
    """Main function"""
    print("Fetching fleet anomalies from TESAIoT Platform...")

    if not API_TOKEN:
        print("[ERROR] API token not set. Set TESAIOT_API_TOKEN environment variable.")
        return

    try:
        # Get all anomalies for last 7 days
        print("\n[1/3] Fetching 7-day overview...")
        data = await get_fleet_anomalies(days=7)
        print_fleet_anomalies_report(data)

        # Get critical anomalies only
        print("[2/3] Fetching critical anomalies...")
        critical_data = await get_fleet_anomalies(days=7, severity="critical")
        if critical_data.get("summary", {}).get("total_anomalies", 0) > 0:
            print(f"   🔴 Critical anomalies found: {critical_data['summary']['total_anomalies']}")
        else:
            print("   ✅ No critical anomalies in the last 7 days")

        # Get last 24 hours
        print("[3/3] Fetching last 24 hours...")
        recent_data = await get_fleet_anomalies(days=1)
        if recent_data.get("summary"):
            total = recent_data["summary"].get("total_anomalies", 0)
            print(f"   📊 Last 24h: {total} anomalies detected")

    except httpx.HTTPStatusError as e:
        print(f"[ERROR] HTTP {e.response.status_code}: {e.response.text}")
    except Exception as e:
        print(f"[ERROR] {type(e).__name__}: {e}")


if __name__ == "__main__":
    asyncio.run(main())
