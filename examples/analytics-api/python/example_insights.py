"""
AI Insights Examples for TESAIoT Analytics API

This example demonstrates:
- Generating AI-powered insights
- Filtering by insight type and confidence
- Actionable recommendations
- Fleet health summary
- Trend analysis and correlations

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
import json
from datetime import datetime
from analytics_client import AnalyticsClient


async def get_all_insights():
    """Get all AI-generated insights"""
    print("\n" + "=" * 60)
    print("All AI Insights (Last 7 Days)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_insights(days=7)

        # Fleet summary
        summary = result.get("fleet_summary", {})
        print("\n--- Fleet Summary ---")
        print(f"Total Devices: {summary.get('total_devices', 0)}")
        print(f"Active Devices: {summary.get('active_devices', 0)}")
        print(f"Anomaly Rate: {summary.get('anomaly_rate', 0):.2%}")
        print(f"Health Score: {summary.get('health_score', 0):.1f}/100")

        # All insights
        insights = result.get("insights", [])
        print(f"\n--- {len(insights)} Insights Generated ---")

        for insight in insights:
            severity_icon = {
                "critical": "!",
                "warning": "!",
                "info": "i"
            }.get(insight.get("severity", "info"), "*")

            print(f"\n[{severity_icon}] {insight['title']}")
            print(f"    Type: {insight['type']}")
            print(f"    Severity: {insight['severity']}")
            print(f"    Confidence: {insight['confidence']:.0%}")
            print(f"    {insight['description']}")

            if insight.get("actionable"):
                print(f"    Recommended Actions:")
                for action in insight.get("recommended_actions", []):
                    print(f"      - {action}")


async def get_insights_by_type():
    """Filter insights by type"""
    print("\n" + "=" * 60)
    print("Insights by Type")
    print("=" * 60)

    insight_types = [
        ("anomaly_pattern", "Anomaly Patterns"),
        ("trend", "Trend Analysis"),
        ("correlation", "Correlations"),
        ("recommendation", "Recommendations")
    ]

    async with AnalyticsClient() as client:
        for type_id, type_name in insight_types:
            result = await client.get_insights(
                days=7,
                insight_types=[type_id]
            )

            insights = result.get("insights", [])
            print(f"\n{type_name}: {len(insights)} insights")

            for insight in insights[:3]:
                print(f"  - {insight['title']}")
                print(f"    Confidence: {insight['confidence']:.0%}")


async def get_high_confidence_insights():
    """Get only high-confidence insights"""
    print("\n" + "=" * 60)
    print("High Confidence Insights (>80%)")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_insights(
            days=7,
            min_confidence=0.8
        )

        insights = result.get("insights", [])
        print(f"\nFound {len(insights)} high-confidence insights")

        for insight in insights:
            print(f"\n[{insight['confidence']:.0%}] {insight['title']}")
            print(f"  {insight['description']}")


async def get_actionable_recommendations():
    """Get actionable recommendations"""
    print("\n" + "=" * 60)
    print("Actionable Recommendations")
    print("=" * 60)

    async with AnalyticsClient() as client:
        # Get from insights endpoint
        result = await client.get_insights(days=7)

        actionable = [i for i in result.get("insights", [])
                      if i.get("actionable", False)]

        print(f"\n{len(actionable)} Actionable Insights:")

        for insight in actionable:
            print(f"\n{insight['title']}")
            print(f"  Severity: {insight['severity']}")
            print(f"  Confidence: {insight['confidence']:.0%}")
            print(f"  Actions:")
            for action in insight.get("recommended_actions", []):
                print(f"    [ ] {action}")

        # Also get dedicated recommendations endpoint
        print("\n--- Priority Recommendations ---")
        recs = await client.get_recommendations(priority="high", limit=5)

        for rec in recs.get("recommendations", []):
            print(f"\n[HIGH] {rec.get('title', 'Untitled')}")
            print(f"  Impact: {rec.get('impact', 'N/A')}")
            print(f"  Effort: {rec.get('effort', 'N/A')}")


async def analyze_trends():
    """Analyze trends from insights"""
    print("\n" + "=" * 60)
    print("Trend Analysis")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_insights(
            days=30,
            insight_types=["trend"]
        )

        trends = result.get("insights", [])

        # Categorize trends
        increasing = [t for t in trends if "increasing" in t.get("description", "").lower()]
        decreasing = [t for t in trends if "decreasing" in t.get("description", "").lower()]
        stable = [t for t in trends if t not in increasing and t not in decreasing]

        print(f"\nTrend Summary:")
        print(f"  Increasing trends: {len(increasing)}")
        print(f"  Decreasing trends: {len(decreasing)}")
        print(f"  Stable trends: {len(stable)}")

        if increasing:
            print(f"\n! Increasing Trends (Attention Needed):")
            for t in increasing[:5]:
                print(f"  - {t['title']}")

        if decreasing:
            print(f"\n Good News - Decreasing Trends:")
            for t in decreasing[:5]:
                print(f"  - {t['title']}")


async def analyze_correlations():
    """Analyze correlations from insights"""
    print("\n" + "=" * 60)
    print("Correlation Analysis")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_insights(
            days=7,
            insight_types=["correlation"]
        )

        correlations = result.get("insights", [])
        print(f"\nFound {len(correlations)} correlations")

        # Strong correlations (high confidence)
        strong = [c for c in correlations if c.get("confidence", 0) > 0.8]

        if strong:
            print(f"\nStrong Correlations (>80% confidence):")
            for corr in strong:
                print(f"\n  {corr['title']}")
                print(f"  {corr['description']}")
                print(f"  Confidence: {corr['confidence']:.0%}")


async def fleet_health_dashboard():
    """Generate fleet health dashboard data"""
    print("\n" + "=" * 60)
    print("Fleet Health Dashboard")
    print("=" * 60)
    print(f"Generated: {datetime.utcnow().isoformat()}Z")

    async with AnalyticsClient() as client:
        result = await client.get_insights(days=7)

        summary = result.get("fleet_summary", {})
        insights = result.get("insights", [])

        # Health score visualization
        health_score = summary.get("health_score", 0)
        bar_length = int(health_score / 5)
        health_bar = "" * bar_length + "" * (20 - bar_length)

        print(f"\n Health Score: [{health_bar}] {health_score:.0f}/100")

        # Status indicators
        print(f"\n--- Status Indicators ---")

        # Device status
        total = summary.get("total_devices", 0)
        active = summary.get("active_devices", 0)
        device_pct = (active / total * 100) if total > 0 else 0
        status = "OK" if device_pct > 90 else "WARN" if device_pct > 70 else "ALERT"
        print(f"[{status}] Devices Online: {active}/{total} ({device_pct:.0f}%)")

        # Anomaly rate
        anomaly_rate = summary.get("anomaly_rate", 0)
        status = "OK" if anomaly_rate < 0.05 else "WARN" if anomaly_rate < 0.1 else "ALERT"
        print(f"[{status}] Anomaly Rate: {anomaly_rate:.1%}")

        # Critical insights count
        critical = len([i for i in insights if i.get("severity") == "critical"])
        status = "OK" if critical == 0 else "ALERT"
        print(f"[{status}] Critical Issues: {critical}")

        # Key metrics
        print(f"\n--- Key Metrics ---")
        metrics = summary.get("metrics", {})
        for key, value in metrics.items():
            if isinstance(value, float):
                print(f"  {key}: {value:.2f}")
            else:
                print(f"  {key}: {value}")

        # Top priorities
        print(f"\n--- Top Priorities ---")
        critical_insights = [i for i in insights if i.get("severity") == "critical"][:3]
        warning_insights = [i for i in insights if i.get("severity") == "warning"][:3]

        for i, insight in enumerate(critical_insights + warning_insights, 1):
            print(f"{i}. [{insight['severity'].upper()}] {insight['title']}")


async def insights_report():
    """Generate comprehensive insights report"""
    print("\n" + "=" * 60)
    print("AI Insights Report")
    print("=" * 60)
    print(f"Generated: {datetime.utcnow().isoformat()}Z")
    print(f"Analysis Period: Last 7 days")

    async with AnalyticsClient() as client:
        result = await client.get_insights(days=7)

        summary = result.get("fleet_summary", {})
        insights = result.get("insights", [])

        print("\n" + "=" * 60)
        print("EXECUTIVE SUMMARY")
        print("=" * 60)

        health = summary.get("health_score", 0)
        print(f"Fleet Health: {health:.0f}/100 ", end="")
        if health >= 90:
            print("(Excellent)")
        elif health >= 70:
            print("(Good)")
        elif health >= 50:
            print("(Fair)")
        else:
            print("(Needs Attention)")

        print(f"Total Insights: {len(insights)}")

        # Count by severity
        by_severity = {}
        for i in insights:
            sev = i.get("severity", "info")
            by_severity[sev] = by_severity.get(sev, 0) + 1

        for sev in ["critical", "warning", "info"]:
            if sev in by_severity:
                print(f"  {sev.capitalize()}: {by_severity[sev]}")

        # Count by type
        print("\nInsights by Type:")
        by_type = {}
        for i in insights:
            t = i.get("type", "other")
            by_type[t] = by_type.get(t, 0) + 1

        for t, count in sorted(by_type.items(), key=lambda x: x[1], reverse=True):
            print(f"  {t}: {count}")

        print("\n" + "=" * 60)
        print("DETAILED FINDINGS")
        print("=" * 60)

        # Group insights by type
        for insight_type in ["anomaly_pattern", "trend", "correlation", "recommendation"]:
            type_insights = [i for i in insights if i.get("type") == insight_type]
            if type_insights:
                print(f"\n--- {insight_type.replace('_', ' ').title()} ---")
                for i in type_insights[:5]:
                    print(f"\n  {i['title']}")
                    print(f"  Severity: {i['severity']} | Confidence: {i['confidence']:.0%}")
                    print(f"  {i['description'][:200]}...")

        print("\n" + "=" * 60)
        print("ACTION ITEMS")
        print("=" * 60)

        actionable = [i for i in insights if i.get("actionable")]
        actionable.sort(key=lambda x: (
            {"critical": 0, "warning": 1, "info": 2}.get(x.get("severity"), 3),
            -x.get("confidence", 0)
        ))

        for i, insight in enumerate(actionable[:10], 1):
            print(f"\n{i}. {insight['title']}")
            print(f"   Priority: {insight['severity'].upper()}")
            for action in insight.get("recommended_actions", [])[:2]:
                print(f"   - {action}")


async def main():
    """Run all insights examples"""
    print("=" * 60)
    print("TESAIoT Analytics API - AI Insights Examples")
    print("=" * 60)

    await get_all_insights()
    await get_insights_by_type()
    await get_high_confidence_insights()
    await get_actionable_recommendations()
    await analyze_trends()
    await analyze_correlations()
    await fleet_health_dashboard()
    await insights_report()

    print("\n" + "=" * 60)
    print("All examples completed!")


if __name__ == "__main__":
    asyncio.run(main())
