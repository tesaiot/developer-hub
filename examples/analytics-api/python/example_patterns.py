"""
Pattern Recognition Examples for TESAIoT Analytics API

This example demonstrates:
- K-means clustering of device behavior
- Cluster quality metrics (silhouette score)
- Outlier detection
- Finding similar devices
- Feature analysis

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import os
import json
from datetime import datetime
from analytics_client import AnalyticsClient, TimeRange


async def basic_clustering():
    """Basic K-means clustering example"""
    print("\n" + "=" * 60)
    print("Basic K-means Clustering")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_clusters(
            metric_name="temperature",
            n_clusters=5
        )

        print(f"\nClustering Results:")
        print(f"  Metric: temperature")
        print(f"  Silhouette Score: {result.get('silhouette_score', 'N/A'):.3f}")
        print(f"  (Score > 0.5 indicates good cluster separation)")

        clusters = result.get("clusters", [])
        print(f"\n{len(clusters)} Clusters Found:")

        for cluster in clusters:
            print(f"\n  Cluster {cluster['cluster_id']}: {cluster.get('cluster_name', 'Unnamed')}")
            print(f"    Devices: {cluster['device_count']}")
            print(f"    Characteristics:")

            chars = cluster.get("characteristics", {})
            for key, value in chars.items():
                if isinstance(value, float):
                    print(f"      {key}: {value:.2f}")
                else:
                    print(f"      {key}: {value}")


async def clustering_multiple_metrics():
    """Cluster devices by different metrics"""
    print("\n" + "=" * 60)
    print("Clustering by Multiple Metrics")
    print("=" * 60)

    metrics = ["temperature", "humidity", "power_consumption", "cpu_usage"]

    async with AnalyticsClient() as client:
        for metric in metrics:
            try:
                result = await client.get_clusters(
                    metric_name=metric,
                    n_clusters=3
                )

                score = result.get("silhouette_score", 0)
                n_clusters = len(result.get("clusters", []))

                quality = "Excellent" if score > 0.7 else "Good" if score > 0.5 else "Fair"

                print(f"\n{metric.upper()}:")
                print(f"  Silhouette: {score:.3f} ({quality})")
                print(f"  Clusters: {n_clusters}")

            except Exception as e:
                print(f"\n{metric.upper()}: Error - {e}")


async def outlier_detection():
    """Detect outlier devices that don't fit patterns"""
    print("\n" + "=" * 60)
    print("Outlier Detection")
    print("=" * 60)

    async with AnalyticsClient() as client:
        # Get clusters with outliers
        result = await client.get_clusters(
            metric_name="temperature",
            n_clusters=5,
            include_outliers=True
        )

        outliers = result.get("outliers", [])
        print(f"\nFound {len(outliers)} outlier devices")

        if outliers:
            print("\nOutlier Devices:")
            for outlier in outliers[:10]:
                print(f"  {outlier['device_id']}")
                print(f"    Outlier Score: {outlier['outlier_score']:.2f}")
                print(f"    Reason: {outlier.get('reason', 'N/A')}")

        # Also get dedicated outliers endpoint
        print("\n--- Dedicated Outlier Analysis ---")
        outlier_result = await client.get_outliers(
            metric_name="temperature",
            threshold=0.7
        )

        outlier_devices = outlier_result.get("outliers", [])
        print(f"Outliers (threshold 0.7): {len(outlier_devices)} devices")


async def find_similar_devices():
    """Find devices with similar behavior patterns"""
    print("\n" + "=" * 60)
    print("Find Similar Devices")
    print("=" * 60)

    # Example: Find devices similar to a specific device
    target_device = "device-001"  # Replace with actual device ID

    async with AnalyticsClient() as client:
        result = await client.find_similar_devices(
            device_id=target_device,
            metric_name="temperature",
            top_k=10
        )

        print(f"\nDevices similar to: {target_device}")

        # Source device features
        source = result.get("source_device", {})
        print(f"\nSource Device Features:")
        features = source.get("features", {})
        for key, value in features.items():
            if isinstance(value, float):
                print(f"  {key}: {value:.3f}")
            else:
                print(f"  {key}: {value}")

        # Similar devices
        similar = result.get("similar_devices", [])
        print(f"\nTop {len(similar)} Similar Devices:")

        for device in similar:
            print(f"\n  {device['device_id']}")
            print(f"    Similarity: {device['similarity_score']:.2%}")
            print(f"    Cluster: {device.get('cluster_id', 'N/A')}")


async def cluster_characteristics_analysis():
    """Deep analysis of cluster characteristics"""
    print("\n" + "=" * 60)
    print("Cluster Characteristics Analysis")
    print("=" * 60)

    async with AnalyticsClient() as client:
        result = await client.get_clusters(
            metric_name="temperature",
            n_clusters=5
        )

        clusters = result.get("clusters", [])

        # Feature comparison across clusters
        print("\nFeature Comparison Across Clusters:")
        print("-" * 80)
        print(f"{'Cluster':<12} {'Mean':>10} {'Std Dev':>10} {'Trend':>10} {'Entropy':>10}")
        print("-" * 80)

        for cluster in clusters:
            chars = cluster.get("characteristics", {})
            print(f"{cluster['cluster_id']:<12} "
                  f"{chars.get('mean', 0):>10.2f} "
                  f"{chars.get('std_dev', 0):>10.2f} "
                  f"{chars.get('trend_slope', 0):>10.3f} "
                  f"{chars.get('entropy', 0):>10.3f}")

        print("-" * 80)

        # Identify cluster types
        print("\nCluster Type Identification:")
        for cluster in clusters:
            chars = cluster.get("characteristics", {})
            cluster_type = identify_cluster_type(chars)
            print(f"  Cluster {cluster['cluster_id']}: {cluster_type}")


def identify_cluster_type(characteristics):
    """Identify cluster type based on characteristics"""
    mean = characteristics.get("mean", 0)
    std_dev = characteristics.get("std_dev", 0)
    trend = characteristics.get("trend_slope", 0)
    entropy = characteristics.get("entropy", 0)

    types = []

    # Stability analysis
    cv = std_dev / mean if mean != 0 else 0  # Coefficient of variation
    if cv < 0.1:
        types.append("Stable")
    elif cv > 0.3:
        types.append("Volatile")

    # Trend analysis
    if trend > 0.01:
        types.append("Increasing")
    elif trend < -0.01:
        types.append("Decreasing")

    # Entropy analysis
    if entropy > 0.8:
        types.append("High-entropy (random)")
    elif entropy < 0.3:
        types.append("Low-entropy (predictable)")

    return ", ".join(types) if types else "Normal"


async def time_based_clustering():
    """Cluster analysis over different time periods"""
    print("\n" + "=" * 60)
    print("Time-Based Clustering Analysis")
    print("=" * 60)

    time_periods = [
        ("Last 24 hours", 1),
        ("Last 7 days", 7),
        ("Last 30 days", 30)
    ]

    async with AnalyticsClient() as client:
        for period_name, days in time_periods:
            time_range = TimeRange.last_days(days)

            result = await client.get_clusters(
                metric_name="temperature",
                n_clusters=5,
                start_time=time_range.start,
                end_time=time_range.end
            )

            score = result.get("silhouette_score", 0)
            clusters = result.get("clusters", [])

            print(f"\n{period_name}:")
            print(f"  Silhouette Score: {score:.3f}")
            print(f"  Cluster Distribution:")

            for cluster in clusters:
                print(f"    Cluster {cluster['cluster_id']}: {cluster['device_count']} devices")


async def pattern_report():
    """Generate comprehensive pattern recognition report"""
    print("\n" + "=" * 60)
    print("Pattern Recognition Report")
    print("=" * 60)
    print(f"Generated: {datetime.utcnow().isoformat()}Z")

    async with AnalyticsClient() as client:
        # Analyze temperature patterns
        result = await client.get_clusters(
            metric_name="temperature",
            n_clusters=5,
            include_outliers=True
        )

        print("\n--- Clustering Quality ---")
        score = result.get("silhouette_score", 0)
        print(f"Silhouette Score: {score:.3f}")

        if score >= 0.7:
            print("Interpretation: Excellent cluster separation")
        elif score >= 0.5:
            print("Interpretation: Good cluster separation")
        elif score >= 0.3:
            print("Interpretation: Fair cluster separation")
        else:
            print("Interpretation: Poor clustering - consider different parameters")

        print("\n--- Cluster Summary ---")
        clusters = result.get("clusters", [])
        total_devices = sum(c.get("device_count", 0) for c in clusters)

        for cluster in clusters:
            count = cluster.get("device_count", 0)
            pct = (count / total_devices * 100) if total_devices > 0 else 0
            print(f"Cluster {cluster['cluster_id']}: {count} devices ({pct:.1f}%)")

        print("\n--- Outlier Analysis ---")
        outliers = result.get("outliers", [])
        outlier_pct = (len(outliers) / total_devices * 100) if total_devices > 0 else 0

        print(f"Outlier devices: {len(outliers)} ({outlier_pct:.1f}%)")

        if outlier_pct > 10:
            print("WARNING: High outlier percentage - investigate unusual devices")

        print("\n--- Recommendations ---")
        if score < 0.5:
            print("- Consider adjusting n_clusters parameter")
            print("- Try different metrics for clustering")
        if len(outliers) > 5:
            print("- Review outlier devices for potential issues")
        if any(c.get("device_count", 0) == 1 for c in clusters):
            print("- Some clusters have only 1 device - may indicate anomalies")


async def main():
    """Run all pattern recognition examples"""
    print("=" * 60)
    print("TESAIoT Analytics API - Pattern Recognition Examples")
    print("=" * 60)

    await basic_clustering()
    await clustering_multiple_metrics()
    await outlier_detection()
    await find_similar_devices()
    await cluster_characteristics_analysis()
    await time_based_clustering()
    await pattern_report()

    print("\n" + "=" * 60)
    print("All examples completed!")


if __name__ == "__main__":
    asyncio.run(main())
