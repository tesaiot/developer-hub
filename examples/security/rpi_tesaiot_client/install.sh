#!/bin/bash
# TESAIoT Portable Deployment - Installation Script
# Installs dependencies and TESAIoT to /opt/tesaiot

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="/opt/tesaiot"

echo "============================================"
echo "TESAIoT Installation Script"
echo "============================================"
echo "Source: $SCRIPT_DIR"
echo "Target: $INSTALL_DIR"
echo ""

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
    echo "This script requires root privileges."
    echo "Please run: sudo ./install.sh"
    exit 1
fi

# Install system dependencies
echo "[1/7] Installing system dependencies..."
apt-get update -qq

# Required libraries for TESAIoT binaries
apt-get install -y \
    libssl3 \
    libpaho-mqtt3c1 \
    libgpiod2 \
    i2c-tools

echo "  Dependencies installed."

# Install bundled OPTIGA Trust M libraries (if available)
echo ""
echo "[2/7] Installing OPTIGA Trust M libraries..."
INSTALL_BUNDLED_TRUSTM=0

if [ -f "$SCRIPT_DIR/lib/libtrustm.so" ]; then
    # Determine architecture-specific library path
    ARCH=$(uname -m)
    if [ "$ARCH" = "aarch64" ]; then
        LIB_PATH="/usr/lib/aarch64-linux-gnu"
        OSSL_PATH="/usr/lib/aarch64-linux-gnu/ossl-modules"
    elif [ "$ARCH" = "armv7l" ] || [ "$ARCH" = "armhf" ]; then
        LIB_PATH="/usr/lib/arm-linux-gnueabihf"
        OSSL_PATH="/usr/lib/arm-linux-gnueabihf/ossl-modules"
    else
        LIB_PATH="/usr/local/lib"
        OSSL_PATH="/usr/local/lib/ossl-modules"
    fi

    # Install libtrustm.so
    mkdir -p "$LIB_PATH"
    cp "$SCRIPT_DIR/lib/libtrustm.so" "$LIB_PATH/"
    chmod 755 "$LIB_PATH/libtrustm.so"
    echo "  Installed: $LIB_PATH/libtrustm.so"
    INSTALL_BUNDLED_TRUSTM=1

    # Install trustm_provider.so
    if [ -f "$SCRIPT_DIR/lib/trustm_provider.so" ]; then
        mkdir -p "$OSSL_PATH"
        cp "$SCRIPT_DIR/lib/trustm_provider.so" "$OSSL_PATH/"
        chmod 755 "$OSSL_PATH/trustm_provider.so"
        echo "  Installed: $OSSL_PATH/trustm_provider.so"
    fi

    # Update linker cache
    ldconfig
    echo "  Library cache updated."
else
    echo "  No bundled libraries found. Checking system..."
    if ldconfig -p | grep -q "libtrustm"; then
        echo "  System libtrustm.so found."
    else
        echo "  WARNING: libtrustm.so not found!"
        echo "  You may need to build from source. See README.md for instructions."
    fi
fi

# Verify OPTIGA Trust M installation
echo ""
echo "[3/7] Verifying OPTIGA Trust M installation..."
if ldconfig -p | grep -q "libtrustm"; then
    echo "  libtrustm.so: OK"
else
    echo "  libtrustm.so: NOT FOUND (workflows may fail)"
fi

TRUSTM_PROVIDER=$(find /usr/lib -name "trustm_provider.so" 2>/dev/null | head -1)
if [ -n "$TRUSTM_PROVIDER" ]; then
    echo "  trustm_provider.so: $TRUSTM_PROVIDER"
else
    echo "  trustm_provider.so: NOT FOUND (will use bundled from lib/)"
fi

# Create installation directories
echo ""
echo "[4/7] Creating directories..."
mkdir -p "$INSTALL_DIR"/{bin/trustm,lib,include/tesaiot,config,scripts,credentials}

# Copy binaries
echo "[5/7] Installing binaries..."
cp "$SCRIPT_DIR/bin/tesaiot_csr_client" "$INSTALL_DIR/bin/"
cp "$SCRIPT_DIR/bin/tesaiot_pu_client" "$INSTALL_DIR/bin/"
chmod +x "$INSTALL_DIR/bin/"*

# Copy trustm tools
echo "[6/7] Installing trustm tools..."
if [ -d "$SCRIPT_DIR/bin/trustm" ] && [ "$(ls -A $SCRIPT_DIR/bin/trustm)" ]; then
    cp "$SCRIPT_DIR/bin/trustm/"* "$INSTALL_DIR/bin/trustm/"
    chmod +x "$INSTALL_DIR/bin/trustm/"*
else
    echo "  WARNING: No trustm tools found in package."
    echo "  Please ensure linux-optiga-trust-m is installed separately."
fi

# Copy library and shim
echo "[7/7] Installing libraries and finalizing..."
cp "$SCRIPT_DIR/lib/libtesaiot.a" "$INSTALL_DIR/lib/"
cp "$SCRIPT_DIR/lib/libpaho_shim.so" "$INSTALL_DIR/lib/"

# Copy bundled OPTIGA Trust M libraries for fallback
if [ -f "$SCRIPT_DIR/lib/libtrustm.so" ]; then
    cp "$SCRIPT_DIR/lib/libtrustm.so" "$INSTALL_DIR/lib/"
fi
if [ -f "$SCRIPT_DIR/lib/trustm_provider.so" ]; then
    cp "$SCRIPT_DIR/lib/trustm_provider.so" "$INSTALL_DIR/lib/"
fi

# Copy headers
cp "$SCRIPT_DIR/include/tesaiot/"*.h "$INSTALL_DIR/include/tesaiot/"

# Copy credentials (required for ../credentials path relative to bin/)
if [ -d "$SCRIPT_DIR/credentials" ] && [ "$(ls -A $SCRIPT_DIR/credentials)" ]; then
    cp "$SCRIPT_DIR/credentials/"* "$INSTALL_DIR/credentials/"
    echo "  Credentials copied to $INSTALL_DIR/credentials/"
else
    echo "  WARNING: No credentials found in package."
    echo "  You must copy CA certificates to $INSTALL_DIR/credentials/"
fi

# Copy scripts and config
cp "$SCRIPT_DIR/scripts/"*.sh "$INSTALL_DIR/scripts/"
chmod +x "$INSTALL_DIR/scripts/"*.sh
cp "$SCRIPT_DIR/config/"* "$INSTALL_DIR/config/"

# Create profile script for PATH
cat > /etc/profile.d/tesaiot.sh << 'EOF'
# TESAIoT environment configuration
export PATH="$PATH:/opt/tesaiot/bin:/opt/tesaiot/bin/trustm"
export TRUSTM_LIB_PATH="/opt/tesaiot/bin/trustm"
export TRUSTM_DATA_PATH="/opt/tesaiot/bin/trustm/trustm_data"
EOF

# Create symlinks for convenience
ln -sf "$INSTALL_DIR/scripts/run_csr_workflow.sh" "$INSTALL_DIR/bin/run_csr"
ln -sf "$INSTALL_DIR/scripts/run_pu_workflow.sh" "$INSTALL_DIR/bin/run_pu"

echo ""
echo "============================================"
echo "Installation Complete!"
echo "============================================"
echo ""
echo "Installed to: $INSTALL_DIR"
echo ""
echo "Directory structure:"
echo "  $INSTALL_DIR/"
echo "  ├── bin/"
echo "  │   ├── tesaiot_csr_client"
echo "  │   ├── tesaiot_pu_client"
echo "  │   ├── run_csr -> scripts/run_csr_workflow.sh"
echo "  │   ├── run_pu -> scripts/run_pu_workflow.sh"
echo "  │   └── trustm/"
echo "  ├── credentials/        (CA certificates)"
echo "  ├── lib/"
echo "  │   ├── libtesaiot.a"
echo "  │   ├── libpaho_shim.so"
echo "  │   ├── libtrustm.so      (bundled, libgpiod v2.x compatible)"
echo "  │   └── trustm_provider.so"
echo "  ├── include/tesaiot/"
echo "  ├── config/"
echo "  └── scripts/"
echo ""
echo "Usage:"
echo "  # Reboot or source environment"
echo "  source /etc/profile.d/tesaiot.sh"
echo ""
echo "  # Run CSR workflow"
echo "  run_csr --use-shim"
echo ""
echo "  # Run Protected Update workflow"
echo "  run_pu --use-shim"
echo ""
echo "============================================"
