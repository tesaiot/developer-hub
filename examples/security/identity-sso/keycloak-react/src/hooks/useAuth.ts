import { useCallback, useMemo } from "react";
import { useKeycloak } from "@react-keycloak/web";
import type { User, KeycloakTokenParsed } from "../types/auth";
import { hasRole, hasAnyRole } from "../services/keycloak";

/**
 * Custom hook for authentication
 *
 * Provides convenient access to authentication state and actions.
 */
export function useAuth() {
  const { keycloak, initialized } = useKeycloak();

  /**
   * Parse user information from Keycloak token
   */
  const user = useMemo<User | null>(() => {
    if (!keycloak.authenticated || !keycloak.tokenParsed) {
      return null;
    }

    const token = keycloak.tokenParsed as KeycloakTokenParsed;
    const clientId =
      import.meta.env.VITE_KEYCLOAK_CLIENT_ID || "tesaiot-web-app";

    // Collect all roles
    const realmRoles = token.realm_access?.roles || [];
    const clientRoles = token.resource_access?.[clientId]?.roles || [];
    const allRoles = [...new Set([...realmRoles, ...clientRoles])];

    return {
      id: token.sub || "",
      username: token.preferred_username || "",
      email: token.email || "",
      name: token.name || token.preferred_username || "",
      firstName: token.given_name,
      lastName: token.family_name,
      emailVerified: token.email_verified || false,
      organizationId: token.organization_id,
      organizationName: token.organization_name,
      roles: allRoles,
    };
  }, [keycloak.authenticated, keycloak.tokenParsed]);

  /**
   * Trigger login flow
   */
  const login = useCallback(
    (redirectUri?: string) => {
      keycloak.login({
        redirectUri: redirectUri || window.location.origin + "/dashboard",
      });
    },
    [keycloak],
  );

  /**
   * Trigger logout flow
   */
  const logout = useCallback(
    (redirectUri?: string) => {
      keycloak.logout({
        redirectUri: redirectUri || window.location.origin,
      });
    },
    [keycloak],
  );

  /**
   * Check if user has a specific role
   */
  const checkRole = useCallback(
    (role: string) => {
      return hasRole(keycloak, role);
    },
    [keycloak],
  );

  /**
   * Check if user has any of the specified roles
   */
  const checkAnyRole = useCallback(
    (roles: string[]) => {
      return hasAnyRole(keycloak, roles);
    },
    [keycloak],
  );

  /**
   * Get access token (refreshing if necessary)
   */
  const getAccessToken = useCallback(async (): Promise<string | null> => {
    if (!keycloak.authenticated) {
      return null;
    }

    try {
      await keycloak.updateToken(30);
      return keycloak.token || null;
    } catch (error) {
      console.error("Failed to refresh token:", error);
      keycloak.login();
      return null;
    }
  }, [keycloak]);

  return {
    // State
    initialized,
    isAuthenticated: keycloak.authenticated || false,
    isLoading: !initialized,
    user,
    token: keycloak.token,

    // Actions
    login,
    logout,
    getAccessToken,

    // Role checks
    hasRole: checkRole,
    hasAnyRole: checkAnyRole,

    // Raw keycloak instance (for advanced usage)
    keycloak,
  };
}
