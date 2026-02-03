# TESAIoT Keycloak Flutter SSO Example

Professional Flutter application demonstrating Single Sign-On integration with TESAIoT Platform using Keycloak.

## Features

- **PKCE Flow** - Secure Authorization Code flow with PKCE
- **AppAuth Integration** - Industry-standard flutter_appauth package
- **Secure Storage** - Encrypted token storage with flutter_secure_storage
- **Platform Support** - iOS, Android, Web, macOS, Windows
- **Auto Refresh** - Automatic token refresh in background
- **Biometric Lock** - Optional biometric authentication for token access

## Quick Start

### Prerequisites

- Flutter 3.16+
- Dart 3.2+
- iOS 12+ / Android 6+
- TESAIoT Platform account with OAuth client configured

### Installation

```bash
# Get dependencies
flutter pub get

# Configure Keycloak settings
# Edit lib/config/keycloak_config.dart

# Run on device/emulator
flutter run
```

### Platform-Specific Setup

#### iOS

Add to `ios/Runner/Info.plist`:

```xml
<key>CFBundleURLTypes</key>
<array>
    <dict>
        <key>CFBundleTypeRole</key>
        <string>Editor</string>
        <key>CFBundleURLSchemes</key>
        <array>
            <string>com.tesaiot.sso</string>
        </array>
    </dict>
</array>
```

#### Android

Add to `android/app/build.gradle`:

```gradle
android {
    defaultConfig {
        manifestPlaceholders = [
            'appAuthRedirectScheme': 'com.tesaiot.sso'
        ]
    }
}
```

## Project Structure

```
keycloak-flutter/
├── lib/
│   ├── main.dart                    # App entry point
│   ├── config/
│   │   └── keycloak_config.dart     # Keycloak configuration
│   ├── models/
│   │   ├── user_model.dart          # User data model
│   │   └── token_model.dart         # Token data model
│   ├── services/
│   │   ├── auth_service.dart        # Authentication service
│   │   ├── api_service.dart         # API client
│   │   └── secure_storage.dart      # Secure token storage
│   ├── providers/
│   │   └── auth_provider.dart       # State management
│   └── screens/
│       ├── login_screen.dart        # Login UI
│       ├── home_screen.dart         # Home after login
│       ├── profile_screen.dart      # User profile
│       └── devices_screen.dart      # Device list
├── pubspec.yaml
└── README.md
```

## Configuration

### Keycloak Settings

Edit `lib/config/keycloak_config.dart`:

```dart
class KeycloakConfig {
  static const String issuer = 'https://auth.tesaiot.org/realms/tesa-iot-platform';
  static const String clientId = 'tesa-iot-web';
  static const String redirectUri = 'com.tesaiot.sso://callback';
  static const String postLogoutRedirectUri = 'com.tesaiot.sso://logout';
  static const List<String> scopes = ['openid', 'profile', 'email'];
}
```

### Keycloak Client Setup

In TESAIoT Admin Portal:

1. **Client Type**: Public
2. **Standard Flow**: Enabled
3. **Valid Redirect URIs**: `com.tesaiot.sso://callback`
4. **Valid Post Logout Redirect URIs**: `com.tesaiot.sso://logout`
5. **PKCE Code Challenge Method**: S256

## Usage Examples

### Basic Authentication

```dart
import 'package:tesaiot_sso/services/auth_service.dart';

final authService = AuthService();

// Login
await authService.login();

// Check authentication
if (authService.isAuthenticated) {
  print('User: ${authService.user?.name}');
}

// Logout
await authService.logout();
```

### Using with Provider

```dart
// In your widget
Consumer<AuthProvider>(
  builder: (context, auth, _) {
    if (auth.isLoading) {
      return CircularProgressIndicator();
    }

    if (auth.isAuthenticated) {
      return HomeScreen(user: auth.user!);
    }

    return LoginScreen();
  },
)
```

### Making Authenticated API Calls

```dart
import 'package:tesaiot_sso/services/api_service.dart';

final apiService = ApiService();

// Get devices
final devices = await apiService.getDevices();

// Create device
await apiService.createDevice({
  'name': 'New Sensor',
  'device_id': 'sensor-001',
});
```

## Security Best Practices

| Practice                | Implementation                              |
| ----------------------- | ------------------------------------------- |
| **PKCE**                | Always enabled for mobile apps              |
| **Secure Storage**      | Encrypted with flutter_secure_storage       |
| **Token Refresh**       | Auto-refresh before expiry                  |
| **Biometric Auth**      | Optional biometric for sensitive operations |
| **Certificate Pinning** | Recommended for production                  |

---

**Category:** Security / Identity  
**Platforms:** iOS, Android, Web, macOS, Windows  
**Last Updated:** 2026-01-29
