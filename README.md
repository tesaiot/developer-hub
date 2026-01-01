# TESAIoT Platform - Open Source Examples

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-TESAIoT-green.svg)](https://www.tesaiot.com)

Official integration examples for TESAIoT Platform - a secure, enterprise-grade IoT platform for device management, telemetry, and AI-powered analytics.

## Examples Overview

| Example | Language | Description | Difficulty |
|---------|----------|-------------|------------|
| [python-cli](./python-cli/) | Python | REST API client with CLI interface | Beginner |
| [device-mtls](./device-mtls/) | C | Mutual TLS device authentication | Intermediate |
| [device-servertls](./device-servertls/) | C | Server TLS with API key auth | Intermediate |
| [react-dashboard](./react-dashboard/) | React/TS | Edge AI visualization dashboard | Intermediate |
| [mqtt-quic](./mqtt-quic/) | C/Python | MQTT over QUIC protocol | Advanced |
| [nodered-integration](./nodered-integration/) | Node-RED | Visual workflow integration | Beginner |
| [wss-mqtt-streaming](./wss-mqtt-streaming/) | Node.js/Python | Real-time MQTT via WebSocket | Intermediate |
| [ai-service-template](./ai-service-template/) | Python/FastAPI | Third-party AI service template | Advanced |
| [live-streaming-dashboard](./live-streaming-dashboard/) | React/TS | Real-time MQTT dashboard | Advanced |

## Quick Start

### 1. Get Your Credentials

1. Sign up at [admin.tesaiot.com](https://admin.tesaiot.com)
2. Create an organization
3. Generate API Key (for REST API)
4. Generate MQTT Token (for real-time streaming)

### 2. Choose an Example

```bash
# Clone the repository
git clone https://github.com/tesaiot/developer-hub.git
cd developer-hub/examples

# Navigate to your chosen example
cd python-cli

# Follow the example's README
cat README.md
```

### 3. Configure and Run

Each example includes:
- `.env.example` - Configuration template
- `README.md` - Quick start guide
- `HOWTO.md` - Detailed tutorial
- `ARCHITECTURE.md` - System design explanation

## Platform Features

TESAIoT Platform provides:

- **Device Management** - Register, configure, and monitor IoT devices
- **Secure Connectivity** - mTLS, TLS 1.3, MQTT over QUIC
- **Real-time Telemetry** - Sub-second data streaming via MQTT/WebSocket
- **AI/ML Integration** - Edge AI inference and anomaly detection
- **Certificate Management** - Automated PKI with Vault integration
- **API Gateway** - Rate limiting, authentication, routing via APISIX

## Security Standards

- ETSI EN 303 645 (Consumer IoT Security)
- ISO/IEC 27402 (IoT Security Guidelines)
- TLS 1.2+ enforced for all connections
- Zero-trust architecture

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.

### Contribution Areas

- Bug fixes and improvements
- New example implementations
- Documentation enhancements
- Translations

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](./LICENSE) file for details.

### Attribution Requirement

If you use this code, please include attribution to TESAIoT Platform. See [NOTICE](./NOTICE) for details.

**Example attribution:**
```markdown
Built with [TESAIoT Platform Examples](https://github.com/tesaiot/developer-hub/tree/main/examples)
Copyright 2025 TESAIoT Platform by TESA
```

## Support

- **Issues**: [GitHub Issues](https://github.com/tesaiot/developer-hub/issues)
- **Discussions**: [GitHub Discussions](https://github.com/tesaiot/developer-hub/discussions)


---

**Built with security in mind for the IoT future.**

Copyright 2025 TESAIoT Platform by TESA
