# n8n Automation Architecture

## Overview

Pre-built n8n workflows for automating TESAIoT Platform integrations with external services.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        n8n Platform                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Workflow Engine                       │   │
│  └──────────────────────────┬───────────────────────────────┘   │
│                             │                                   │
│  ┌──────────────────────────┼─────────────────────────────────┐ │
│  │                          │                                 │ │
│  │  ┌─────────┐    ┌────────┴───────┐    ┌─────────┐          │ │
│  │  │ Trigger │───▶│   Processing   │───▶│ Action  │          │ │
│  │  │  Nodes  │    │     Nodes      │    │  Nodes  │          │ │
│  │  └─────────┘    └────────────────┘    └─────────┘          │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
        │                                              │
        │  Trigger (Webhook/Schedule)                  │  Actions
        ▼                                              ▼
┌──────────────────┐                      ┌──────────────────┐
│  TESAIoT API     │                      │  External        │
│  (REST/Webhook)  │                      │  - Slack         │
│                  │                      │  - Email         │
│                  │                      │  - Google Sheets │
└──────────────────┘                      └──────────────────┘
```

## Workflow: Device Alert to Slack

```
┌─────────────────────────────────────────────────────────────────┐
│                 device-alert-slack.json                         │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Webhook    │───▶│     IF       │───▶│   Format     │───▶│    Slack     │
│   Trigger    │    │ temp > 35?   │    │  Message     │    │   Send       │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
      │                    │
      │                    │ No
      │                    ▼
      │            ┌──────────────┐
      │            │    Debug     │
      │            │  (log only)  │
      │            └──────────────┘
      │
      ▼
Incoming webhook from TESAIoT:
{
  "device_id": "sensor-001",
  "event": "temperature_alert",
  "data": {
    "temperature": 38.5,
    "threshold": 35
  }
}
```

## Workflow: Telemetry to Google Sheets

```
┌─────────────────────────────────────────────────────────────────┐
│                 telemetry-to-sheets.json                        │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Schedule   │───▶│ TESAIoT API  │───▶│  Transform   │───▶│Google Sheets │
│  (hourly)    │    │ Get Devices  │    │   Data       │    │   Append     │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │  Loop Over   │
                    │   Devices    │
                    └──────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │ Get Latest   │
                    │  Telemetry   │
                    └──────────────┘
```

## Directory Structure

```
n8n-automation/
├── workflows/
│   ├── device-alert-slack.json      # Alert to Slack
│   ├── telemetry-to-sheets.json     # Log to Sheets
│   └── daily-report-email.json      # Daily summary
├── credentials/
│   └── .env.example                 # Credential template
└── README.md
```

## Importing Workflows

```bash
# Using n8n CLI
n8n import:workflow --input=workflows/device-alert-slack.json

# Using n8n UI
1. Go to Workflows > Import from file
2. Select JSON file
3. Configure credentials
4. Activate workflow
```

## TESAIoT API Integration

```
┌─────────────────────────────────────────────────────────────────┐
│                     n8n HTTP Request Node                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Method: GET                                                    │
│  URL: https://admin.tesaiot.com/api/v1/devices                  │
│                                                                 │
│  Headers:                                                       │
│    X-API-Key: {{ $credentials.tesaiot.apiKey }}                 │
│                                                                 │
│  Query Parameters:                                              │
│    organization_id: {{ $credentials.tesaiot.orgId }}            │
│    limit: 100                                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Webhook Setup

```
1. In TESAIoT Admin Portal:
   - Go to Settings > Webhooks
   - Add new webhook
   - URL: https://your-n8n.com/webhook/tesaiot-alert
   - Events: device.alert, telemetry.threshold

2. In n8n:
   - Add Webhook trigger node
   - Copy webhook URL
   - Set authentication (if required)
```

## Available Workflows

| Workflow | Trigger | Actions |
|----------|---------|---------|
| device-alert-slack | Webhook | Slack notification |
| telemetry-to-sheets | Schedule (1h) | Google Sheets append |
| daily-report-email | Schedule (daily) | Email summary |
| device-offline-pagerduty | Webhook | PagerDuty incident |

## Credentials Required

| Service | Credential Type |
|---------|-----------------|
| TESAIoT | API Key |
| Slack | OAuth / Webhook |
| Google Sheets | OAuth |
| Email | SMTP / OAuth |
