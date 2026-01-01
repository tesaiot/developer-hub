/**
 * TESAIoT Platform - WSS MQTT Third-Party Integration Example
 *
 * This example demonstrates how to connect to the TESAIoT MQTT broker
 * via WebSocket Secure (WSS) and subscribe to device telemetry data.
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
 */

require('dotenv').config();
const mqtt = require('mqtt');

// Configuration from environment variables
const config = {
  // MQTT API Token (generate from TESAIoT Admin UI)
  // Format: tesa_mqtt_<org_prefix>_<random_32_chars>
  token: process.env.MQTT_API_TOKEN || '',

  // WSS Broker URL
  brokerUrl: process.env.MQTT_BROKER_URL || 'wss://mqtt.tesaiot.com:8085/mqtt',

  // Topics to subscribe (default: all devices in organization)
  // Use specific device ID for single device: device/<device_id>/telemetry/#
  subscribeTopic: process.env.MQTT_SUBSCRIBE_TOPIC || 'device/+/telemetry/#',

  // Client ID (must be unique per connection)
  clientId: process.env.MQTT_CLIENT_ID || `tesaiot-third-party-${Date.now()}`,

  // Reconnect settings
  reconnectPeriod: 5000,  // 5 seconds
  connectTimeout: 30000,  // 30 seconds

  // Keep alive interval (seconds)
  keepalive: 60,
};

// Validate configuration
if (!config.token) {
  console.error('ERROR: MQTT_API_TOKEN is required');
  console.error('Please set MQTT_API_TOKEN in .env file or environment variable');
  console.error('Generate a token from TESAIoT Admin UI > Organization Settings > MQTT API Tokens');
  process.exit(1);
}

if (!config.token.startsWith('tesa_mqtt_')) {
  console.error('ERROR: Invalid token format');
  console.error('Token should start with "tesa_mqtt_"');
  process.exit(1);
}

// Create MQTT client
console.log('Connecting to TESAIoT MQTT Broker via WSS...');
console.log(`  Broker URL: ${config.brokerUrl}`);
console.log(`  Client ID: ${config.clientId}`);
console.log(`  Token: ${config.token.substring(0, 20)}...${config.token.slice(-4)}`);

const client = mqtt.connect(config.brokerUrl, {
  username: config.token,
  password: config.token,  // Same value for both username and password
  clientId: config.clientId,
  rejectUnauthorized: false,  // Set to true in production with proper CA certs
  reconnectPeriod: config.reconnectPeriod,
  connectTimeout: config.connectTimeout,
  keepalive: config.keepalive,
  clean: true,  // Start with clean session
});

// Connection event handlers
client.on('connect', () => {
  console.log('Connected to TESAIoT MQTT Broker!');

  // Subscribe to telemetry topic
  client.subscribe(config.subscribeTopic, { qos: 1 }, (err, granted) => {
    if (err) {
      console.error('Subscription error:', err.message);
      return;
    }

    console.log('Subscribed to topics:');
    granted.forEach((sub) => {
      console.log(`  - ${sub.topic} (QoS ${sub.qos})`);
    });
    console.log('\nWaiting for telemetry messages...\n');
  });
});

client.on('message', (topic, payload) => {
  // Parse topic to extract device ID
  // Topic format: device/<device_id>/telemetry/<sensor_type>
  const topicParts = topic.split('/');
  const deviceId = topicParts[1];
  const sensorType = topicParts.slice(3).join('/') || 'default';

  // Parse payload (assuming JSON format)
  let data;
  try {
    data = JSON.parse(payload.toString());
  } catch (e) {
    data = payload.toString();
  }

  // Log received telemetry
  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] Device: ${deviceId}`);
  console.log(`  Sensor: ${sensorType}`);
  console.log(`  Data:`, JSON.stringify(data, null, 2));
  console.log('');

  // Process telemetry data here
  // Example: Store in database, trigger alerts, update dashboard, etc.
  processtelemetry(deviceId, sensorType, data);
});

client.on('error', (err) => {
  console.error('MQTT Error:', err.message);

  if (err.message.includes('Not authorized')) {
    console.error('Authentication failed. Check your MQTT API Token.');
    console.error('Token may be expired, revoked, or invalid.');
  }
});

client.on('offline', () => {
  console.warn('MQTT client offline');
});

client.on('reconnect', () => {
  console.log('Reconnecting to MQTT broker...');
});

client.on('close', () => {
  console.log('MQTT connection closed');
});

// Example telemetry processing function
function processtelemetry(deviceId, sensorType, data) {
  // Implement your business logic here
  // Examples:
  // - Store in time-series database (InfluxDB, TimescaleDB)
  // - Send to analytics service
  // - Check thresholds and trigger alerts
  // - Update real-time dashboard via WebSocket
  // - Forward to cloud services (AWS IoT, Azure IoT Hub, etc.)
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  client.end(true, () => {
    console.log('MQTT connection closed');
    process.exit(0);
  });
});

process.on('SIGTERM', () => {
  console.log('\nReceived SIGTERM, shutting down...');
  client.end(true, () => {
    process.exit(0);
  });
});

// Keep process running
console.log('Press Ctrl+C to exit\n');
