/**
 * TESAIoT API Client
 *
 * Connects to TESAIoT Platform via API Gateway using API Key authentication.
 * This client fetches telemetry data from BDH AI API endpoints.
 *
 * Licensed under Apache License 2.0
 * Copyright (c) 2024-2025 TESAIoT Platform
 */

// Configuration - Replace with your own API Key from TESAIoT Platform
const API_CONFIG = {
  // TESAIoT API Gateway URL
  baseUrl: 'https://admin.tesaiot.com',

  // API Key from TESAIoT Platform > API Keys menu
  // Get your own API Key at: https://admin.tesaiot.com/api-keys
  apiKey: 'tesa_ak_AR4oJyORvH38xAW7jfC9x53ezAPw4kPN',
};

// Types for telemetry data
export interface TelemetryPoint {
  timestamp: string;
  time: string;
  temperature?: number;
  humidity?: number;
  pressure?: number;
  ai_confidence?: number;
  ai_anomalyScore?: number;
  ai_prediction?: 'normal' | 'warning' | 'anomaly';
  [key: string]: string | number | undefined;
}

export interface AIResult {
  timestamp: string;
  confidence: number;
  anomaly_score: number;
  prediction: 'normal' | 'warning' | 'anomaly';
  model_version?: string;
  latency_ms?: number;
}

export interface DeviceInfo {
  device_id: string;
  name: string;
  device_type?: string;
  status?: string;
}

/**
 * Fetch helper with API Key authentication
 */
async function apiFetch<T>(endpoint: string, options?: RequestInit): Promise<T> {
  const url = `${API_CONFIG.baseUrl}${endpoint}`;

  const response = await fetch(url, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      'X-API-Key': API_CONFIG.apiKey,
      ...options?.headers,
    },
  });

  if (!response.ok) {
    const error = await response.text();
    throw new Error(`API Error (${response.status}): ${error}`);
  }

  return response.json();
}

/**
 * Fetch telemetry data from TESAIoT Telemetry API
 *
 * @param deviceId - Device UUID or ID
 * @param startDate - Start date (YYYY-MM-DD)
 * @param endDate - End date (YYYY-MM-DD)
 * @param limit - Maximum number of records (default 1000)
 */
export async function fetchTelemetryData(
  deviceId: string,
  startDate: string,
  endDate: string,
  limit: number = 1000
): Promise<TelemetryPoint[]> {
  // Use the correct TESAIoT telemetry endpoint
  const endpoint = `/api/v1/telemetry/${deviceId}/query?start_time=${startDate}T00:00:00Z&end_time=${endDate}T23:59:59Z&limit=${limit}`;

  try {
    const response = await apiFetch<{ data: TelemetryPoint[] }>(endpoint);
    // The API returns { data: [...] }, extract the array
    return response.data || [];
  } catch (error) {
    console.error('Failed to fetch telemetry data:', error);
    throw error;
  }
}

/**
 * Fetch AI inference results from TESAIoT Telemetry API
 *
 * Note: AI results are embedded in telemetry data as ai_* fields.
 * This function fetches the latest telemetry and extracts AI-related fields.
 *
 * @param deviceId - Device UUID or ID
 * @param limit - Maximum number of records (default 100)
 */
export async function fetchAIResults(
  deviceId: string,
  limit: number = 100
): Promise<AIResult[]> {
  // Use latest telemetry endpoint - AI results are embedded in telemetry data
  const endpoint = `/api/v1/telemetry/${deviceId}/latest?limit=${limit}`;

  try {
    const response = await apiFetch<{ data: TelemetryPoint[] }>(endpoint);
    const telemetryData = response.data || [];

    // Extract AI results from telemetry data (ai_* fields)
    return telemetryData
      .filter(point => point.ai_confidence !== undefined)
      .map(point => ({
        timestamp: point.timestamp,
        confidence: point.ai_confidence || 0,
        anomaly_score: point.ai_anomalyScore || 0,
        prediction: point.ai_prediction || 'normal',
        latency_ms: point.ai_latency as number | undefined,
      }));
  } catch (error) {
    console.error('Failed to fetch AI results:', error);
    throw error;
  }
}

/**
 * Fetch list of devices from TESAIoT Platform
 */
export async function fetchDevices(): Promise<DeviceInfo[]> {
  const endpoint = '/api/v1/devices';

  try {
    const data = await apiFetch<{ devices: DeviceInfo[] }>(endpoint);
    return data.devices || [];
  } catch (error) {
    console.error('Failed to fetch devices:', error);
    throw error;
  }
}

/**
 * Update API configuration (e.g., for custom API Key)
 */
export function configureApi(config: Partial<typeof API_CONFIG>) {
  Object.assign(API_CONFIG, config);
}

export default {
  fetchTelemetryData,
  fetchAIResults,
  fetchDevices,
  configureApi,
};
