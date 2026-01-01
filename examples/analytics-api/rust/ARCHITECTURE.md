# Analytics API Rust Client - Architecture

## Overview

High-performance Rust client for TESAIoT Analytics API. Provides type-safe access to anomaly detection, connectivity status, latency statistics, and throughput metrics.

## Architecture Diagram

```ini
┌─────────────────────────────────────────────────────────────────┐
│                      Rust Application                           │
├─────────────────────────────────────────────────────────────────┤
│  src/main.rs                                                    │
│  ├── AnalyticsClient struct                                     │
│  │   ├── get_anomalies() -> Result<Vec<Anomaly>>                │
│  │   ├── get_connectivity_status() -> Result<ConnectivityStatus>│
│  │   ├── get_latency_stats() -> Result<LatencyStats>            │
│  │   ├── get_throughput_stats() -> Result<ThroughputStats>      │
│  │   └── get_fleet_anomalies() -> Result<FleetAnomalies>        │
│  └── Strongly typed response structs (serde)                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS (TLS 1.2+)
                              │ X-API-KEY header
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TESAIoT Platform                             │
├─────────────────────────────────────────────────────────────────┤
│  /api/v1/bdh-ai/                                                │
│  ├── anomalies              → Anomaly detection results         │
│  ├── connectivity/status    → Device online/offline status      │
│  ├── connectivity/latency   → Network latency statistics        │
│  ├── connectivity/throughput→ Message throughput metrics        │
│  └── analytics/anomalies    → Fleet-wide anomaly overview       │
└─────────────────────────────────────────────────────────────────┘
```

## Type System

```rust
// Strongly typed API responses
#[derive(Deserialize)]
struct Anomaly {
    device_id: String,
    timestamp: DateTime<Utc>,
    anomaly_type: String,
    confidence: f64,
}

#[derive(Deserialize)]
struct LatencyStats {
    avg_latency_ms: f64,
    p95_latency_ms: f64,
    p99_latency_ms: f64,
}
```

## Workflow

```ini
┌────────────┐     ┌────────────┐     ┌────────────────┐
│ Build with │────►│ Load env   │────►│ Create client  │
│ cargo build│     │ API_KEY    │     │ with reqwest   │
└────────────┘     └────────────┘     └───────┬────────┘
                                              │
                   ┌──────────────────────────┘
                   ▼
┌────────────────────────────────────────────────────┐
│          Async API Calls (tokio runtime)           │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌──────────┐  │
│  │get_     │ │get_     │ │get_     │ │get_fleet_│  │
│  │anomalies│ │latency  │ │through- │ │anomalies │  │
│  │         │ │_stats   │ │put_stats│ │          │  │
│  └─────────┘ └─────────┘ └─────────┘ └──────────┘  │
└────────────────────────────────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────────────────┐
│    Deserialize JSON → Rust structs (serde_json)    │
└────────────────────────────────────────────────────┘
```

## Error Handling

```rust
// Result-based error handling
match client.get_anomalies().await {
    Ok(anomalies) => process(anomalies),
    Err(e) => eprintln!("API error: {}", e),
}
```

## Dependencies (Cargo.toml)

```toml
[dependencies]
reqwest = { version = "0.11", features = ["json"] }
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
dotenv = "0.15"
```

## Files

```ini
analytics-api/rust/
├── Cargo.toml           # Rust dependencies
├── src/
│   ├── main.rs          # Entry point and examples
│   ├── client.rs        # AnalyticsClient implementation
│   └── models.rs        # Response type definitions
├── .env.example         # Environment template
├── README.md            # Usage documentation
└── ARCHITECTURE.md      # This file
```

## Performance Benefits

- **Zero-cost abstractions**: No runtime overhead
- **Compile-time safety**: Type errors caught at build time
- **Async/await**: Non-blocking I/O with tokio
- **Memory safety**: No null pointer exceptions
