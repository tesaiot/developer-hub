//! Complete Dashboard Example for TESAIoT Analytics API (Rust)
//!
//! This example demonstrates building a comprehensive IoT analytics
//! dashboard that combines all four analytics domains.
//!
//! Run with: cargo run --example dashboard

use std::collections::HashMap;
use tesaiot_analytics::{
    AnalyticsClient, AnomaliesResponse, ClustersResponse, ConnectivityResponse,
    InsightsResponse, LatencyResponse, QualityResponse, TimeRange, ThroughputResponse,
};

/// Dashboard data container
struct DashboardData {
    timestamp: String,
    fleet_health: FleetHealth,
    anomalies: AnomaliesResponse,
    clusters: ClustersResponse,
    insights: InsightsResponse,
    connectivity: ConnectivityResponse,
    latency: LatencyResponse,
    throughput: ThroughputResponse,
    quality: QualityResponse,
    alerts: Vec<Alert>,
}

struct FleetHealth {
    overall_score: f64,
    component_scores: HashMap<String, f64>,
    status: String,
}

struct Alert {
    level: String,
    alert_type: String,
    title: String,
    description: String,
}

/// Collect all dashboard data
async fn collect_dashboard_data(client: &AnalyticsClient) -> Result<DashboardData, Box<dyn std::error::Error>> {
    // Collect all data concurrently
    let (anomalies, clusters, insights, connectivity, latency, throughput, quality) = tokio::try_join!(
        client.get_anomalies(None, Some(vec!["critical", "high", "medium"]), None, 100, 0),
        client.get_clusters("temperature", 5, None, true),
        client.get_insights(7, None, 0.7),
        client.get_connectivity_status(None),
        client.get_latency_stats(24),
        client.get_throughput_stats(24),
        client.get_connection_quality()
    )?;

    let alerts = generate_alerts(&anomalies, &connectivity, &latency, &quality);
    let fleet_health = calculate_fleet_health(&anomalies, &connectivity, &insights, &latency);

    Ok(DashboardData {
        timestamp: chrono::Utc::now().to_rfc3339(),
        fleet_health,
        anomalies,
        clusters,
        insights,
        connectivity,
        latency,
        throughput,
        quality,
        alerts,
    })
}

/// Calculate overall fleet health score
fn calculate_fleet_health(
    anomalies: &AnomaliesResponse,
    connectivity: &ConnectivityResponse,
    insights: &InsightsResponse,
    latency: &LatencyResponse,
) -> FleetHealth {
    let mut scores = HashMap::new();

    // Anomaly score
    let total_devices = connectivity.summary.total.max(1);
    let anomaly_rate = anomalies.summary.total as f64 / total_devices as f64;
    scores.insert("anomaly".to_string(), (100.0 - anomaly_rate * 1000.0).max(0.0));

    // Connectivity score
    let online_pct = connectivity.summary.online as f64 / total_devices as f64 * 100.0;
    scores.insert("connectivity".to_string(), online_pct);

    // Latency score
    let latency_score = (100.0 - latency.summary.p95_latency_ms / 10.0).max(0.0);
    scores.insert("latency".to_string(), latency_score);

    // Insights severity score
    let critical_count = insights.insights.iter()
        .filter(|i| i.severity == "critical")
        .count();
    let warning_count = insights.insights.iter()
        .filter(|i| i.severity == "warning")
        .count();
    let insights_score = (100.0 - (critical_count as f64 * 20.0) - (warning_count as f64 * 5.0)).max(0.0);
    scores.insert("insights".to_string(), insights_score);

    // Calculate weighted overall
    let overall = scores["anomaly"] * 0.3
        + scores["connectivity"] * 0.3
        + scores["latency"] * 0.2
        + scores["insights"] * 0.2;

    let status = if overall >= 90.0 {
        "EXCELLENT"
    } else if overall >= 70.0 {
        "GOOD"
    } else if overall >= 50.0 {
        "FAIR"
    } else if overall >= 30.0 {
        "POOR"
    } else {
        "CRITICAL"
    };

    FleetHealth {
        overall_score: (overall * 10.0).round() / 10.0,
        component_scores: scores,
        status: status.to_string(),
    }
}

/// Generate alerts based on current data
fn generate_alerts(
    anomalies: &AnomaliesResponse,
    connectivity: &ConnectivityResponse,
    latency: &LatencyResponse,
    quality: &QualityResponse,
) -> Vec<Alert> {
    let mut alerts = Vec::new();

    // Critical anomaly alerts
    let critical_count = anomalies.summary.by_severity.get("critical").unwrap_or(&0);
    if *critical_count > 0 {
        alerts.push(Alert {
            level: "critical".to_string(),
            alert_type: "anomaly".to_string(),
            title: format!("{} Critical Anomalies Detected", critical_count),
            description: "Immediate investigation recommended".to_string(),
        });
    }

    // Offline device alerts
    let offline_pct = connectivity.summary.offline as f64
        / connectivity.summary.total.max(1) as f64
        * 100.0;

    if offline_pct > 20.0 {
        alerts.push(Alert {
            level: "critical".to_string(),
            alert_type: "connectivity".to_string(),
            title: format!(
                "{} Devices Offline ({:.0}%)",
                connectivity.summary.offline, offline_pct
            ),
            description: "Network connectivity issue detected".to_string(),
        });
    } else if connectivity.summary.offline > 0 {
        alerts.push(Alert {
            level: "warning".to_string(),
            alert_type: "connectivity".to_string(),
            title: format!("{} Device(s) Offline", connectivity.summary.offline),
            description: "Some devices are not responding".to_string(),
        });
    }

    // Latency alerts
    if latency.summary.p95_latency_ms > 1000.0 {
        alerts.push(Alert {
            level: "critical".to_string(),
            alert_type: "latency".to_string(),
            title: format!("High Latency: {:.0}ms P95", latency.summary.p95_latency_ms),
            description: "Network performance severely degraded".to_string(),
        });
    } else if latency.summary.p95_latency_ms > 500.0 {
        alerts.push(Alert {
            level: "warning".to_string(),
            alert_type: "latency".to_string(),
            title: format!("Elevated Latency: {:.0}ms P95", latency.summary.p95_latency_ms),
            description: "Network performance degraded".to_string(),
        });
    }

    // Quality alerts
    if quality.summary.distribution.poor > 5 {
        alerts.push(Alert {
            level: "warning".to_string(),
            alert_type: "quality".to_string(),
            title: format!(
                "{} Devices with Poor Connection Quality",
                quality.summary.distribution.poor
            ),
            description: "Review device connections and network path".to_string(),
        });
    }

    alerts
}

/// Render dashboard to console
fn render_dashboard(data: &DashboardData) {
    println!("\n{}", "=".repeat(80));
    println!("{:^80}", "TESAIoT ANALYTICS DASHBOARD");
    println!("{}", "=".repeat(80));
    println!("Generated: {}", data.timestamp);

    // Fleet Health
    println!("\n{}", "-".repeat(80));
    println!(" FLEET HEALTH");
    println!("{}", "-".repeat(80));

    let bar_len = (data.fleet_health.overall_score / 5.0) as usize;
    let bar: String = "\u{2588}".repeat(bar_len) + &"\u{2591}".repeat(20 - bar_len);
    println!(
        "\n  Overall: [{}] {}/100 ({})",
        bar, data.fleet_health.overall_score, data.fleet_health.status
    );

    println!("\n  Component Scores:");
    for (component, score) in &data.fleet_health.component_scores {
        println!("    {:15} {:.0}/100", component, score);
    }

    // Alerts
    if !data.alerts.is_empty() {
        println!("\n{}", "-".repeat(80));
        println!(" ACTIVE ALERTS");
        println!("{}", "-".repeat(80));

        for alert in &data.alerts {
            let icon = if alert.level == "critical" { "\u{2757}" } else { "\u{26A0}" };
            println!("\n  [{}] {}", icon, alert.title);
            println!("      {}", alert.description);
        }
    }

    // Anomalies
    println!("\n{}", "-".repeat(80));
    println!(" ANOMALY DETECTION");
    println!("{}", "-".repeat(80));

    println!("\n  Total Anomalies (7 days): {}", data.anomalies.summary.total);
    println!("  By Severity:");
    for (sev, count) in &data.anomalies.summary.by_severity {
        println!("    {:10} {}", sev, count);
    }

    // Clusters
    println!("\n{}", "-".repeat(80));
    println!(" PATTERN RECOGNITION");
    println!("{}", "-".repeat(80));

    println!("\n  Clusters: {}", data.clusters.clusters.len());
    println!("  Silhouette Score: {:.3}", data.clusters.silhouette_score);

    for cluster in &data.clusters.clusters {
        println!(
            "\n    Cluster {}: {} devices",
            cluster.cluster_id, cluster.device_count
        );
    }

    if !data.clusters.outliers.is_empty() {
        println!("\n  Outliers: {} devices", data.clusters.outliers.len());
    }

    // Insights
    println!("\n{}", "-".repeat(80));
    println!(" AI INSIGHTS");
    println!("{}", "-".repeat(80));

    println!("\n  Total Insights: {}", data.insights.insights.len());

    let critical_insights: Vec<_> = data.insights.insights.iter()
        .filter(|i| i.severity == "critical")
        .collect();

    if !critical_insights.is_empty() {
        println!("\n  Critical Insights:");
        for insight in critical_insights.iter().take(3) {
            println!("    \u{2757} {}", insight.title);
        }
    }

    let actionable: Vec<_> = data.insights.insights.iter()
        .filter(|i| i.actionable)
        .collect();

    if !actionable.is_empty() {
        println!("\n  Actionable Recommendations ({}):", actionable.len());
        for insight in actionable.iter().take(3) {
            println!("    - {}", insight.title);
        }
    }

    // Connectivity
    println!("\n{}", "-".repeat(80));
    println!(" CONNECTIVITY");
    println!("{}", "-".repeat(80));

    println!(
        "\n  Devices: {}/{} online",
        data.connectivity.summary.online, data.connectivity.summary.total
    );

    println!("\n  Latency:");
    println!("    Average: {:.1} ms", data.latency.summary.avg_latency_ms);
    println!("    P95: {:.1} ms", data.latency.summary.p95_latency_ms);

    println!("\n  Throughput (24h):");
    println!("    Total Messages: {}", data.throughput.summary.total_messages);
    println!("    Avg/Hour: {:.0}", data.throughput.summary.avg_per_hour);

    println!(
        "\n  Connection Quality: {:.0}/100",
        data.quality.summary.average_quality_score
    );

    // Footer
    println!("\n{}", "=".repeat(80));
    println!("{:^80}", "END OF REPORT");
    println!("{}", "=".repeat(80));
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();

    if args.iter().any(|a| a == "--help") {
        println!("TESAIoT Analytics Dashboard");
        println!("\nUsage:");
        println!("  cargo run --example dashboard          # Single refresh");
        println!("  cargo run --example dashboard -- --loop  # Continuous (60s)");
        return Ok(());
    }

    let client = AnalyticsClient::from_env()?;

    if args.iter().any(|a| a == "--loop") {
        println!("Starting dashboard refresh loop...");
        println!("Refresh interval: 60 seconds");
        println!("Press Ctrl+C to stop\n");

        for i in 0..5 {
            let data = collect_dashboard_data(&client).await?;
            render_dashboard(&data);
            println!(
                "\n[Refresh {}/5 - Next refresh in 60 seconds]",
                i + 1
            );
            tokio::time::sleep(std::time::Duration::from_secs(60)).await;
        }
    } else {
        println!("Collecting dashboard data...");
        let data = collect_dashboard_data(&client).await?;
        render_dashboard(&data);
    }

    Ok(())
}
