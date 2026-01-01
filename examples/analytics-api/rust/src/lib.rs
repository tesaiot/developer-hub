//! TESAIoT BDH AI Analytics API Client
//!
//! A type-safe Rust client for the TESAIoT Analytics API.
//!
//! # Usage
//!
//! ```rust
//! use tesaiot_analytics::{AnalyticsClient, TimeRange};
//!
//! #[tokio::main]
//! async fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     let client = AnalyticsClient::new(
//!         "https://admin.tesaiot.com/api/v1/bdh-ai",
//!         "your_jwt_token"
//!     )?;
//!
//!     let anomalies = client.get_anomalies(None, None, None, 100, 0).await?;
//!     println!("Found {} anomalies", anomalies.summary.total);
//!
//!     Ok(())
//! }
//! ```

use chrono::{DateTime, Duration, Utc};
use reqwest::Client;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use thiserror::Error;

// ============================================================
// Error Types
// ============================================================

#[derive(Error, Debug)]
pub enum AnalyticsError {
    #[error("HTTP error: {0}")]
    Http(#[from] reqwest::Error),

    #[error("API error {status}: {message}")]
    Api { status: u16, message: String },

    #[error("Configuration error: {0}")]
    Config(String),

    #[error("Serialization error: {0}")]
    Serialization(#[from] serde_json::Error),
}

pub type Result<T> = std::result::Result<T, AnalyticsError>;

// ============================================================
// Time Range
// ============================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TimeRange {
    pub start: String,
    pub end: String,
}

impl TimeRange {
    /// Create time range for last N days
    pub fn last_days(days: i64) -> Self {
        let end = Utc::now();
        let start = end - Duration::days(days);
        Self {
            start: start.to_rfc3339(),
            end: end.to_rfc3339(),
        }
    }

    /// Create time range for last N hours
    pub fn last_hours(hours: i64) -> Self {
        let end = Utc::now();
        let start = end - Duration::hours(hours);
        Self {
            start: start.to_rfc3339(),
            end: end.to_rfc3339(),
        }
    }

    /// Create custom time range
    pub fn custom(start: DateTime<Utc>, end: DateTime<Utc>) -> Self {
        Self {
            start: start.to_rfc3339(),
            end: end.to_rfc3339(),
        }
    }
}

// ============================================================
// Data Models
// ============================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Anomaly {
    pub id: String,
    pub device_id: String,
    pub device_name: String,
    pub metric: String,
    pub value: f64,
    pub severity: String,
    pub score: f64,
    pub timestamp: String,
    #[serde(default)]
    pub acknowledged: bool,
    #[serde(default)]
    pub resolved: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AnomalySummary {
    pub total: i64,
    #[serde(default)]
    pub by_severity: HashMap<String, i64>,
    #[serde(default)]
    pub by_metric: HashMap<String, i64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AnomaliesResponse {
    pub anomalies: Vec<Anomaly>,
    pub summary: AnomalySummary,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TimelinePoint {
    pub date: String,
    pub counts: HashMap<String, i64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Trend {
    pub direction: String,
    pub percent_change: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TimelineResponse {
    pub timeline: Vec<TimelinePoint>,
    pub trend: Trend,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Cluster {
    pub cluster_id: i32,
    pub cluster_name: String,
    pub device_count: i64,
    pub characteristics: HashMap<String, f64>,
    pub devices: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Outlier {
    pub device_id: String,
    pub outlier_score: f64,
    pub reason: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClustersResponse {
    pub clusters: Vec<Cluster>,
    pub silhouette_score: f64,
    #[serde(default)]
    pub outliers: Vec<Outlier>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Insight {
    pub id: String,
    #[serde(rename = "type")]
    pub insight_type: String,
    pub severity: String,
    pub title: String,
    pub description: String,
    pub confidence: f64,
    pub actionable: bool,
    #[serde(default)]
    pub recommended_actions: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FleetSummary {
    pub total_devices: i64,
    pub active_devices: i64,
    pub anomaly_rate: f64,
    pub health_score: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InsightsResponse {
    pub insights: Vec<Insight>,
    pub fleet_summary: FleetSummary,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DeviceStatus {
    pub device_id: String,
    pub device_name: String,
    pub status: String,
    pub last_seen: String,
    pub uptime_percent: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ConnectivitySummary {
    #[serde(alias = "total", alias = "total_devices")]
    pub total_devices: i64,
    #[serde(alias = "online", alias = "online_count")]
    pub online_count: i64,
    #[serde(alias = "offline", alias = "offline_count")]
    pub offline_count: i64,
    #[serde(default)]
    pub unknown_count: i64,
    #[serde(default)]
    pub online_percentage: f64,
    #[serde(default)]
    pub avg_connection_duration_hours: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ConnectivityResponse {
    pub devices: Vec<DeviceStatus>,
    pub summary: ConnectivitySummary,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LatencySummary {
    #[serde(alias = "avg_latency_ms", alias = "overall_avg_ms")]
    pub overall_avg_ms: f64,
    #[serde(alias = "p95_latency_ms", alias = "overall_p95_ms")]
    pub overall_p95_ms: f64,
    #[serde(alias = "p99_latency_ms", alias = "overall_p99_ms")]
    pub overall_p99_ms: f64,
    #[serde(default)]
    pub devices_with_high_latency: i64,
    #[serde(default)]
    pub high_latency_threshold_ms: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LatencyResponse {
    pub summary: LatencySummary,
    #[serde(default)]
    pub devices: Vec<HashMap<String, serde_json::Value>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThroughputSummary {
    #[serde(alias = "total_messages", alias = "total_messages_in")]
    pub total_messages_in: i64,
    #[serde(default)]
    pub total_messages_out: i64,
    #[serde(default)]
    pub total_bytes_in: i64,
    #[serde(default)]
    pub total_bytes_out: i64,
    #[serde(alias = "avg_per_hour", alias = "avg_messages_per_minute")]
    pub avg_messages_per_minute: f64,
    #[serde(alias = "peak_per_hour", alias = "peak_messages_per_minute", default)]
    pub peak_messages_per_minute: i64,
    #[serde(default)]
    pub avg_active_connections: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThroughputResponse {
    pub summary: ThroughputSummary,
    #[serde(default)]
    pub timeline: Vec<HashMap<String, serde_json::Value>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QualityDistribution {
    pub excellent: i64,
    pub good: i64,
    pub fair: i64,
    pub poor: i64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QualitySummary {
    pub average_quality_score: f64,
    pub distribution: QualityDistribution,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QualityResponse {
    pub summary: QualitySummary,
    #[serde(default)]
    pub issues: Vec<HashMap<String, serde_json::Value>>,
}

// ============================================================
// Analytics Client
// ============================================================

pub struct AnalyticsClient {
    client: Client,
    base_url: String,
    api_token: String,
}

impl AnalyticsClient {
    /// Create a new Analytics client
    pub fn new(base_url: &str, api_token: &str) -> Result<Self> {
        if api_token.is_empty() {
            return Err(AnalyticsError::Config(
                "API token is required".to_string(),
            ));
        }

        let client = Client::builder()
            .timeout(std::time::Duration::from_secs(30))
            .build()?;

        Ok(Self {
            client,
            base_url: base_url.to_string(),
            api_token: api_token.to_string(),
        })
    }

    /// Create client from environment variables
    pub fn from_env() -> Result<Self> {
        let base_url = std::env::var("TESAIOT_API_URL")
            .unwrap_or_else(|_| "https://admin.tesaiot.com/api/v1/bdh-ai".to_string());
        let api_token = std::env::var("TESAIOT_API_TOKEN")
            .map_err(|_| AnalyticsError::Config("TESAIOT_API_TOKEN not set".to_string()))?;

        Self::new(&base_url, &api_token)
    }

    /// Make a POST request
    async fn post<T: Serialize, R: for<'de> Deserialize<'de>>(
        &self,
        path: &str,
        body: &T,
    ) -> Result<R> {
        let url = format!("{}{}", self.base_url, path);
        let response = self
            .client
            .post(&url)
            .header("X-API-KEY", &self.api_token)
            .header("Content-Type", "application/json")
            .json(body)
            .send()
            .await?;

        if !response.status().is_success() {
            let status = response.status().as_u16();
            let message = response.text().await.unwrap_or_default();
            return Err(AnalyticsError::Api { status, message });
        }

        Ok(response.json().await?)
    }

    /// Make a GET request
    async fn get<R: for<'de> Deserialize<'de>>(
        &self,
        path: &str,
        params: &[(&str, String)],
    ) -> Result<R> {
        let url = format!("{}{}", self.base_url, path);
        let mut request = self
            .client
            .get(&url)
            .header("X-API-KEY", &self.api_token);

        for (key, value) in params {
            if !value.is_empty() {
                request = request.query(&[(key, value)]);
            }
        }

        let response = request.send().await?;

        if !response.status().is_success() {
            let status = response.status().as_u16();
            let message = response.text().await.unwrap_or_default();
            return Err(AnalyticsError::Api { status, message });
        }

        Ok(response.json().await?)
    }

    // --------------------------------------------------------
    // Anomaly Detection APIs
    // --------------------------------------------------------

    /// Get aggregated anomalies
    /// Note: API uses GET method with query parameters
    pub async fn get_anomalies(
        &self,
        _time_range: Option<TimeRange>,
        severity_filter: Option<Vec<&str>>,
        _device_ids: Option<Vec<&str>>,
        limit: i64,
        offset: i64,
    ) -> Result<AnomaliesResponse> {
        let mut params = vec![
            ("limit", limit.to_string()),
            ("offset", offset.to_string()),
        ];

        if let Some(filter) = severity_filter {
            for severity in filter {
                params.push(("severity", severity.to_string()));
            }
        }

        self.get("/anomalies", &params).await
    }

    /// Get anomaly timeline
    pub async fn get_anomaly_timeline(
        &self,
        days: i64,
        group_by: &str,
    ) -> Result<TimelineResponse> {
        let payload = serde_json::json!({
            "days": days,
            "group_by": group_by
        });

        self.post("/analytics/anomalies/timeline", &payload).await
    }

    // --------------------------------------------------------
    // Pattern Recognition APIs
    // --------------------------------------------------------

    /// Get K-means clusters
    pub async fn get_clusters(
        &self,
        metric_name: &str,
        n_clusters: i32,
        time_range: Option<TimeRange>,
        include_outliers: bool,
    ) -> Result<ClustersResponse> {
        let tr = time_range.unwrap_or_else(|| TimeRange::last_days(7));

        let payload = serde_json::json!({
            "metric_name": metric_name,
            "n_clusters": n_clusters,
            "time_range": { "start": tr.start, "end": tr.end },
            "include_outliers": include_outliers
        });

        self.post("/patterns/clusters", &payload).await
    }

    // --------------------------------------------------------
    // Insights APIs
    // --------------------------------------------------------

    /// Get AI insights
    pub async fn get_insights(
        &self,
        days: i64,
        insight_types: Option<Vec<&str>>,
        min_confidence: f64,
    ) -> Result<InsightsResponse> {
        let mut payload = serde_json::json!({
            "analysis_period_days": days,
            "min_confidence": min_confidence
        });

        if let Some(types) = insight_types {
            payload["insight_types"] = serde_json::json!(types);
        }

        self.post("/insights", &payload).await
    }

    // --------------------------------------------------------
    // Connectivity APIs
    // --------------------------------------------------------

    /// Get connectivity status
    pub async fn get_connectivity_status(
        &self,
        status_filter: Option<&str>,
    ) -> Result<ConnectivityResponse> {
        let params = vec![("status", status_filter.unwrap_or("").to_string())];
        self.get("/connectivity/status", &params).await
    }

    /// Get latency statistics
    pub async fn get_latency_stats(&self, hours: i64) -> Result<LatencyResponse> {
        let params = vec![("hours", hours.to_string())];
        self.get("/connectivity/latency", &params).await
    }

    /// Get throughput statistics
    pub async fn get_throughput_stats(&self, hours: i64) -> Result<ThroughputResponse> {
        let params = vec![("hours", hours.to_string())];
        self.get("/connectivity/throughput", &params).await
    }

    /// Get connection quality
    pub async fn get_connection_quality(&self) -> Result<QualityResponse> {
        self.get("/connectivity/quality", &[]).await
    }
}
