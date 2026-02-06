/**
 * Authentication Types
 */

export interface User {
  id: string;
  username: string;
  email: string;
  name: string;
  firstName?: string;
  lastName?: string;
  emailVerified: boolean;
  organizationId?: string;
  organizationName?: string;
  roles: string[];
}

export interface TokenInfo {
  accessToken: string;
  refreshToken?: string;
  idToken?: string;
  expiresIn: number;
  tokenType: string;
}

export interface AuthState {
  isAuthenticated: boolean;
  isLoading: boolean;
  user: User | null;
  error: string | null;
}

export interface KeycloakTokenParsed {
  exp?: number;
  iat?: number;
  jti?: string;
  iss?: string;
  aud?: string | string[];
  sub?: string;
  typ?: string;
  azp?: string;
  session_state?: string;
  acr?: string;
  realm_access?: {
    roles: string[];
  };
  resource_access?: {
    [key: string]: {
      roles: string[];
    };
  };
  scope?: string;
  email_verified?: boolean;
  name?: string;
  preferred_username?: string;
  given_name?: string;
  family_name?: string;
  email?: string;
  organization_id?: string;
  organization_name?: string;
}
