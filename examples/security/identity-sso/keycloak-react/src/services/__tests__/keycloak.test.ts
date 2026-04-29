import { describe, it, expect, vi, beforeEach } from "vitest";

/**
 * Tests for Keycloak helper functions.
 *
 * These tests verify the authentication helper functions:
 * - hasRole() - Check if user has a specific role
 * - hasAnyRole() - Check if user has any of the specified roles
 * - getOrganizationId() - Get user's organization ID from token
 * - getUserDisplayName() - Get user's display name from token
 */

// Mock Keycloak module before importing functions that depend on it
vi.mock("keycloak-js", () => {
  return {
    default: vi.fn().mockImplementation(() => ({
      hasRealmRole: vi.fn(),
      hasResourceRole: vi.fn(),
      tokenParsed: {},
    })),
  };
});

// Import functions after mocking
import {
  hasRole,
  hasAnyRole,
  getOrganizationId,
  getUserDisplayName,
} from "../keycloak";

// Type for mock Keycloak
type MockKeycloak = {
  hasRealmRole: ReturnType<typeof vi.fn>;
  hasResourceRole: ReturnType<typeof vi.fn>;
  tokenParsed: Record<string, unknown> | null | undefined;
};

describe("keycloak helper functions", () => {
  let mockKeycloak: MockKeycloak;

  beforeEach(() => {
    // Create a mock Keycloak instance
    mockKeycloak = {
      hasRealmRole: vi.fn(),
      hasResourceRole: vi.fn(),
      tokenParsed: {},
    };
  });

  describe("hasRole", () => {
    it("returns true when user has realm role", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(true);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasRole(mockKeycloak, "admin");

      expect(result).toBe(true);
      expect(mockKeycloak.hasRealmRole).toHaveBeenCalledWith("admin");
    });

    it("returns true when user has client role", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(true);

      const result = hasRole(mockKeycloak, "device-manager");

      expect(result).toBe(true);
      expect(mockKeycloak.hasResourceRole).toHaveBeenCalledWith(
        "device-manager",
        "tesa-iot-web"
      );
    });

    it("returns false when user has neither realm nor client role", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasRole(mockKeycloak, "nonexistent-role");

      expect(result).toBe(false);
    });

    it("checks both realm and client roles", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      hasRole(mockKeycloak, "test-role");

      expect(mockKeycloak.hasRealmRole).toHaveBeenCalledWith("test-role");
      expect(mockKeycloak.hasResourceRole).toHaveBeenCalledWith(
        "test-role",
        "tesa-iot-web"
      );
    });

    it("handles role with special characters", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(true);

      const result = hasRole(mockKeycloak, "role-with-dashes_123");

      expect(result).toBe(true);
    });

    it("handles empty role name", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasRole(mockKeycloak, "");

      expect(result).toBe(false);
    });
  });

  describe("hasAnyRole", () => {
    it("returns true when user has one of the roles", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockImplementation(
        (role: string) => role === "user"
      );
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasAnyRole(mockKeycloak, ["admin", "user", "viewer"]);

      expect(result).toBe(true);
    });

    it("returns false when user has none of the roles", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasAnyRole(mockKeycloak, ["admin", "superuser"]);

      expect(result).toBe(false);
    });

    it("returns false for empty roles array", () => {
      const result = hasAnyRole(mockKeycloak, []);

      expect(result).toBe(false);
    });

    it("returns true when user has first role in list", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockImplementation(
        (role: string) => role === "admin"
      );
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasAnyRole(mockKeycloak, ["admin", "user", "viewer"]);

      expect(result).toBe(true);
    });

    it("returns true when user has last role in list", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockImplementation(
        (role: string) => role === "viewer"
      );
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      const result = hasAnyRole(mockKeycloak, ["admin", "user", "viewer"]);

      expect(result).toBe(true);
    });

    it("returns true when user has role as client role but not realm role", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(false);
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(true);

      const result = hasAnyRole(mockKeycloak, ["device-manager", "admin"]);

      expect(result).toBe(true);
    });

    it("stops checking after finding first match", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockImplementation(
        (role: string) => role === "admin"
      );
      vi.mocked(mockKeycloak.hasResourceRole).mockReturnValue(false);

      hasAnyRole(mockKeycloak, ["admin", "user", "viewer"]);

      // hasRealmRole should be called for "admin", "user", and "viewer"
      // until it finds a match
      expect(mockKeycloak.hasRealmRole).toHaveBeenCalledWith("admin");
    });

    it("handles single role array", () => {
      vi.mocked(mockKeycloak.hasRealmRole).mockReturnValue(true);

      const result = hasAnyRole(mockKeycloak, ["admin"]);

      expect(result).toBe(true);
    });
  });

  describe("getOrganizationId", () => {
    it("returns organization_id from token", () => {
      mockKeycloak.tokenParsed = {
        organization_id: "org-12345",
      };

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBe("org-12345");
    });

    it("returns undefined when organization_id is not present", () => {
      mockKeycloak.tokenParsed = {
        sub: "user-123",
      };

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBeUndefined();
    });

    it("returns undefined when tokenParsed is undefined", () => {
      mockKeycloak.tokenParsed = undefined;

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBeUndefined();
    });

    it("returns undefined when tokenParsed is null", () => {
      mockKeycloak.tokenParsed = null;

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBeUndefined();
    });

    it("returns empty string when organization_id is empty", () => {
      mockKeycloak.tokenParsed = {
        organization_id: "",
      };

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBe("");
    });

    it("handles organization_id with special characters", () => {
      mockKeycloak.tokenParsed = {
        organization_id: "org_123-test.org",
      };

      const result = getOrganizationId(mockKeycloak);

      expect(result).toBe("org_123-test.org");
    });
  });

  describe("getUserDisplayName", () => {
    it("returns name when available", () => {
      mockKeycloak.tokenParsed = {
        name: "John Doe",
        preferred_username: "johndoe",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("John Doe");
    });

    it("returns preferred_username when name is not available", () => {
      mockKeycloak.tokenParsed = {
        preferred_username: "johndoe",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("johndoe");
    });

    it("returns 'User' when neither name nor preferred_username is available", () => {
      mockKeycloak.tokenParsed = {
        sub: "user-123",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("User");
    });

    it("returns 'User' when tokenParsed is undefined", () => {
      mockKeycloak.tokenParsed = undefined;

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("User");
    });

    it("returns 'User' when tokenParsed is null", () => {
      mockKeycloak.tokenParsed = null;

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("User");
    });

    it("prefers name over preferred_username", () => {
      mockKeycloak.tokenParsed = {
        name: "Full Name",
        preferred_username: "username",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("Full Name");
    });

    it("handles empty name string", () => {
      mockKeycloak.tokenParsed = {
        name: "",
        preferred_username: "johndoe",
      };

      const result = getUserDisplayName(mockKeycloak);

      // Empty string is falsy, so preferred_username should be used
      expect(result).toBe("johndoe");
    });

    it("handles name with special characters", () => {
      mockKeycloak.tokenParsed = {
        name: "José García-Müller",
        preferred_username: "josegm",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe("José García-Müller");
    });

    it("handles very long names", () => {
      const longName = "A".repeat(100);
      mockKeycloak.tokenParsed = {
        name: longName,
        preferred_username: "user",
      };

      const result = getUserDisplayName(mockKeycloak);

      expect(result).toBe(longName);
    });
  });
});
