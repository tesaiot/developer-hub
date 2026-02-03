import { Link } from "react-router-dom";
import { useAuth } from "../hooks/useAuth";

/**
 * Home Page Component
 *
 * Public landing page with SSO login option.
 */
function HomePage() {
  const { isAuthenticated, login, user } = useAuth();

  return (
    <div className="max-w-4xl mx-auto">
      {/* Hero Section */}
      <div className="text-center py-12">
        <h1 className="text-4xl font-bold text-gray-900 mb-4">
          TESAIoT SSO Example
        </h1>
        <p className="text-xl text-gray-600 mb-8">
          Single Sign-On integration with Keycloak Identity Service
        </p>

        {isAuthenticated ? (
          <div className="space-y-4">
            <p className="text-lg text-green-600">
              ✓ Welcome back, {user?.name}!
            </p>
            <div className="flex justify-center space-x-4">
              <Link to="/dashboard" className="btn btn-primary">
                Go to Dashboard
              </Link>
              <Link to="/profile" className="btn btn-secondary">
                View Profile
              </Link>
            </div>
          </div>
        ) : (
          <button
            onClick={() => login()}
            className="btn btn-primary text-lg px-8 py-3">
            Login with SSO
          </button>
        )}
      </div>

      {/* Features Section */}
      <div className="grid md:grid-cols-3 gap-6 py-12">
        <div className="card">
          <div className="w-12 h-12 bg-blue-100 rounded-lg flex items-center justify-center mb-4">
            <svg
              className="w-6 h-6 text-blue-600"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24">
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z"
              />
            </svg>
          </div>
          <h3 className="text-lg font-semibold text-gray-900 mb-2">
            Secure Authentication
          </h3>
          <p className="text-gray-600">
            OAuth 2.0 / OpenID Connect with PKCE for maximum security.
          </p>
        </div>

        <div className="card">
          <div className="w-12 h-12 bg-green-100 rounded-lg flex items-center justify-center mb-4">
            <svg
              className="w-6 h-6 text-green-600"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24">
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z"
              />
            </svg>
          </div>
          <h3 className="text-lg font-semibold text-gray-900 mb-2">
            Single Sign-On
          </h3>
          <p className="text-gray-600">
            One login for all TESAIoT services and applications.
          </p>
        </div>

        <div className="card">
          <div className="w-12 h-12 bg-purple-100 rounded-lg flex items-center justify-center mb-4">
            <svg
              className="w-6 h-6 text-purple-600"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24">
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z"
              />
            </svg>
          </div>
          <h3 className="text-lg font-semibold text-gray-900 mb-2">
            Role-Based Access
          </h3>
          <p className="text-gray-600">
            Fine-grained permissions with realm and client roles.
          </p>
        </div>
      </div>

      {/* How It Works */}
      <div className="py-12">
        <h2 className="text-2xl font-bold text-gray-900 mb-6 text-center">
          How It Works
        </h2>
        <div className="card">
          <ol className="space-y-4">
            <li className="flex items-start">
              <span className="flex-shrink-0 w-8 h-8 bg-blue-600 text-white rounded-full flex items-center justify-center text-sm font-medium mr-4">
                1
              </span>
              <div>
                <h4 className="font-medium text-gray-900">Click Login</h4>
                <p className="text-gray-600">
                  User clicks the "Login with SSO" button
                </p>
              </div>
            </li>
            <li className="flex items-start">
              <span className="flex-shrink-0 w-8 h-8 bg-blue-600 text-white rounded-full flex items-center justify-center text-sm font-medium mr-4">
                2
              </span>
              <div>
                <h4 className="font-medium text-gray-900">
                  Redirect to Keycloak
                </h4>
                <p className="text-gray-600">
                  Browser redirects to TESAIoT Keycloak login page
                </p>
              </div>
            </li>
            <li className="flex items-start">
              <span className="flex-shrink-0 w-8 h-8 bg-blue-600 text-white rounded-full flex items-center justify-center text-sm font-medium mr-4">
                3
              </span>
              <div>
                <h4 className="font-medium text-gray-900">Authenticate</h4>
                <p className="text-gray-600">
                  Enter credentials or use social login (Google, GitHub)
                </p>
              </div>
            </li>
            <li className="flex items-start">
              <span className="flex-shrink-0 w-8 h-8 bg-blue-600 text-white rounded-full flex items-center justify-center text-sm font-medium mr-4">
                4
              </span>
              <div>
                <h4 className="font-medium text-gray-900">Receive Tokens</h4>
                <p className="text-gray-600">
                  App receives JWT tokens for API authentication
                </p>
              </div>
            </li>
          </ol>
        </div>
      </div>
    </div>
  );
}

export default HomePage;
