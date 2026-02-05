//! TESAIoT WSS Live Streaming - Rust Client
//!
//! Subscribe to real-time device telemetry via WebSocket Secure MQTT.
//!
//! # Prerequisites
//! 1. Generate an MQTT API Token from TESAIoT Admin UI
//!    (Organization Settings > MQTT API Tokens > Generate New Token)
//! 2. Copy your token and save it in .env file
//!
//! # Usage
//! ```bash
//! cp .env.example .env
//! # Edit .env with your token
//! cargo run --release
//! ```
//!
//! See: <https://github.com/tesaiot/developer-hub>

use chrono::Utc;
use rumqttc::{AsyncClient, Event, Incoming, MqttOptions, QoS, Transport};
use serde_json::Value;
use std::env;
use std::error::Error;
use std::time::Duration;
use tokio::signal;
use url::Url;

/// Application configuration
struct Config {
    token: String,
    broker_url: String,
    topic: String,
    client_id: String,
    host: String,
    port: u16,
}

impl Config {
    /// Load configuration from environment variables
    fn from_env() -> Result<Self, Box<dyn Error>> {
        let token = env::var("MQTT_API_TOKEN").unwrap_or_default();
        let broker_url = env::var("MQTT_BROKER_URL")
            .unwrap_or_else(|_| "wss://mqtt.tesaiot.com:8085/mqtt".to_string());
        let topic =
            env::var("MQTT_TOPIC").unwrap_or_else(|_| "device/+/telemetry/#".to_string());
        let client_id = env::var("MQTT_CLIENT_ID")
            .unwrap_or_else(|_| format!("tesaiot-rust-{}", Utc::now().timestamp()));

        // Parse broker URL
        let (host, port) = Self::parse_broker_url(&broker_url)?;

        Ok(Config {
            token,
            broker_url,
            topic,
            client_id,
            host,
            port,
        })
    }

    /// Parse WSS URL into host and port
    fn parse_broker_url(url_str: &str) -> Result<(String, u16), Box<dyn Error>> {
        let url = Url::parse(url_str)?;
        let host = url.host_str().ok_or("Invalid host in URL")?.to_string();
        let port = url.port().unwrap_or(8085);
        Ok((host, port))
    }

    /// Validate configuration
    fn validate(&self) -> Result<(), Box<dyn Error>> {
        if self.token.is_empty() {
            eprintln!("❌ ERROR: MQTT_API_TOKEN is required");
            eprintln!();
            eprintln!("Please set MQTT_API_TOKEN in .env file or environment variable");
            eprintln!("Generate a token from TESAIoT Admin UI:");
            eprintln!("  1. Login to https://admin.tesaiot.com");
            eprintln!("  2. Go to Organization Settings > MQTT API Tokens");
            eprintln!("  3. Click \"Generate New Token\"");
            return Err("MQTT_API_TOKEN is required".into());
        }

        if !self.token.starts_with("tesa_mqtt_") {
            eprintln!("❌ ERROR: Invalid token format");
            eprintln!();
            eprintln!("Token should start with \"tesa_mqtt_\"");
            return Err("Invalid token format".into());
        }

        Ok(())
    }
}

/// Display application banner
fn display_banner() {
    println!();
    println!("┌─────────────────────────────────────────────────┐");
    println!("│  TESAIoT WSS Live Streaming - Rust Client       │");
    println!("└─────────────────────────────────────────────────┘");
    println!();
}

/// Process received telemetry message
///
/// Customize this function to handle telemetry data:
/// - Store in database
/// - Forward to webhook
/// - Trigger alerts
/// - Update dashboard
fn process_message(device_id: &str, sensor_type: &str, data: &Value) {
    // Example: Add your custom processing logic here
    // - Store in PostgreSQL/MongoDB
    // - Send to Redis for real-time dashboard
    // - Trigger webhook for external systems
    // - Check thresholds and send alerts
    let _ = (device_id, sensor_type, data); // Suppress unused warnings
}

/// Handle incoming MQTT publish message
fn handle_publish(topic: &str, payload: &[u8]) {
    // Parse topic: device/<device_id>/telemetry/<sensor_type>
    let parts: Vec<&str> = topic.split('/').collect();
    let device_id = parts.get(1).unwrap_or(&"unknown");
    let sensor_type = if parts.len() > 3 {
        parts[3..].join("/")
    } else {
        "default".to_string()
    };

    // Parse JSON payload
    let data: Value = match serde_json::from_slice(payload) {
        Ok(v) => v,
        Err(_) => {
            let raw = String::from_utf8_lossy(payload);
            serde_json::json!({"raw": raw})
        }
    };

    // Log received telemetry
    let timestamp = Utc::now().to_rfc3339();
    println!("[{}] {}", timestamp, topic);
    println!("  Device: {}", device_id);
    println!("  Sensor: {}", sensor_type);
    println!("  Data: {}", serde_json::to_string_pretty(&data).unwrap_or_default());
    println!();

    // Process message
    process_message(device_id, &sensor_type, &data);
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    // Load .env file
    dotenv::dotenv().ok();

    display_banner();

    // Load and validate configuration
    let config = Config::from_env()?;
    config.validate()?;

    println!("Connecting to TESAIoT MQTT Broker via WSS...");
    println!("  Broker: {}", config.broker_url);
    println!("  Client ID: {}", config.client_id);
    println!(
        "  Token: {}...{}",
        &config.token[..20.min(config.token.len())],
        &config.token[config.token.len().saturating_sub(4)..]
    );
    println!();

    // Create MQTT options
    let mut mqtt_options = MqttOptions::new(&config.client_id, &config.host, config.port);
    mqtt_options.set_credentials(&config.token, &config.token);
    mqtt_options.set_keep_alive(Duration::from_secs(60));
    mqtt_options.set_transport(Transport::wss_with_default_config());

    // Create async client and event loop
    let (client, mut eventloop) = AsyncClient::new(mqtt_options, 10);

    // Subscribe to topic
    client
        .subscribe(&config.topic, QoS::AtLeastOnce)
        .await?;

    println!("✅ Connected to TESAIoT MQTT Broker!");
    println!("📡 Subscribed to: {}", config.topic);
    println!();
    println!("Waiting for telemetry messages...");
    println!("{}", "─".repeat(50));
    println!();

    // Event loop with graceful shutdown
    loop {
        tokio::select! {
            event = eventloop.poll() => {
                match event {
                    Ok(Event::Incoming(Incoming::Publish(publish))) => {
                        handle_publish(&publish.topic, &publish.payload);
                    }
                    Ok(Event::Incoming(Incoming::ConnAck(_))) => {
                        // Connection acknowledged
                    }
                    Ok(Event::Incoming(Incoming::SubAck(_))) => {
                        // Subscription acknowledged
                    }
                    Ok(_) => {
                        // Other events (PingReq, PingResp, etc.)
                    }
                    Err(e) => {
                        eprintln!("❌ Connection error: {:?}", e);
                        eprintln!("   Reconnecting in 5 seconds...");
                        tokio::time::sleep(Duration::from_secs(5)).await;
                    }
                }
            }
            _ = signal::ctrl_c() => {
                println!();
                println!("Received shutdown signal. Disconnecting...");
                client.disconnect().await?;
                println!("✅ Disconnected. Goodbye!");
                break;
            }
        }
    }

    Ok(())
}
