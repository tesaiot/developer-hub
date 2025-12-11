/**
 * ConnectionPanel Component
 *
 * MQTT connection management panel with token input and status display.
 * Handles WebSocket Secure (WSS) MQTT connections to TESAIoT Platform.
 *
 * Features:
 * - Token input field with validation
 * - Connect/Disconnect button
 * - Connection status indicator (LED style)
 * - Error message display
 *
 * Token Format:
 * MQTT API tokens follow the pattern: tesa_mqtt_<org>_<32chars>
 * Example: tesa_mqtt_acme-corp_a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
 *
 * @example
 * ```tsx
 * <ConnectionPanel
 *   token={token}
 *   onTokenChange={setToken}
 *   status="disconnected"
 *   error={null}
 *   onConnect={handleConnect}
 * />
 * ```
 */

import type { ConnectionStatus } from '../../types';

interface ConnectionPanelProps {
  /** Current MQTT API token value */
  token: string;
  /** Callback when token input changes */
  onTokenChange: (token: string) => void;
  /** Current connection status */
  status: ConnectionStatus;
  /** Error message if connection failed */
  error: string | null;
  /** Callback for connect/disconnect button click */
  onConnect: () => void;
}

/**
 * Get status indicator color based on connection state
 * - Green pulse: Connected
 * - Yellow pulse: Connecting
 * - Red: Disconnected/Error
 */
function getStatusColor(status: ConnectionStatus): string {
  switch (status) {
    case 'connected':
      return 'bg-green-500';
    case 'connecting':
      return 'bg-yellow-500';
    case 'disconnected':
    case 'error':
    default:
      return 'bg-red-500';
  }
}

/**
 * Get human-readable status text
 */
function getStatusText(status: ConnectionStatus): string {
  switch (status) {
    case 'connected':
      return 'Connected';
    case 'connecting':
      return 'Connecting...';
    case 'disconnected':
      return 'Disconnected';
    case 'error':
      return 'Error';
    default:
      return 'Unknown';
  }
}

/**
 * Validate MQTT token format
 * Expected format: tesa_mqtt_<org>_<32chars>
 */
function isValidTokenFormat(token: string): boolean {
  if (!token || token.length < 20) return false;
  // Basic format check: starts with tesa_mqtt_ and has reasonable length
  return token.startsWith('tesa_mqtt_') && token.length >= 45;
}

export function ConnectionPanel({
  token,
  onTokenChange,
  status,
  error,
  onConnect,
}: ConnectionPanelProps) {
  const isConnected = status === 'connected';
  const isConnecting = status === 'connecting';
  const canConnect = !isConnecting && (isConnected || isValidTokenFormat(token));

  return (
    <div className="bg-gray-800 rounded-lg p-4">
      <div className="flex items-center justify-between mb-4">
        <h2 className="text-lg font-semibold">MQTT Connection</h2>

        {/* Status Indicator */}
        <div className="flex items-center gap-2">
          <div
            className={`w-3 h-3 rounded-full ${getStatusColor(status)} ${
              status === 'connected' || status === 'connecting' ? 'animate-pulse' : ''
            }`}
          />
          <span className="text-sm text-gray-400">{getStatusText(status)}</span>
        </div>
      </div>

      {/* Token Input */}
      <div className="space-y-4">
        <div>
          <label htmlFor="mqtt-token" className="block text-sm text-gray-400 mb-2">
            MQTT API Token
          </label>
          <input
            id="mqtt-token"
            type="password"
            value={token}
            onChange={(e) => onTokenChange(e.target.value)}
            placeholder="tesa_mqtt_your-org_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            disabled={isConnected || isConnecting}
            className={`w-full px-4 py-2 bg-gray-900 border rounded-lg text-white placeholder-gray-500 focus:outline-none focus:ring-2 focus:ring-blue-500 ${
              isConnected || isConnecting
                ? 'border-gray-700 opacity-50 cursor-not-allowed'
                : 'border-gray-600'
            }`}
          />
          <p className="text-xs text-gray-500 mt-1">
            Get your token from{' '}
            <a
              href="https://admin.tesaiot.com"
              target="_blank"
              rel="noopener noreferrer"
              className="text-blue-400 hover:underline"
            >
              TESAIoT Admin Portal
            </a>
            {' → Settings → MQTT API Tokens'}
          </p>
        </div>

        {/* Error Message */}
        {error && (
          <div className="bg-red-900/30 border border-red-500/50 rounded-lg p-3">
            <p className="text-red-400 text-sm">{error}</p>
          </div>
        )}

        {/* Connect/Disconnect Button */}
        <button
          onClick={onConnect}
          disabled={!canConnect}
          className={`w-full py-2 px-4 rounded-lg font-medium transition-colors ${
            isConnected
              ? 'bg-red-600 hover:bg-red-700 text-white'
              : isConnecting
              ? 'bg-gray-600 text-gray-400 cursor-not-allowed'
              : canConnect
              ? 'bg-blue-600 hover:bg-blue-700 text-white'
              : 'bg-gray-700 text-gray-500 cursor-not-allowed'
          }`}
        >
          {isConnecting ? (
            <span className="flex items-center justify-center gap-2">
              <svg
                className="animate-spin h-4 w-4"
                xmlns="http://www.w3.org/2000/svg"
                fill="none"
                viewBox="0 0 24 24"
              >
                <circle
                  className="opacity-25"
                  cx="12"
                  cy="12"
                  r="10"
                  stroke="currentColor"
                  strokeWidth="4"
                />
                <path
                  className="opacity-75"
                  fill="currentColor"
                  d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
                />
              </svg>
              Connecting...
            </span>
          ) : isConnected ? (
            'Disconnect'
          ) : (
            'Connect'
          )}
        </button>
      </div>
    </div>
  );
}

export default ConnectionPanel;
