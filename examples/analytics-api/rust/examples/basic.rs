//! Basic Usage Example for TESAIoT Analytics API (Rust)
//!
//! This example demonstrates:
//! - Client initialization
//! - Making API requests
//! - Handling responses
//! - Error handling
//!
//! Run with: cargo run --example basic

use tesaiot_analytics::{AnalyticsClient, TimeRange};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("{}", "=".repeat(60));
    println!("TESAIoT Analytics API - Basic Example (Rust)");
    println!("{}", "=".repeat(60));

    // Initialize client from environment
    let client = AnalyticsClient::from_env()?;

    // 1. Get recent anomalies
    println!("\n1. Fetching recent anomalies...");
    match client
        .get_anomalies(
            None,
            None, // Don't filter by severity - get all
            None,
            5,
            0,
        )
        .await
    {
        Ok(result) => {
            println!("   Found {} anomalies", result.anomalies.len());
            for anomaly in result.anomalies.iter().take(3) {
                println!(
                    "   - {}: {} ({})",
                    anomaly.device_name, anomaly.metric, anomaly.severity
                );
            }
        }
        Err(e) => println!("   Error: {}", e),
    }

    // 2. Get latency statistics
    println!("\n2. Fetching latency statistics...");
    match client.get_latency_stats(24).await {
        Ok(result) => {
            println!("   Average latency: {} ms", result.summary.overall_avg_ms);
            println!("   P95 latency: {} ms", result.summary.overall_p95_ms);
        }
        Err(e) => println!("   Error: {}", e),
    }

    // 3. Get throughput statistics
    println!("\n3. Fetching throughput statistics...");
    match client.get_throughput_stats(24).await {
        Ok(result) => {
            println!("   Total messages in: {}", result.summary.total_messages_in);
            println!("   Avg per minute: {:.2}", result.summary.avg_messages_per_minute);
        }
        Err(e) => println!("   Error: {}", e),
    }

    // 4. Get connectivity status
    println!("\n4. Fetching connectivity status...");
    match client.get_connectivity_status(None).await {
        Ok(result) => {
            println!("   Total devices: {}", result.summary.total_devices);
            println!("   Online: {}", result.summary.online_count);
            println!("   Offline: {}", result.summary.offline_count);
        }
        Err(e) => println!("   Error: {}", e),
    }

    println!("\n{}", "=".repeat(60));
    println!("Done!");

    Ok(())
}

/// Example with custom time range
#[allow(dead_code)]
async fn custom_time_range_example(client: &AnalyticsClient) -> Result<(), Box<dyn std::error::Error>> {
    println!("\nCustom Time Range Example:");

    // Last 24 hours
    let time_range = TimeRange::last_hours(24);
    println!("  Last 24 hours:");
    println!("    Start: {}", time_range.start);
    println!("    End: {}", time_range.end);

    // Last 30 days
    let time_range = TimeRange::last_days(30);
    println!("  Last 30 days:");
    println!("    Start: {}", time_range.start);
    println!("    End: {}", time_range.end);

    // Get anomalies with custom time range
    let anomalies = client
        .get_anomalies(
            Some(TimeRange::last_days(7)),
            None,
            None,
            10,
            0,
        )
        .await?;

    println!("  Anomalies in last 7 days: {}", anomalies.summary.total);

    Ok(())
}
