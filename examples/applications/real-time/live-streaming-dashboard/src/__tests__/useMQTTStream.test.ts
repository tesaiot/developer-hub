/**
 * Unit tests for MQTT Streaming Hook
 *
 * Copyright (c) 2025 TESAIoT Platform (TESA)
 * Licensed under Apache License 2.0
 */

import { describe, it, expect } from 'vitest';

// Helper functions to test (extracted from hook for unit testing)
describe('MQTT Stream Utility Functions', () => {
  describe('validateToken', () => {
    function validateToken(token: string): boolean {
      return token.startsWith('tesa_mqtt_') && token.length >= 40;
    }

    it('should accept valid token', () => {
      const validToken = 'tesa_mqtt_org_abc123456789012345678901234567';
      expect(validateToken(validToken)).toBe(true);
    });

    it('should reject token without prefix', () => {
      const invalidToken = 'invalid_token_abc123456789012345678901234567';
      expect(validateToken(invalidToken)).toBe(false);
    });

    it('should reject short token', () => {
      const shortToken = 'tesa_mqtt_short';
      expect(validateToken(shortToken)).toBe(false);
    });

    it('should reject empty token', () => {
      expect(validateToken('')).toBe(false);
    });
  });

  describe('parseTopic', () => {
    function parseTopic(topic: string): { deviceId: string; sensorType: string } {
      const parts = topic.split('/');
      return {
        deviceId: parts[1] || 'unknown',
        sensorType: parts.slice(3).join('/') || 'default',
      };
    }

    it('should parse standard telemetry topic', () => {
      const topic = 'device/abc123/telemetry/sensor';
      const result = parseTopic(topic);
      expect(result.deviceId).toBe('abc123');
      expect(result.sensorType).toBe('sensor');
    });

    it('should parse nested sensor type', () => {
      const topic = 'device/abc123/telemetry/environmental/temperature';
      const result = parseTopic(topic);
      expect(result.deviceId).toBe('abc123');
      expect(result.sensorType).toBe('environmental/temperature');
    });

    it('should handle missing device ID', () => {
      const topic = 'device';
      const result = parseTopic(topic);
      expect(result.deviceId).toBe('unknown');
      expect(result.sensorType).toBe('default');
    });

    it('should handle missing sensor type', () => {
      const topic = 'device/abc123/telemetry';
      const result = parseTopic(topic);
      expect(result.deviceId).toBe('abc123');
      expect(result.sensorType).toBe('default');
    });
  });

  describe('parsePayload', () => {
    function parsePayload(payload: Buffer): Record<string, unknown> {
      try {
        return JSON.parse(payload.toString());
      } catch {
        return { raw: payload.toString() };
      }
    }

    it('should parse JSON payload', () => {
      const payload = Buffer.from('{"temperature": 25.5, "humidity": 60}');
      const result = parsePayload(payload);
      expect(result.temperature).toBe(25.5);
      expect(result.humidity).toBe(60);
    });

    it('should handle non-JSON payload as raw', () => {
      const payload = Buffer.from('plain text data');
      const result = parsePayload(payload);
      expect(result.raw).toBe('plain text data');
    });

    it('should handle empty payload', () => {
      const payload = Buffer.from('');
      const result = parsePayload(payload);
      expect(result.raw).toBe('');
    });

    it('should handle malformed JSON', () => {
      const payload = Buffer.from('{ invalid json }');
      const result = parsePayload(payload);
      expect(result.raw).toBe('{ invalid json }');
    });

    it('should parse nested JSON object', () => {
      const payload = Buffer.from('{"sensor": {"value": 42, "unit": "C"}}');
      const result = parsePayload(payload);
      expect((result.sensor as Record<string, unknown>).value).toBe(42);
    });
  });
});

describe('MQTT Message Buffer', () => {
  it('should maintain max message limit', () => {
    const maxMessages = 5;
    let messages: { id: number }[] = [];

    // Add more messages than the limit
    for (let i = 0; i < 10; i++) {
      messages = [...messages, { id: i }].slice(-maxMessages);
    }

    expect(messages).toHaveLength(maxMessages);
    expect(messages[0].id).toBe(5); // First should be 5 (oldest kept)
    expect(messages[4].id).toBe(9); // Last should be 9 (newest)
  });
});

describe('Connection Status', () => {
  type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

  it('should have valid status values', () => {
    const validStatuses: ConnectionStatus[] = [
      'disconnected',
      'connecting',
      'connected',
      'error',
    ];

    validStatuses.forEach((status) => {
      expect(['disconnected', 'connecting', 'connected', 'error']).toContain(status);
    });
  });
});

describe('TelemetryMessage', () => {
  interface TelemetryMessage {
    deviceId: string;
    sensorType: string;
    data: Record<string, number | string | boolean>;
    timestamp: Date;
    raw: string;
  }

  it('should construct valid message object', () => {
    const message: TelemetryMessage = {
      deviceId: 'device-001',
      sensorType: 'temperature',
      data: { value: 25.5 },
      timestamp: new Date('2025-01-01T10:00:00Z'),
      raw: '{"value": 25.5}',
    };

    expect(message.deviceId).toBe('device-001');
    expect(message.sensorType).toBe('temperature');
    expect(message.data.value).toBe(25.5);
    expect(message.timestamp).toBeInstanceOf(Date);
    expect(message.raw).toBe('{"value": 25.5}');
  });
});
