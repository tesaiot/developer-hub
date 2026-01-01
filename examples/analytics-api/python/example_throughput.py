#!/usr/bin/env python3
"""
TESAIoT BDH AI Analytics - Throughput Statistics Example

This example demonstrates how to:
1. Get message throughput statistics
2. Analyze throughput by topic
3. Identify traffic patterns over time

API Endpoint: GET /api/v1/bdh-ai/connectivity/throughput

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


async def get_throughput_stats(hours: int = 24) -> dict:
    """
    Get message throughput statistics.

    Args:
        hours: Time range in hours (default: 24)

    Returns:
        dict with throughput statistics, timeline, and topic breakdown
    """
    async with httpx.AsyncClient(verify=False, timeout=30.0) as client:
        response = await client.get(
            f"{API_BASE_URL}/connectivity/throughput",
            params={"hours": hours},
            headers={"X-API-KEY": API_TOKEN}
        )
        response.raise_for_status()
        return response.json()


def format_bytes(bytes_value: int) -> str:
    """Format bytes to human-readable string"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_value < 1024:
            return f"{bytes_value:.2f} {unit}"
        bytes_value /= 1024
    return f"{bytes_value:.2f} TB"


def print_throughput_report(data: dict):
    """Print formatted throughput report"""
    print("\n" + "=" * 60)
    print("  TESAIoT BDH AI - Message Throughput Report")
    print("=" * 60)

    # Summary statistics
    if "summary" in data:
        summary = data["summary"]
        print("\n📊 Summary Statistics:")
        print(f"   • Total Messages: {summary.get('total_messages', 0):,}")
        print(f"   • Total Bytes: {format_bytes(summary.get('total_bytes', 0))}")
        print(f"   • Messages/Hour: {summary.get('messages_per_hour', 0):,.0f}")
        print(f"   • Bytes/Hour: {format_bytes(summary.get('bytes_per_hour', 0))}")
        print(f"   • Peak Messages/Min: {summary.get('peak_messages_per_min', 0):,}")
        print(f"   • Active Devices: {summary.get('active_devices', 0)}")

    # By topic breakdown
    if "by_topic" in data and data["by_topic"]:
        print("\n📬 Throughput by Topic:")
        print("-" * 60)
        print(f"{'Topic':<30} {'Messages':<12} {'Bytes':<15}")
        print("-" * 60)

        for topic in data["by_topic"][:10]:  # Top 10 topics
            topic_name = topic.get("topic", "Unknown")
            # Truncate long topic names
            if len(topic_name) > 28:
                topic_name = topic_name[:25] + "..."
            messages = topic.get("message_count", 0)
            bytes_val = topic.get("total_bytes", 0)
            print(f"{topic_name:<30} {messages:<12,} {format_bytes(bytes_val):<15}")

    # By device breakdown
    if "by_device" in data and data["by_device"]:
        print("\n📱 Top 5 Active Devices:")
        print("-" * 60)
        print(f"{'Device ID':<40} {'Messages':<12}")
        print("-" * 60)

        for device in data["by_device"][:5]:
            device_id = device.get("device_id", "Unknown")[:36]
            messages = device.get("message_count", 0)
            print(f"{device_id:<40} {messages:<12,}")

    # Timeline visualization
    if "timeline" in data and data["timeline"]:
        print("\n📈 Throughput Timeline (Last 6 hours):")
        max_msgs = max(p.get("message_count", 1) for p in data["timeline"][-6:])

        for point in data["timeline"][-6:]:
            timestamp = point.get("timestamp", "")[:16]
            msgs = point.get("message_count", 0)
            bar_len = int((msgs / max_msgs) * 40) if max_msgs > 0 else 0
            bar = "█" * bar_len
            print(f"   {timestamp}: {bar} ({msgs:,} msgs)")

    print("\n" + "=" * 60)
    print(f"Generated at: {datetime.utcnow().isoformat()}Z")
    print("=" * 60 + "\n")


async def main():
    """Main function"""
    print("Fetching throughput statistics from TESAIoT Platform...")

    if not API_TOKEN:
        print("[ERROR] API token not set. Set TESAIOT_API_TOKEN environment variable.")
        return

    try:
        # Get throughput stats for last 24 hours
        data = await get_throughput_stats(hours=24)
        print_throughput_report(data)

        # Compare with 1-hour stats
        print("\nFetching 1-hour stats for recent activity...")
        recent_data = await get_throughput_stats(hours=1)

        if recent_data.get("summary"):
            recent = recent_data["summary"]
            print(f"📊 Last Hour: {recent.get('total_messages', 0):,} messages, "
                  f"{format_bytes(recent.get('total_bytes', 0))}")

    except httpx.HTTPStatusError as e:
        print(f"[ERROR] HTTP {e.response.status_code}: {e.response.text}")
    except Exception as e:
        print(f"[ERROR] {type(e).__name__}: {e}")


if __name__ == "__main__":
    asyncio.run(main())
