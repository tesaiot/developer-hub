# TESAIoT Keycloak React SSO Example

Professional React application demonstrating Single Sign-On integration with TESAIoT Platform using Keycloak.

## Features

- **PKCE Flow** - Secure Authorization Code flow with PKCE
- **Token Management** - Automatic token refresh and secure storage
- **Protected Routes** - Route guards with role-based access
- **User Profile** - Display user information from ID token
- **API Integration** - Authenticated API calls to TESAIoT Core API
- **TypeScript** - Full type safety

## Quick Start

### Prerequisites

- Node.js 18+ or 20+
- npm or yarn
- TESAIoT Platform account with OAuth client configured

### Installation

```bash
# Install dependencies
npm install

# Copy environment configuration
cp .env.example .env

# Edit .env with your Keycloak client settings
# VITE_KEYCLOAK_URL=https://auth.tesaiot.org
# VITE_KEYCLOAK_REALM=tesa-iot-platform
# VITE_KEYCLOAK_CLIENT_ID=tesa-iot-web

# Start development server
npm run dev
```

Open [http://localhost:5173](http://localhost:5173) in your browser.

### Production Build

```bash
npm run build
npm run preview
```

## Project Structure

```
keycloak-react/
├── src/
│   ├── components/           # Reusable UI components
│   │   ├── Header.tsx        # Navigation with user menu
│   │   ├── LoginButton.tsx   # SSO login trigger
│   │   ├── LogoutButton.tsx  # SSO logout with redirect
│   │   ├── ProtectedRoute.tsx # Route guard HOC
│   │   └── UserProfile.tsx   # User info display
│   ├── contexts/
│   │   └── AuthContext.tsx   # Authentication state provider
│   ├── hooks/
│   │   ├── useAuth.ts        # Auth context hook
│   │   └── useApi.ts         # Authenticated API hook
│   ├── pages/
│   │   ├── HomePage.tsx      # Public landing page
│   │   ├── DashboardPage.tsx # Protected dashboard
│   │   ├── DevicesPage.tsx   # Device list (requires role)
│   │   └── CallbackPage.tsx  # OAuth callback handler
│   ├── services/
│   │   ├── keycloak.ts       # Keycloak configuration
│   │   └── api.ts            # TESAIoT API client
│   ├── types/
│   │   └── auth.ts           # TypeScript types
│   ├── App.tsx               # Main application
│   └── main.tsx              # Entry point
├── .env.example              # Environment template
├── package.json
├── tsconfig.json
└── vite.config.ts
```

## Configuration

### Environment Variables

| Variable                  | Description         | Example                            |
| ------------------------- | ------------------- | ---------------------------------- |
| `VITE_KEYCLOAK_URL`       | Keycloak server URL | `https://auth.tesaiot.org`         |
| `VITE_KEYCLOAK_REALM`     | Keycloak realm name | `tesa-iot-platform`                |
| `VITE_KEYCLOAK_CLIENT_ID` | OAuth client ID     | `tesa-iot-web`                     |
| `VITE_API_BASE_URL`       | TESAIoT API URL     | `https://admin.tesaiot.com/api/v1` |

### Keycloak Client Setup

In TESAIoT Admin Portal (or Keycloak Admin Console):

1. **Client Type**: Public
2. **Standard Flow**: Enabled
3. **Direct Access Grants**: Disabled
4. **Valid Redirect URIs**:
   - `http://localhost:5173/*` (development)
   - `https://your-app.com/*` (production)
5. **Web Origins**:
   - `http://localhost:5173`
   - `https://your-app.com`
6. **PKCE Code Challenge Method**: S256

## Usage Examples

### Basic Login/Logout

```tsx
import { useAuth } from "./hooks/useAuth";

function LoginButton() {
  const { login, logout, isAuthenticated, user } = useAuth();

  if (isAuthenticated) {
    return (
      <div>
        <span>Welcome, {user?.name}</span>
        <button onClick={logout}>Logout</button>
      </div>
    );
  }

  return <button onClick={login}>Login with SSO</button>;
}
```

### Protected Route

```tsx
import { ProtectedRoute } from "./components/ProtectedRoute";

function App() {
  return (
    <Routes>
      <Route path="/" element={<HomePage />} />
      <Route
        path="/dashboard"
        element={
          <ProtectedRoute>
            <DashboardPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/devices"
        element={
          <ProtectedRoute requiredRoles={["device-manager"]}>
            <DevicesPage />
          </ProtectedRoute>
        }
      />
    </Routes>
  );
}
```

### Authenticated API Call

```tsx
import { useApi } from "./hooks/useApi";

function DevicesList() {
  const { fetchWithAuth, loading, error } = useApi();
  const [devices, setDevices] = useState([]);

  useEffect(() => {
    async function loadDevices() {
      const data = await fetchWithAuth("/devices");
      setDevices(data);
    }
    loadDevices();
  }, []);

  if (loading) return <Spinner />;
  if (error) return <Error message={error} />;

  return (
    <ul>
      {devices.map((device) => (
        <li key={device.id}>{device.name}</li>
      ))}
    </ul>
  );
}
```

## Security Best Practices

| Practice            | Implementation                     |
| ------------------- | ---------------------------------- |
| **PKCE**            | Enabled by default with S256       |
| **Token Storage**   | In-memory only (not localStorage)  |
| **Silent Refresh**  | Automatic token refresh via iframe |
| **CSRF Protection** | State parameter validation         |
| **Secure Logout**   | Server-side session termination    |

## Troubleshooting

### CORS Errors

Ensure your Keycloak client has the correct Web Origins configured.

### Token Refresh Issues

Check that your Keycloak client has refresh tokens enabled and the token lifespan is appropriate.

### Invalid Redirect URI

Verify that your redirect URI matches exactly (including trailing slashes).

---

**Category:** Security / Identity  
**Last Updated:** 2026-01-29
