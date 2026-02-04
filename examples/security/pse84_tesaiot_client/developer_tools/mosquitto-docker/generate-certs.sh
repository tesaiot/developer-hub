#!/bin/bash
# Generate self-signed certificates for Mosquitto TLS testing
# Run this script from the mosquitto-docker directory

CERTS_DIR="./certs"
mkdir -p "$CERTS_DIR"

# Get the local machine's IP address (for SAN)
LOCAL_IP=$(ipconfig getifaddr en0 2>/dev/null || echo "192.168.1.100")
echo "Using local IP: $LOCAL_IP"

# Certificate validity (days)
DAYS=365

# Subject for CA
CA_SUBJECT="/C=TH/ST=Bangkok/L=Bangkok/O=Test MQTT CA/CN=Test Root CA"

# Subject for Server
SERVER_SUBJECT="/C=TH/ST=Bangkok/L=Bangkok/O=Test MQTT Server/CN=localhost"

echo "=== Generating CA (Certificate Authority) ==="
openssl genrsa -out "$CERTS_DIR/ca.key" 2048
openssl req -x509 -new -nodes -key "$CERTS_DIR/ca.key" -sha256 -days $DAYS -out "$CERTS_DIR/ca.crt" -subj "$CA_SUBJECT"

echo "=== Generating Server Certificate ==="
# Create server key
openssl genrsa -out "$CERTS_DIR/server.key" 2048

# Create server CSR
openssl req -new -key "$CERTS_DIR/server.key" -out "$CERTS_DIR/server.csr" -subj "$SERVER_SUBJECT"

# Create extensions file for SAN (Subject Alternative Name)
cat > "$CERTS_DIR/server.ext" << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = mosquitto
DNS.3 = mosquitto-test
IP.1 = 127.0.0.1
IP.2 = $LOCAL_IP
EOF

# Sign server certificate with CA
openssl x509 -req -in "$CERTS_DIR/server.csr" -CA "$CERTS_DIR/ca.crt" -CAkey "$CERTS_DIR/ca.key" \
    -CAcreateserial -out "$CERTS_DIR/server.crt" -days $DAYS -sha256 -extfile "$CERTS_DIR/server.ext"

# Clean up CSR and ext file
rm -f "$CERTS_DIR/server.csr" "$CERTS_DIR/server.ext"

echo "=== Certificates Generated ==="
echo "CA Certificate:     $CERTS_DIR/ca.crt"
echo "Server Certificate: $CERTS_DIR/server.crt"
echo "Server Key:         $CERTS_DIR/server.key"
echo ""
echo "Use ca.crt as ROOT_CA_CERTIFICATE in mqtt_client_config.h"
echo ""

# Display CA certificate in PEM format for copy-paste
echo "=== CA Certificate (copy this to ROOT_CA_CERTIFICATE) ==="
cat "$CERTS_DIR/ca.crt"
