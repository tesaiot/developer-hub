/**
 * MQTT Streaming Hook
 *
 * Manages WebSocket Secure connection to TESAIoT MQTT broker
 * for real-time telemetry subscription.
 *
 * Features:
 * - Token-based authentication
 * - Automatic reconnection
 * - Message buffering
 * - Connection state management
 *
 * Usage:
 * ```tsx
 * const { isConnected, messages, connect, disconnect } = useMQTTStream({
 *   token: 'tesa_mqtt_org_xxx',
 *   topic: 'device/+/telemetry/#',
 * });
 * ```
 *
 * @see https://docs.tesaiot.com/mqtt-api
 */

import { useState, useEffect, useCallback, useRef } from 'react';
import mqtt, { MqttClient, IClientOptions } from 'mqtt';
import type { TelemetryMessage, ConnectionStatus } from '../types';

interface UseMQTTStreamOptions {
  token: string;
  brokerUrl?: string;
  topic?: string;
  maxMessages?: number;
}

interface UseMQTTStreamResult {
  status: ConnectionStatus;
  error: Error | null;
  messages: TelemetryMessage[];
  messageCount: number;
  connect: () => void;
  disconnect: () => void;
  clearMessages: () => void;
}

const DEFAULT_BROKER_URL = 'wss://mqtt.tesaiot.com:8085/mqtt';
const DEFAULT_TOPIC = 'device/+/telemetry/#';
const DEFAULT_MAX_MESSAGES = 1000;

/**
 * Validate MQTT API token format
 *
 * Token format: tesa_mqtt_<org_prefix>_<32_random_chars>
 */
function validateToken(token: string): boolean {
  return token.startsWith('tesa_mqtt_') && token.length >= 40;
}

/**
 * Parse MQTT topic to extract device ID and sensor type
 *
 * Topic format: device/<device_id>/telemetry/<sensor_type>
 */
function parseTopic(topic: string): { deviceId: string; sensorType: string } {
  const parts = topic.split('/');
  return {
    deviceId: parts[1] || 'unknown',
    sensorType: parts.slice(3).join('/') || 'default',
  };
}

/**
 * Parse message payload (assumes JSON format)
 */
function parsePayload(payload: Buffer): Record<string, unknown> {
  try {
    return JSON.parse(payload.toString());
  } catch {
    return { raw: payload.toString() };
  }
}

export function useMQTTStream({
  token,
  brokerUrl = DEFAULT_BROKER_URL,
  topic = DEFAULT_TOPIC,
  maxMessages = DEFAULT_MAX_MESSAGES,
}: UseMQTTStreamOptions): UseMQTTStreamResult {
  const [status, setStatus] = useState<ConnectionStatus>('disconnected');
  const [error, setError] = useState<Error | null>(null);
  const [messages, setMessages] = useState<TelemetryMessage[]>([]);

  const clientRef = useRef<MqttClient | null>(null);

  /**
   * Connect to MQTT broker
   */
  const connect = useCallback(() => {
    // Validate token
    if (!validateToken(token)) {
      setError(new Error('Invalid token format. Token must start with "tesa_mqtt_"'));
      setStatus('error');
      return;
    }

    // Don't reconnect if already connected
    if (clientRef.current?.connected) {
      return;
    }

    setStatus('connecting');
    setError(null);

    const clientId = `tesaiot-dashboard-${Date.now()}`;

    const options: IClientOptions = {
      username: token,
      password: token,
      clientId,
      reconnectPeriod: 5000,
      connectTimeout: 30000,
      keepalive: 60,
      clean: true,
    };

    console.log(`[MQTT] Connecting to ${brokerUrl}...`);
    const client = mqtt.connect(brokerUrl, options);
    clientRef.current = client;

    client.on('connect', () => {
      console.log('[MQTT] Connected successfully');
      setStatus('connected');

      client.subscribe(topic, { qos: 1 }, (err, granted) => {
        if (err) {
          console.error('[MQTT] Subscription error:', err);
          setError(new Error(`Subscription failed: ${err.message}`));
        } else {
          console.log('[MQTT] Subscribed to:', granted?.map((g) => g.topic).join(', '));
        }
      });
    });

    client.on('message', (receivedTopic, payload) => {
      const { deviceId, sensorType } = parseTopic(receivedTopic);
      const data = parsePayload(payload);

      const message: TelemetryMessage = {
        deviceId,
        sensorType,
        data: data as Record<string, number | string | boolean>,
        timestamp: new Date(),
        raw: payload.toString(),
      };

      setMessages((prev) => {
        const updated = [...prev, message];
        // Keep only last maxMessages
        return updated.slice(-maxMessages);
      });
    });

    client.on('error', (err) => {
      console.error('[MQTT] Error:', err);
      setError(err);
      setStatus('error');
    });

    client.on('close', () => {
      console.log('[MQTT] Connection closed');
      if (status !== 'error') {
        setStatus('disconnected');
      }
    });

    client.on('reconnect', () => {
      console.log('[MQTT] Reconnecting...');
      setStatus('connecting');
    });
  }, [token, brokerUrl, topic, maxMessages, status]);

  /**
   * Disconnect from MQTT broker
   */
  const disconnect = useCallback(() => {
    if (clientRef.current) {
      console.log('[MQTT] Disconnecting...');
      clientRef.current.end();
      clientRef.current = null;
      setStatus('disconnected');
    }
  }, []);

  /**
   * Clear message buffer
   */
  const clearMessages = useCallback(() => {
    setMessages([]);
  }, []);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (clientRef.current) {
        clientRef.current.end();
        clientRef.current = null;
      }
    };
  }, []);

  return {
    status,
    error,
    messages,
    messageCount: messages.length,
    connect,
    disconnect,
    clearMessages,
  };
}
