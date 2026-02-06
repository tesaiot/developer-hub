interface LoadingSpinnerProps {
  message?: string;
  size?: "sm" | "md" | "lg";
}

/**
 * Loading Spinner Component
 *
 * Displays a loading spinner with optional message.
 */
function LoadingSpinner({ message, size = "md" }: LoadingSpinnerProps) {
  const sizeClasses = {
    sm: "w-6 h-6",
    md: "w-10 h-10",
    lg: "w-16 h-16",
  };

  return (
    <div className="flex flex-col items-center justify-center space-y-4">
      <div
        className={`${sizeClasses[size]} border-4 border-blue-200 border-t-blue-600 rounded-full animate-spin`}
      />
      {message && <p className="text-gray-600 text-sm">{message}</p>}
    </div>
  );
}

export default LoadingSpinner;
