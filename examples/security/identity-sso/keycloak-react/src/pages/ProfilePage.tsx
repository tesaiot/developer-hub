import { useAuth } from "../hooks/useAuth";

/**
 * Profile Page Component
 *
 * Displays user profile information from the ID token.
 */
function ProfilePage() {
  const { user, token, keycloak } = useAuth();

  // Decode token for display (educational purposes)
  const tokenParts = token?.split(".") || [];
  const decodedPayload = tokenParts[1] ? JSON.parse(atob(tokenParts[1])) : null;

  return (
    <div className="max-w-4xl mx-auto">
      <div className="mb-8">
        <h1 className="text-3xl font-bold text-gray-900 mb-2">Profile</h1>
        <p className="text-gray-600">Your account information</p>
      </div>

      {/* User Info */}
      <div className="card mb-6">
        <div className="flex items-start space-x-6">
          <div className="w-20 h-20 bg-blue-100 rounded-full flex items-center justify-center">
            <span className="text-3xl text-blue-600 font-bold">
              {user?.name?.charAt(0).toUpperCase() || "U"}
            </span>
          </div>
          <div className="flex-1">
            <h2 className="text-2xl font-bold text-gray-900">{user?.name}</h2>
            <p className="text-gray-600">{user?.email}</p>
            {user?.emailVerified && (
              <span className="inline-flex items-center px-2 py-1 mt-2 bg-green-100 text-green-800 text-xs rounded-full">
                <svg
                  className="w-3 h-3 mr-1"
                  fill="currentColor"
                  viewBox="0 0 20 20">
                  <path
                    fillRule="evenodd"
                    d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z"
                    clipRule="evenodd"
                  />
                </svg>
                Email Verified
              </span>
            )}
          </div>
        </div>
      </div>

      {/* Account Details */}
      <div className="card mb-6">
        <h3 className="text-lg font-semibold text-gray-900 mb-4">
          Account Details
        </h3>
        <div className="grid md:grid-cols-2 gap-4">
          <div>
            <label className="block text-sm text-gray-600 mb-1">User ID</label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded">
              {user?.id}
            </p>
          </div>
          <div>
            <label className="block text-sm text-gray-600 mb-1">Username</label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded">
              {user?.username}
            </p>
          </div>
          <div>
            <label className="block text-sm text-gray-600 mb-1">
              First Name
            </label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded">
              {user?.firstName || "N/A"}
            </p>
          </div>
          <div>
            <label className="block text-sm text-gray-600 mb-1">
              Last Name
            </label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded">
              {user?.lastName || "N/A"}
            </p>
          </div>
          <div>
            <label className="block text-sm text-gray-600 mb-1">
              Organization
            </label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded">
              {user?.organizationName || "N/A"}
            </p>
          </div>
          <div>
            <label className="block text-sm text-gray-600 mb-1">
              Organization ID
            </label>
            <p className="font-mono text-sm bg-gray-100 px-3 py-2 rounded truncate">
              {user?.organizationId || "N/A"}
            </p>
          </div>
        </div>
      </div>

      {/* Roles */}
      <div className="card mb-6">
        <h3 className="text-lg font-semibold text-gray-900 mb-4">Roles</h3>
        <div className="flex flex-wrap gap-2">
          {user?.roles.map((role) => (
            <span
              key={role}
              className="px-3 py-1 bg-blue-100 text-blue-800 text-sm rounded-full">
              {role}
            </span>
          ))}
        </div>
      </div>

      {/* Token Info (Educational) */}
      <div className="card">
        <h3 className="text-lg font-semibold text-gray-900 mb-4">
          Access Token (Decoded)
        </h3>
        <p className="text-sm text-gray-600 mb-4">
          This is the decoded JWT payload. In production, never expose tokens in
          the UI.
        </p>
        <pre className="bg-gray-900 text-green-400 p-4 rounded-lg overflow-x-auto text-xs">
          {JSON.stringify(decodedPayload, null, 2)}
        </pre>
      </div>

      {/* Actions */}
      <div className="mt-6 flex space-x-4">
        <button
          onClick={() => keycloak.accountManagement()}
          className="btn btn-secondary">
          Manage Account
        </button>
      </div>
    </div>
  );
}

export default ProfilePage;
