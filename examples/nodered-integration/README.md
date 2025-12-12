# TESAIoT Node-RED Integration Guide

> **English version** - For Thai version, see [README-TH.md](README-TH.md)

This project shows how to build open-source friendly Node-RED nodes and FlowFuse Dashboard pages styled after the TESAIoT Admin UI, calling the platform through the APISIX API Gateway.

---

## 1. Project Layout

```
nodered-to-platform/
├── Dockerfile                  # Multi-stage build for reproducible images
├── README.md                   # This guide (English)
├── README-TH.md                # Thai version
├── docker-compose.yml          # Sample stack with health-checks
├── package.json                # Node-RED project manifest
├── settings.js                 # Runtime config (API defaults, admin path)
├── tsconfig.json               # TypeScript compiler options
├── .env.example                # Copy to `.env` and fill API key
├── run.sh                      # Bootstrap/build/start helper
├── flows/
│   └── tesaiot-flow.json       # Production-style dashboard
└── src/
    ├── lib/client.ts           # Axios client with TLS guards
    └── nodes/                  # Custom TESAIoT nodes
        ├── tesaiot-api-gateway.ts
        ├── tesaiot-device-lists.ts
        ├── tesaiot-device-profile.ts
        ├── tesaiot-device-data.ts
        └── tesaiot-api-usage.ts
```

---

## 2. Custom Node Blueprint

- Each node is authored in TypeScript, compiled to `dist/nodes/*.js`, and heavily commented to explain Why/What/How for intermediate developers.
- Runtime is stateless: every input message describes the work, making debugging with the Node-RED sidebar trivial.
- The config node caches an Axios client with retry/backoff so downstream nodes simply call `gatewayNode.getClient()`.

---

## 3. Getting Started

### 3.1 Local Development

1. `cp .env.example .env` then fill `TESAIOT_API_KEY` and optional overrides.
2. Install dependencies: `./run.sh bootstrap`
3. Build TypeScript nodes: `./run.sh build`
4. Start Node-RED: `./run.sh start`
5. Import `flows/tesaiot-flow.json` via the Node-RED editor or copy into `settings.js` flow path.

### 3.2 Docker Workflow

1. Copy `.env.example` → `.env` and set credentials as above.
2. Build and launch: `./run.sh compose up`
3. Stop and clean: `./run.sh compose down`

---

## 4. Dashboard Walkthrough

| Section | Description |
| --- | --- |
| Device Snapshot | Displays device totals and the first 25 rows for quick health checks |
| Device Profile | Shows profile fields for the first device in the list |
| Device Telemetry | Renders moving averages and a recent-history table |
| API Usage | Reports API request/minute, secure fleet percentage, and last telemetry timestamp |

> Styling tweaks live in `theme/custom.css`, keeping Tailwind-inspired tokens available without requiring the Tailwind toolchain.

---

## 5. Troubleshooting

- **gRPC/WebSocket issues** – ensure ports 1880 (Node-RED) and 1883/8883 (MQTT) are reachable; the flow defaults to the production endpoints.
- **Flow remains blank** – re-import `tesaiot-flow.json` after pulling updates; the `ui-template` widgets rely on `msg.payload`.
- **TypeScript build fails** – run `npm install` again and confirm Node ≥ 18.18; the repo uses `esbuild` within the build script.

---

## 6. Further Reading

- [Node-RED Creating Nodes](https://nodered.org/docs/creating-nodes/) – official guide for packaging custom nodes
- [FlowFuse Dashboard Docs](https://dashboard.flowfuse.com/) – component reference and layout best practices
- [TESAIoT Admin UI](https://admin.tesaiot.com/) – authenticate to download credential bundles used throughout this sample

---

> Contributions are welcome—open an issue or PR if you spot improvements.
