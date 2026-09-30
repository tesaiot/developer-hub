# i11 — System Status Panel

Production-quality system status dashboard displayed as a 2-column grid of
color-coded cards. Each card shows a system metric with a status indicator
dot (green=OK, yellow=warning, red=error).

## Status Cards

| Card    | Data Source                        | Warning Threshold    | Error Threshold |
|---------|------------------------------------|----------------------|-----------------|
| CPU     | Simulated from tick count          | > 60%                | > 80%          |
| Heap    | `xPortGetFreeHeapSize()`           | < 16 KB free         | < 4 KB free    |
| Uptime  | `xTaskGetTickCount()`              | Always green         | —              |
| Sensors | BSP-guarded sensor count           | Some unavailable     | —              |
| BMI270  | `ipc_sensorhub_snapshot()` IMU     | No data yet          | —              |
| WiFi    | `ipc_sensorhub_wifi_connected()`   | Not connected        | —              |

## Key Techniques

- `example_card_create()` helper for consistent card styling
- `xPortGetFreeHeapSize()` for real FreeRTOS heap monitoring
- BSP feature guards to count available sensors per board
- WiFi state via IPC (WiFi runs on CM33_NS, not CM55)
- 500ms update interval for smooth monitoring
