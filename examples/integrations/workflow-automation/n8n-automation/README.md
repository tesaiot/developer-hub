# n8n Workflow Automation for TESAIoT

Pre-built n8n workflows for automating IoT device management and alerting.

---

## Overview

[n8n](https://n8n.io/) is a free and open workflow automation tool. These templates help you:

- Monitor device telemetry and trigger alerts
- Integrate with Slack, Email, PagerDuty, and other services
- Automate device provisioning and configuration
- Create custom dashboards and reports

## Prerequisites

### n8n Installation

```bash
# Docker (recommended)
docker run -d --name n8n \
  -p 5678:5678 \
  -v ~/.n8n:/home/node/.n8n \
  n8nio/n8n

# npm
npm install n8n -g
n8n start
```

### TESAIoT Configuration

1. Create API credentials in TESAIoT Admin UI
2. Note your API Key and base URL

## Available Workflows

| Workflow | Description | Triggers |
|----------|-------------|----------|
| [device-alert-slack](./workflows/device-alert-slack.json) | Alert on anomaly detection | MQTT/Webhook |
| [telemetry-to-sheets](./workflows/telemetry-to-sheets.json) | Log telemetry to Google Sheets | Schedule |
| [device-provisioning](./workflows/device-provisioning.json) | Automated device setup | HTTP Request |
| [daily-report](./workflows/daily-report.json) | Daily summary email | Schedule |

## Quick Start

### 1. Import Workflow

1. Open n8n UI at `http://localhost:5678`
2. Click **Workflows** → **Import from File**
3. Select a workflow JSON from `./workflows/`

### 2. Configure Credentials

1. Open the imported workflow
2. Click on nodes with warning icons
3. Add your TESAIoT API credentials:
   - **API Key**: From TESAIoT Admin UI
   - **Base URL**: `https://api.tesaiot.com`

### 3. Activate Workflow

1. Click the **Active** toggle
2. Test with **Execute Workflow**

## Workflow Details

### Device Alert Slack

Sends Slack notifications when device anomaly is detected.

**Trigger:** Webhook from TESAIoT
**Actions:**
1. Parse anomaly event
2. Lookup device info
3. Format Slack message
4. Send to channel

**Required Credentials:**
- TESAIoT API Key
- Slack Webhook URL

### Telemetry to Sheets

Logs hourly telemetry summaries to Google Sheets.

**Trigger:** Schedule (every hour)
**Actions:**
1. Fetch devices list
2. Get latest telemetry
3. Append to Google Sheet

**Required Credentials:**
- TESAIoT API Key
- Google OAuth

### Device Provisioning

Automates new device setup via HTTP trigger.

**Trigger:** HTTP Request
**Actions:**
1. Create device in TESAIoT
2. Generate credentials
3. Send email with setup instructions

**Required Credentials:**
- TESAIoT API Key
- SMTP Email

## Custom Workflows

### TESAIoT API Endpoints

Use these in your custom n8n workflows:

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/devices` | GET | List all devices |
| `/api/v1/devices/{id}` | GET | Device details |
| `/api/v1/devices/{id}/telemetry` | GET | Device telemetry |
| `/api/v1/devices/{id}/commands` | POST | Send command |

### Example HTTP Request Node

```json
{
  "method": "GET",
  "url": "https://api.tesaiot.com/api/v1/devices",
  "headers": {
    "X-API-KEY": "{{ $credentials.apiKey }}"
  }
}
```

## Docker Compose Setup

For production deployment with persistence:

```yaml
# docker-compose.yml
version: '3.8'
services:
  n8n:
    image: n8nio/n8n
    ports:
      - "5678:5678"
    environment:
      - N8N_BASIC_AUTH_ACTIVE=true
      - N8N_BASIC_AUTH_USER=admin
      - N8N_BASIC_AUTH_PASSWORD=changeme
      - WEBHOOK_URL=https://n8n.yourdomain.com/
    volumes:
      - n8n_data:/home/node/.n8n

volumes:
  n8n_data:
```

## Troubleshooting

### Webhook Not Receiving Events
- Verify n8n is accessible from TESAIoT
- Check webhook URL is correct
- Review n8n execution logs

### API Authentication Failed
- Verify API Key is correct
- Check API Key has required permissions
- Confirm base URL format

### Google Sheets Permission Denied
- Re-authorize Google OAuth
- Check sheet sharing permissions

## Security Notes

- Store credentials in n8n's credential manager
- Use HTTPS for webhook endpoints
- Restrict n8n access with basic auth
- Rotate API keys periodically

---

**Category:** Workflow Automation
**Last Updated:** 2025-12-27
