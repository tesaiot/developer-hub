# Rust Implementation Architecture

## Overview

Async WebSocket-based MQTT client using the `rumqttc` library with `tokio` runtime for real-time telemetry streaming.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Rust Application                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  main.rs (Entry Point)                │  │
│  │                                                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐   │  │
│  │  │ dotenv      │  │ Config      │  │ Validation   │   │  │
│  │  │ loader      │  │ struct      │  │ functions    │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘   │  │
│  │         │                │                │           │  │
│  │         └────────────────┴────────────────┘           │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │             rumqttc Client                    │    │  │
│  │  │                                               │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ MqttOptions  │  │ AsyncClient         │    │    │  │
│  │  │  │ (WSS config) │  │ + EventLoop         │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                          │                            │  │
│  │                          ▼                            │  │
│  │  ┌───────────────────────────────────────────────┐    │  │
│  │  │           Event Processing (async)            │    │  │
│  │  │  ┌──────────────┐  ┌─────────────────────┐    │    │  │
│  │  │  │ Event::      │  │ process_message()   │    │    │  │
│  │  │  │ Incoming     │  │ (Custom Handler)    │    │    │  │
│  │  │  └──────────────┘  └─────────────────────┘    │    │  │
│  │  └───────────────────────────────────────────────┘    │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Code Structure

```rust
// Configuration
struct Config {
    token: String,
    broker_url: String,
    topic: String,
    client_id: String,
}

// Main async function
#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    // 1. Load .env
    dotenv::dotenv().ok();

    // 2. Parse config
    let config = Config::from_env()?;

    // 3. Create MQTT options
    let mut options = MqttOptions::new(
        &config.client_id,
        &config.host,
        config.port
    );
    options.set_transport(Transport::wss_with_default_config());
    options.set_credentials(&config.token, &config.token);

    // 4. Create client and event loop
    let (client, mut eventloop) = AsyncClient::new(options, 10);

    // 5. Subscribe
    client.subscribe(&config.topic, QoS::AtLeastOnce).await?;

    // 6. Process events
    loop {
        match eventloop.poll().await {
            Ok(Event::Incoming(Packet::Publish(publish))) => {
                process_message(&publish);
            }
            Err(e) => {
                eprintln!("Error: {:?}", e);
            }
            _ => {}
        }
    }
}
```

## Event Loop

```
┌─────────────────┐
│  tokio::main    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Load Config    │
│  (dotenv)       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  MqttOptions    │
│  with WSS       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  AsyncClient    │
│  + EventLoop    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  client.sub()   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  eventloop      │◀───┐
│  .poll()        │    │
└────────┬────────┘    │
         │             │
         ▼             │
┌─────────────────┐    │
│ match Event {   │    │
│   Publish =>    │────┘
│   process()     │
│ }               │
└─────────────────┘
```

## Key Design Decisions

| Decision            | Rationale                                 |
| ------------------- | ----------------------------------------- |
| `rumqttc`           | Pure Rust, async-first, WebSocket support |
| `tokio` runtime     | Industry standard async runtime           |
| Struct-based config | Type safety, clear validation             |
| Pattern matching    | Idiomatic Rust event handling             |
| Zero-copy parsing   | Performance for high throughput           |

## WebSocket Configuration

```rust
use rumqttc::{MqttOptions, Transport};

// Parse WSS URL
let url = "wss://mqtt.tesaiot.com:8085/mqtt";
let (host, port, path) = parse_wss_url(url);

// Configure options
let mut options = MqttOptions::new(client_id, host, port);

// Enable WSS transport
options.set_transport(Transport::Wss(TlsConfiguration::default()));

// Set WebSocket path
// Note: rumqttc uses the path from the URL automatically
```

## Error Handling

```rust
use rumqttc::{ConnectionError, ClientError};

loop {
    match eventloop.poll().await {
        Ok(event) => handle_event(event),
        Err(ConnectionError::Io(e)) => {
            eprintln!("IO error: {}", e);
            // Reconnect logic
        }
        Err(ConnectionError::MqttState(e)) => {
            eprintln!("MQTT state error: {:?}", e);
        }
        Err(e) => {
            eprintln!("Unknown error: {:?}", e);
        }
    }
}
```

## Performance Considerations

| Aspect       | Implementation                     |
| ------------ | ---------------------------------- |
| Memory       | Zero-copy message handling         |
| Concurrency  | Single event loop (efficient)      |
| Reconnection | Built-in with configurable backoff |
| Buffering    | Configurable channel capacity      |

## Signal Handling

```rust
use tokio::signal;

// Graceful shutdown
tokio::select! {
    _ = async {
        loop {
            eventloop.poll().await?;
        }
    } => {}
    _ = signal::ctrl_c() => {
        println!("Shutting down...");
    }
}
```
