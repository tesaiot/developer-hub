#!/bin/bash
# TESAIoT Protected Update Workflow Runner
# Portable deployment version with bundled libraries

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"
SHIM_PATH="$DEPLOY_DIR/lib/libpaho_shim.so"
OPENSSL_CONF="/tmp/tesaiot_trustm_openssl.cnf"

# Set trustm tools path
export TRUSTM_LIB_PATH="$DEPLOY_DIR/bin/trustm"
export TRUSTM_DATA_PATH="$DEPLOY_DIR/bin/trustm/trustm_data"

# Check for bundled libraries (libtrustm.so)
BUNDLED_LIBS="$DEPLOY_DIR/lib"
USE_BUNDLED_LIBS=0
if [ -f "$BUNDLED_LIBS/libtrustm.so" ]; then
    USE_BUNDLED_LIBS=1
    echo "[PU-RUNNER] Found bundled libtrustm.so"
fi

# Find trustm_provider.so (check bundled first, then system)
TRUSTM_PROVIDER=""
if [ -f "$BUNDLED_LIBS/trustm_provider.so" ]; then
    TRUSTM_PROVIDER="$BUNDLED_LIBS/trustm_provider.so"
    echo "[PU-RUNNER] Using bundled trustm_provider.so"
elif [ -f "/usr/lib/aarch64-linux-gnu/ossl-modules/trustm_provider.so" ]; then
    TRUSTM_PROVIDER="/usr/lib/aarch64-linux-gnu/ossl-modules/trustm_provider.so"
elif [ -f "/usr/lib/arm-linux-gnueabihf/ossl-modules/trustm_provider.so" ]; then
    TRUSTM_PROVIDER="/usr/lib/arm-linux-gnueabihf/ossl-modules/trustm_provider.so"
else
    TRUSTM_PROVIDER=$(find /usr/lib /usr/local/lib /opt -name "trustm_provider.so" 2>/dev/null | head -1)
fi

if [ -z "$TRUSTM_PROVIDER" ]; then
    TRUSTM_PROVIDER="trustm_provider.so"
    echo "[PU-RUNNER] WARNING: Could not find trustm_provider.so"
fi

echo "[PU-RUNNER] Using trustm tools from: $TRUSTM_LIB_PATH"
echo "[PU-RUNNER] Using trustm_provider: $TRUSTM_PROVIDER"

# Create OpenSSL config for trustm_provider (use sudo to remove if owned by root)
sudo rm -f "$OPENSSL_CONF" 2>/dev/null || rm -f "$OPENSSL_CONF" 2>/dev/null
cat > "$OPENSSL_CONF" << EOF
# TESAIoT OpenSSL Configuration with OPTIGA Trust M Provider
openssl_conf = openssl_init

[openssl_init]
providers = provider_sect

[provider_sect]
trustm_provider = trustm_sect
default = default_sect

[trustm_sect]
module = $TRUSTM_PROVIDER
activate = 1

[default_sect]
activate = 1
EOF

# Parse command line arguments
# Default: USE_SHIM=1 (shim is required for OPTIGA Trust M + Paho MQTT)
USE_SHIM=1
ARGS=""
for arg in "$@"; do
    if [ "$arg" = "--no-shim" ]; then
        USE_SHIM=0
    else
        ARGS="$ARGS $arg"
    fi
done

# Build environment variables
ENV_VARS="TRUSTM_OPENSSL_CONF=$OPENSSL_CONF TRUSTM_DATA_PATH=$TRUSTM_DATA_PATH TRUSTM_LIB_PATH=$TRUSTM_LIB_PATH"

# Add bundled library path if available
if [ $USE_BUNDLED_LIBS -eq 1 ]; then
    ENV_VARS="LD_LIBRARY_PATH=$BUNDLED_LIBS:\$LD_LIBRARY_PATH $ENV_VARS"
    echo "[PU-RUNNER] Using bundled libraries from: $BUNDLED_LIBS"
fi

if [ $USE_SHIM -eq 1 ]; then
    echo "[PU-RUNNER] Using OPTIGA shim for Paho MQTT TLS (default)"
    echo "[PU-RUNNER] Shim path: $SHIM_PATH"
    echo "========================================"
    if [ $USE_BUNDLED_LIBS -eq 1 ]; then
        sudo -E LD_LIBRARY_PATH="$BUNDLED_LIBS:$LD_LIBRARY_PATH" LD_PRELOAD="$SHIM_PATH" TRUSTM_OPENSSL_CONF="$OPENSSL_CONF" TRUSTM_DATA_PATH="$TRUSTM_DATA_PATH" TRUSTM_LIB_PATH="$TRUSTM_LIB_PATH" "$DEPLOY_DIR/bin/tesaiot_pu_client" $ARGS
    else
        sudo -E LD_PRELOAD="$SHIM_PATH" TRUSTM_OPENSSL_CONF="$OPENSSL_CONF" TRUSTM_DATA_PATH="$TRUSTM_DATA_PATH" TRUSTM_LIB_PATH="$TRUSTM_LIB_PATH" "$DEPLOY_DIR/bin/tesaiot_pu_client" $ARGS
    fi
else
    echo "[PU-RUNNER] Running without shim"
    echo "========================================"
    if [ $USE_BUNDLED_LIBS -eq 1 ]; then
        sudo LD_LIBRARY_PATH="$BUNDLED_LIBS:$LD_LIBRARY_PATH" TRUSTM_DATA_PATH="$TRUSTM_DATA_PATH" TRUSTM_LIB_PATH="$TRUSTM_LIB_PATH" "$DEPLOY_DIR/bin/tesaiot_pu_client" $ARGS
    else
        sudo TRUSTM_DATA_PATH="$TRUSTM_DATA_PATH" TRUSTM_LIB_PATH="$TRUSTM_LIB_PATH" "$DEPLOY_DIR/bin/tesaiot_pu_client" $ARGS
    fi
fi
