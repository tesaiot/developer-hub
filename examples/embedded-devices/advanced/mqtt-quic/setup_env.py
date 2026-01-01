#!/usr/bin/env python3
"""
Setup .env file from downloaded credentials and certificates
Reads mqtt-quic-config.json and creates .env for Python/C++ examples
"""

import json
import os
from pathlib import Path

def setup_env():
    """Read credentials and create .env file"""

    # Paths
    script_dir = Path(__file__).parent
    creds_dir = script_dir / "credential_certifcates"
    config_file = creds_dir / "mqtt-quic-config.json"
    ca_file = creds_dir / "ca-chain.pem"
    env_file = script_dir / ".env"

    # Check if config file exists
    if not config_file.exists():
        print(f"❌ Error: {config_file} not found")
        print(f"   Please download credentials from UI first")
        return False

    # Read configuration
    with open(config_file, 'r') as f:
        config = json.load(f)

    # Extract values
    mqtt_host = config.get('host', 'mqtt.tesaiot.com')
    mqtt_port = config.get('port', 14567)
    mqtt_username = config['auth']['username']
    mqtt_password = config['auth']['password']
    ca_cert_path = str(creds_dir / config['tls']['ca_file'])

    # Verify CA file exists
    if not ca_file.exists():
        print(f"❌ Error: {ca_file} not found")
        return False

    # Create .env content
    env_content = f"""# MQTT over QUIC Configuration
# Auto-generated from credential_certifcates/mqtt-quic-config.json
# DO NOT commit this file to git (contains password)

# MQTT Broker Settings
MQTT_HOST={mqtt_host}
MQTT_PORT={mqtt_port}
MQTT_TRANSPORT=quic

# Authentication (Server-TLS mode)
MQTT_USERNAME={mqtt_username}
MQTT_PASSWORD={mqtt_password}

# TLS/SSL Settings
MQTT_TLS_ENABLED=true
MQTT_TLS_VERSION=1.3
MQTT_CA_CERT={ca_cert_path}

# Connection Settings
MQTT_KEEPALIVE=60
MQTT_CLEAN_SESSION=true
MQTT_CONNECT_TIMEOUT=10

# Topics
MQTT_TOPIC_TELEMETRY=devices/{mqtt_username}/telemetry
MQTT_TOPIC_COMMAND=devices/{mqtt_username}/command
MQTT_TOPIC_STATUS=devices/{mqtt_username}/status
"""

    # Write .env file
    with open(env_file, 'w') as f:
        f.write(env_content)

    print(f"✅ .env file created successfully: {env_file}")
    print(f"\n📋 Configuration Summary:")
    print(f"   Host: {mqtt_host}")
    print(f"   Port: {mqtt_port} (UDP)")
    print(f"   Transport: QUIC")
    print(f"   Username: {mqtt_username}")
    print(f"   Password: {'*' * len(mqtt_password)}")
    print(f"   CA Certificate: {ca_cert_path}")
    print(f"\n🔒 Security:")
    print(f"   - TLS 1.3 (mandatory in QUIC)")
    print(f"   - Server-TLS authentication")
    print(f"   - Password encrypted in transit")

    # Create .env.example (without password)
    env_example = env_content.replace(mqtt_password, "YOUR_PASSWORD_HERE")
    env_example_file = script_dir / ".env.example"
    with open(env_example_file, 'w') as f:
        f.write(env_example)
    print(f"\n✅ .env.example created (safe to commit)")

    return True

if __name__ == '__main__':
    success = setup_env()
    exit(0 if success else 1)
