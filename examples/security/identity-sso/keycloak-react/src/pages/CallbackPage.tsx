import { useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "../hooks/useAuth";
import LoadingSpinner from "../components/LoadingSpinner";

/**
 * Callback Page Component
 *
 * Handles the OAuth callback after authentication.
 * Redirects to dashboard once authenticated.
 */
function CallbackPage() {
  const { isAuthenticated, isLoading } = useAuth();
  const navigate = useNavigate();

  useEffect(() => {
    if (!isLoading && isAuthenticated) {
      navigate("/dashboard", { replace: true });
    }
  }, [isAuthenticated, isLoading, navigate]);

  return (
    <div className="min-h-[60vh] flex items-center justify-center">
      <LoadingSpinner message="Completing authentication..." />
    </div>
  );
}

export default CallbackPage;
