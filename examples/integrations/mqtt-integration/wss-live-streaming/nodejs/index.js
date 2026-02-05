/**
 * TESAIoT WSS Live Streaming - Node.js Client
 *
 * Subscribe to real-time device telemetry via WebSocket Secure MQTT.
 *
 * Prerequisites:
 * 1. Generate an MQTT API Token from TESAIoT Admin UI
 *    (Organization Settings > MQTT API Tokens > Generate New Token)
 * 2. Copy your token and save it in .env file
 *
 * Usage:
 *   npm install
 *   cp .env.example .env
 *   # Edit .env with your token
 *   npm start
 *
 * @see https://github.com/tesaiot/developer-hub
 */

require("dotenv").config();
const mqtt = require("mqtt");

// =============================================================================
// Configuration
// =============================================================================

const config = {
  // MQTT API Token (generate from TESAIoT Admin UI)
  // Format: tesa_mqtt_<org_prefix>_<random_32_chars>
  token: process.env.MQTT_API_TOKEN || "",

  // WSS Broker URL
  brokerUrl: process.env.MQTT_BROKER_URL || "wss://mqtt.tesaiot.com:8085/mqtt",

  // Topics to subscribe (default: all devices in organization)
  // Use specific device ID for single device: device/<device_id>/telemetry/#
  topic: process.env.MQTT_TOPIC || "device/+/telemetry/#",

  // Client ID (must be unique per connection)
  clientId: process.env.MQTT_CLIENT_ID || `tesaiot-nodejs-${Date.now()}`,

  // Reconnect settings
  reconnectPeriod: parseInt(process.env.MQTT_RECONNECT_PERIOD) || 5000,
  connectTimeout: 30000,

  // Keep alive interval (seconds)
  keepalive: parseInt(process.env.MQTT_KEEPALIVE) || 60,
};

// =============================================================================
// Validation
// =============================================================================

function validateConfig() {
  if (!config.token) {
    console.error("❌ ERROR: MQTT_API_TOKEN is required");
    console.error("");
    console.error(
      "Please set MQTT_API_TOKEN in .env file or environment variable",
    );
    console.error("Generate a token from TESAIoT Admin UI:");
    console.error("  1. Login to https://admin.tesaiot.com");
    console.error("  2. Go to Organization Settings > MQTT API Tokens");
    console.error('  3. Click "Generate New Token"');
    process.exit(1);
  }

  if (!config.token.startsWith("tesa_mqtt_")) {
    console.error("❌ ERROR: Invalid token format");
    console.error("");
    console.error('Token should start with "tesa_mqtt_"');
    console.error("Current token:", config.token.substring(0, 20) + "...");
    process.exit(1);
  }
}

// =============================================================================
// Display Banner
// =============================================================================

function displayBanner() {
  console.log("");
  console.log("┌─────────────────────────────────────────────────┐");
  console.log("│  TESAIoT WSS Live Streaming - Node.js Client    │");
  console.log("└─────────────────────────────────────────────────┘");
  console.log("");
}

// =============================================================================
// Message Processing
// =============================================================================

/**
 * Process received telemetry message
 * Customize this function to handle telemetry data:
 * - Store in database
 * - Forward to webhook
 * - Trigger alerts
 * - Update dashboard
 *
 * @param {string} deviceId - Device UUID
 * @param {string} sensorType - Sensor type (e.g., 'temperature', 'humidity')
 * @param {object} data - Parsed JSON payload
 */
function processMessage(deviceId, sensorType, data) {
  // Example: Log to console
  // Add your custom processing logic here
  // Example integrations:
  // - Store in PostgreSQL/MongoDB
  // - Send to Redis for real-time dashboard
  // - Trigger webhook for external systems
  // - Check thresholds and send alerts
}

// =============================================================================
// MQTT Client
// =============================================================================

function createClient() {
  console.log("Connecting to TESAIoT MQTT Broker via WSS...");
  console.log(`  Broker URL: ${config.brokerUrl}`);
  console.log(`  Client ID: ${config.clientId}`);
  console.log(
    `  Token: ${config.token.substring(0, 20)}...${config.token.slice(-4)}`,
  );
  console.log("");

  const client = mqtt.connect(config.brokerUrl, {
    username: config.token,
    password: config.token, // Same value for both username and password
    clientId: config.clientId,
    rejectUnauthorized: false, // Set to true in production with proper CA certs
    reconnectPeriod: config.reconnectPeriod,
    connectTimeout: config.connectTimeout,
    keepalive: config.keepalive,
    clean: true, // Start with clean session
  });

  // Connection successful
  client.on("connect", () => {
    console.log("✅ Connected to TESAIoT MQTT Broker!");

    // Subscribe to telemetry topic
    client.subscribe(config.topic, { qos: 1 }, (err, granted) => {
      if (err) {
        console.error("❌ Subscription error:", err.message);
        return;
      }

      console.log("📡 Subscribed to topics:");
      granted.forEach((sub) => {
        console.log(`  - ${sub.topic} (QoS ${sub.qos})`);
      });
      console.log("");
      console.log("Waiting for telemetry messages...");
      console.log("─".repeat(50));
      console.log("");
    });
  });

  // Message received
  client.on("message", (topic, payload) => {
    // Parse topic to extract device ID
    // Topic format: device/<device_id>/telemetry/<sensor_type>
    const topicParts = topic.split("/");
    const deviceId = topicParts[1];
    const sensorType = topicParts.slice(3).join("/") || "default";

    // Parse payload (assuming JSON format)
    let data;
    try {
      data = JSON.parse(payload.toString());
    } catch (e) {
      // If not JSON, use raw string
      data = { raw: payload.toString() };
    }

    // Log received telemetry
    const timestamp = new Date().toISOString();
    console.log(`[${timestamp}] ${topic}`);
    console.log(`  Device: ${deviceId}`);
    console.log(`  Sensor: ${sensorType}`);
    console.log(`  Data:`, JSON.stringify(data, null, 2));
    console.log("");

    // Process telemetry data
    processMessage(deviceId, sensorType, data);
  });

  // Connection error
  client.on("error", (err) => {
    console.error("❌ Connection error:", err.message);
  });

  // Disconnected
  client.on("close", () => {
    console.log("🔌 Disconnected from broker");
    console.log(
      `   Reconnecting in ${config.reconnectPeriod / 1000} seconds...`,
    );
  });

  // Reconnecting
  client.on("reconnect", () => {
    console.log("🔄 Reconnecting...");
  });

  // Offline
  client.on("offline", () => {
    console.log("📴 Client offline");
  });

  return client;
}

// =============================================================================
// Graceful Shutdown
// =============================================================================

function setupGracefulShutdown(client) {
  const shutdown = (signal) => {
    console.log("");
    console.log(`Received ${signal}. Shutting down gracefully...`);
    client.end(true, () => {
      console.log("✅ Disconnected. Goodbye!");
      process.exit(0);
    });
  };

  process.on("SIGINT", () => shutdown("SIGINT"));
  process.on("SIGTERM", () => shutdown("SIGTERM"));
}

// =============================================================================
// Main
// =============================================================================

function main() {
  displayBanner();
  validateConfig();
  const client = createClient();
  setupGracefulShutdown(client);
}

main();
