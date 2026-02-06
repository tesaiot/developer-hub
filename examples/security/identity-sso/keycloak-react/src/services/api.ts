import Keycloak from "keycloak-js";

/**
 * TESAIoT API Client
 *
 * Provides authenticated HTTP client for TESAIoT Core API.
 * Automatically includes Bearer token from Keycloak.
 */

const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL || "https://admin.tesaiot.com/api/v1";

export interface ApiError {
  status: number;
  message: string;
  details?: Record<string, unknown>;
}

export interface Device {
  id: string;
  name: string;
  device_id: string;
  status: "online" | "offline" | "unknown";
  last_seen: string;
  organization_id: string;
  metadata?: Record<string, unknown>;
}

export interface TelemetryData {
  device_id: string;
  timestamp: string;
  metrics: Record<string, number>;
}

/**
 * Create authenticated fetch function
 */
export function createAuthenticatedFetch(keycloak: Keycloak) {
  return async function authenticatedFetch<T>(
    endpoint: string,
    options: RequestInit = {},
  ): Promise<T> {
    // Ensure token is valid, refresh if needed
    try {
      const refreshed = await keycloak.updateToken(30);
      if (refreshed) {
        console.log("Token was refreshed");
      }
    } catch (error) {
      console.error("Failed to refresh token:", error);
      keycloak.login();
      throw new Error("Session expired. Please login again.");
    }

    const headers = new Headers(options.headers);
    headers.set("Authorization", `Bearer ${keycloak.token}`);
    headers.set("Content-Type", "application/json");

    const response = await fetch(`${API_BASE_URL}${endpoint}`, {
      ...options,
      headers,
    });

    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}));
      const error: ApiError = {
        status: response.status,
        message: errorData.message || response.statusText,
        details: errorData,
      };

      if (response.status === 401) {
        keycloak.login();
      }

      throw error;
    }

    return response.json();
  };
}

/**
 * API Service Factory
 */
export function createApiService(keycloak: Keycloak) {
  const authFetch = createAuthenticatedFetch(keycloak);

  return {
    // Device APIs
    devices: {
      list: () => authFetch<Device[]>("/devices"),
      get: (id: string) => authFetch<Device>(`/devices/${id}`),
      create: (data: Partial<Device>) =>
        authFetch<Device>("/devices", {
          method: "POST",
          body: JSON.stringify(data),
        }),
      update: (id: string, data: Partial<Device>) =>
        authFetch<Device>(`/devices/${id}`, {
          method: "PATCH",
          body: JSON.stringify(data),
        }),
      delete: (id: string) =>
        authFetch<void>(`/devices/${id}`, { method: "DELETE" }),
    },

    // Telemetry APIs
    telemetry: {
      latest: (deviceId: string) =>
        authFetch<TelemetryData>(`/devices/${deviceId}/telemetry/latest`),
      history: (deviceId: string, params: { start: string; end: string }) =>
        authFetch<TelemetryData[]>(
          `/devices/${deviceId}/telemetry?start=${params.start}&end=${params.end}`,
        ),
    },

    // User APIs
    user: {
      profile: () =>
        authFetch<{ id: string; email: string; name: string }>("/user/profile"),
      organizations: () =>
        authFetch<{ id: string; name: string }[]>("/user/organizations"),
    },
  };
}
