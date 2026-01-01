# Rust Analytics API Client

A type-safe Rust client for the TESAIoT BDH AI Analytics API.

## Features

- Async/await with tokio
- Type-safe request/response models
- Comprehensive error handling
- Connection pooling
- Automatic retries

## Installation

Add to your `Cargo.toml`:

```toml
[dependencies]
tesaiot-analytics = { path = "." }
tokio = { version = "1", features = ["full"] }
```

## Quick Start

```rust
use tesaiot_analytics::{AnalyticsClient, TimeRange};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let client = AnalyticsClient::new(
        "https://admin.tesaiot.com/api/v1/bdh-ai",
        "your_jwt_token"
    )?;

    // Get anomalies
    let anomalies = client.get_anomalies(
        Some(TimeRange::last_days(7)),
        Some(vec!["critical", "high"]),
        None,
        100,
        0
    ).await?;

    // Get device clusters
    let clusters = client.get_clusters(
        "temperature",
        5,
        None,
        true
    ).await?;

    // Get AI insights
    let insights = client.get_insights(7, None, 0.7).await?;

    // Get connectivity status
    let status = client.get_connectivity_status(None).await?;

    Ok(())
}
```

## Examples

| File | Description |
|------|-------------|
| `src/lib.rs` | Main client library |
| `examples/basic.rs` | Basic usage examples |
| `examples/dashboard.rs` | Complete dashboard example |

## Configuration

Set environment variables:

```bash
export TESAIOT_API_URL="https://admin.tesaiot.com/api/v1/bdh-ai"
export TESAIOT_API_TOKEN="your_jwt_token"
```

## Building

```bash
cargo build
cargo run --example basic
cargo run --example dashboard
```

## License

Apache 2.0
