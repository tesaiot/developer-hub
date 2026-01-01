#!/usr/bin/env python3
"""
TESAIoT BDH AI Analytics - Latency Statistics Example

This example demonstrates how to:
1. Get network latency statistics for devices
2. Analyze latency trends over time
3. Identify devices with high latency

API Endpoint: GET /api/v1/bdh-ai/connectivity/latency

Author: TESAIoT Team
License: Apache 2.0
"""

import os
import asyncio
import httpx
from dotenv import load_dotenv
from datetime import datetime

# Load environment
load_dotenv()

# Configuration
API_BASE_URL = os.getenv("TESAIOT_API_URL", "https://admin.tesaiot.com/api/v1/bdh-ai")
API_TOKEN = os.getenv("TESAIOT_API_TOKEN", "")


async def get_latency_stats(hours: int = 24) -> dict:
    """
    Get network latency statistics.

    Args:
        hours: Time range in hours (default: 24)

    Returns:
        dict with latency statistics and per-device breakdown
    """
    async with httpx.AsyncClient(verify=False, timeout=30.0) as client:
        response = await client.get(
            f"{API_BASE_URL}/connectivity/latency",
            params={"hours": hours},
            headers={"X-API-KEY": API_TOKEN}
        )
        response.raise_for_status()
        return response.json()


def print_latency_report(data: dict):
    """Print formatted latency report"""
    print("\n" + "=" * 60)
    print("  TESAIoT BDH AI - Network Latency Report")
    print("=" * 60)

    # Summary statistics
    if "summary" in data:
        summary = data["summary"]
        print("\n📊 Summary Statistics:")
        print(f"   • Average Latency: {summary.get('avg_latency_ms', 0):.2f} ms")
        print(f"   • Min Latency: {summary.get('min_latency_ms', 0):.2f} ms")
        print(f"   • Max Latency: {summary.get('max_latency_ms', 0):.2f} ms")
        print(f"   • P95 Latency: {summary.get('p95_latency_ms', 0):.2f} ms")
        print(f"   • P99 Latency: {summary.get('p99_latency_ms', 0):.2f} ms")
        print(f"   • Total Samples: {summary.get('total_samples', 0)}")

    # Per-device breakdown
    if "devices" in data and data["devices"]:
        print("\n📱 Per-Device Latency:")
        print("-" * 60)
        print(f"{'Device ID':<40} {'Avg (ms)':<10} {'Status':<10}")
        print("-" * 60)

        for device in data["devices"][:10]:  # Top 10
            device_id = device.get("device_id", "Unknown")[:36]
            avg_latency = device.get("avg_latency_ms", 0)
            status = "🟢 Good" if avg_latency < 100 else "🟡 Fair" if avg_latency < 500 else "🔴 High"
            print(f"{device_id:<40} {avg_latency:<10.2f} {status:<10}")

    # High latency alerts
    if "high_latency_devices" in data and data["high_latency_devices"]:
        print("\n⚠️  High Latency Alerts:")
        for device in data["high_latency_devices"]:
            print(f"   • {device['device_id']}: {device['avg_latency_ms']:.2f} ms")

    # Latency timeline (if available)
    if "timeline" in data and data["timeline"]:
        print("\n📈 Latency Timeline (Last 6 hours):")
        for point in data["timeline"][-6:]:
            timestamp = point.get("timestamp", "")[:16]
            avg = point.get("avg_latency_ms", 0)
            bar = "█" * int(avg / 10) if avg < 500 else "█" * 50 + "+"
            print(f"   {timestamp}: {bar} ({avg:.0f} ms)")

    print("\n" + "=" * 60)
    print(f"Generated at: {datetime.utcnow().isoformat()}Z")
    print("=" * 60 + "\n")


async def main():
    """Main function"""
    print("Fetching latency statistics from TESAIoT Platform...")

    if not API_TOKEN:
        print("[ERROR] API token not set. Set TESAIOT_API_TOKEN environment variable.")
        return

    try:
        # Get latency stats for last 24 hours
        data = await get_latency_stats(hours=24)
        print_latency_report(data)

        # Also try 1-hour stats for recent view
        print("\nFetching 1-hour stats for recent activity...")
        recent_data = await get_latency_stats(hours=1)

        if recent_data.get("summary"):
            recent_summary = recent_data["summary"]
            print(f"📊 Last Hour: Avg={recent_summary.get('avg_latency_ms', 0):.2f}ms, "
                  f"Samples={recent_summary.get('total_samples', 0)}")

    except httpx.HTTPStatusError as e:
        print(f"[ERROR] HTTP {e.response.status_code}: {e.response.text}")
    except Exception as e:
        print(f"[ERROR] {type(e).__name__}: {e}")


if __name__ == "__main__":
    asyncio.run(main())
