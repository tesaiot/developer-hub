import { Link } from "react-router-dom";
import { useAuth } from "../hooks/useAuth";

/**
 * Header Component
 *
 * Navigation header with user menu and authentication actions.
 */
function Header() {
  const { isAuthenticated, user, login, logout } = useAuth();

  return (
    <header className="bg-white shadow-sm border-b border-gray-200">
      <div className="container mx-auto px-4">
        <div className="flex items-center justify-between h-16">
          {/* Logo */}
          <Link to="/" className="flex items-center space-x-2">
            <div className="w-8 h-8 bg-blue-600 rounded-lg flex items-center justify-center">
              <span className="text-white font-bold text-sm">T</span>
            </div>
            <span className="font-semibold text-gray-900">TESAIoT</span>
          </Link>

          {/* Navigation */}
          <nav className="hidden md:flex items-center space-x-6">
            <Link
              to="/"
              className="text-gray-600 hover:text-gray-900 transition-colors">
              Home
            </Link>
            {isAuthenticated && (
              <>
                <Link
                  to="/dashboard"
                  className="text-gray-600 hover:text-gray-900 transition-colors">
                  Dashboard
                </Link>
                <Link
                  to="/devices"
                  className="text-gray-600 hover:text-gray-900 transition-colors">
                  Devices
                </Link>
              </>
            )}
          </nav>

          {/* User Menu */}
          <div className="flex items-center space-x-4">
            {isAuthenticated ? (
              <div className="flex items-center space-x-4">
                <Link
                  to="/profile"
                  className="flex items-center space-x-2 text-gray-700 hover:text-gray-900">
                  <div className="w-8 h-8 bg-blue-100 rounded-full flex items-center justify-center">
                    <span className="text-blue-600 font-medium text-sm">
                      {user?.name?.charAt(0).toUpperCase() || "U"}
                    </span>
                  </div>
                  <span className="hidden sm:inline text-sm font-medium">
                    {user?.name}
                  </span>
                </Link>
                <button
                  onClick={() => logout()}
                  className="btn btn-secondary text-sm">
                  Logout
                </button>
              </div>
            ) : (
              <button onClick={() => login()} className="btn btn-primary">
                Login with SSO
              </button>
            )}
          </div>
        </div>
      </div>
    </header>
  );
}

export default Header;
