import { useState, useCallback, useMemo } from "react";
import { useKeycloak } from "@react-keycloak/web";
import { createApiService, type ApiError } from "../services/api";

/**
 * Custom hook for authenticated API calls
 *
 * Provides loading state, error handling, and automatic token refresh.
 */
export function useApi() {
  const { keycloak } = useKeycloak();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Create API service instance
  const api = useMemo(() => createApiService(keycloak), [keycloak]);

  /**
   * Wrapper function for API calls with loading/error state
   */
  const executeRequest = useCallback(
    async <T>(request: () => Promise<T>): Promise<T | null> => {
      setLoading(true);
      setError(null);

      try {
        const result = await request();
        return result;
      } catch (err) {
        const apiError = err as ApiError;
        setError(apiError.message || "An unexpected error occurred");
        return null;
      } finally {
        setLoading(false);
      }
    },
    [],
  );

  /**
   * Clear any existing errors
   */
  const clearError = useCallback(() => {
    setError(null);
  }, []);

  return {
    api,
    loading,
    error,
    executeRequest,
    clearError,
  };
}
