import Keycloak from "keycloak-js";

/**
 * Keycloak Configuration
 *
 * Initializes the Keycloak client with TESAIoT Platform settings.
 * Configuration is loaded from environment variables.
 */
const keycloak = new Keycloak({
  url: import.meta.env.VITE_KEYCLOAK_URL || "https://auth.tesaiot.org",
  realm: import.meta.env.VITE_KEYCLOAK_REALM || "tesa-iot-platform",
  clientId: import.meta.env.VITE_KEYCLOAK_CLIENT_ID || "tesa-iot-web",
});

export default keycloak;

/**
 * Helper function to check if user has a specific role
 */
export function hasRole(keycloak: Keycloak, role: string): boolean {
  // Check realm roles
  if (keycloak.hasRealmRole(role)) {
    return true;
  }

  // Check client roles
  const clientId = import.meta.env.VITE_KEYCLOAK_CLIENT_ID || "tesa-iot-web";
  if (keycloak.hasResourceRole(role, clientId)) {
    return true;
  }

  return false;
}

/**
 * Helper function to check if user has any of the specified roles
 */
export function hasAnyRole(keycloak: Keycloak, roles: string[]): boolean {
  return roles.some((role) => hasRole(keycloak, role));
}

/**
 * Get user's organization ID from token claims
 */
export function getOrganizationId(keycloak: Keycloak): string | undefined {
  const tokenParsed = keycloak.tokenParsed as Record<string, unknown>;
  return tokenParsed?.organization_id as string | undefined;
}

/**
 * Get user's display name from token claims
 */
export function getUserDisplayName(keycloak: Keycloak): string {
  const tokenParsed = keycloak.tokenParsed as Record<string, unknown>;
  return (
    (tokenParsed?.name as string) ||
    (tokenParsed?.preferred_username as string) ||
    "User"
  );
}
