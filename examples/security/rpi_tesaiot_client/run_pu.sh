#!/bin/bash
# TESAIoT Protected Update Workflow Runner
# This script sets up the environment and runs the PU client with OPTIGA support

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHIM_PATH="$SCRIPT_DIR/lib/libpaho_shim.so"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Source environment
if [ -f "$SCRIPT_DIR/.env" ]; then
    source "$SCRIPT_DIR/.env"
fi

#============================================================================
# Troubleshooting: Clean up root-owned temp files that can cause permission errors
#============================================================================
cleanup_temp_files() {
    local TEMP_FILES=(
        "/tmp/tesaiot_ca.pem"
        "/tmp/tesaiot_device_cert.pem"
        "/tmp/tesaiot_ta_cert.der"
    )

    local needs_cleanup=false
    for f in "${TEMP_FILES[@]}"; do
        if [ -f "$f" ] && [ "$(stat -c '%U' "$f" 2>/dev/null)" = "root" ]; then
            needs_cleanup=true
            break
        fi
    done

    if [ "$needs_cleanup" = true ]; then
        echo -e "${YELLOW}[CLEANUP] Found root-owned temp files that may cause permission errors${NC}"
        for f in "${TEMP_FILES[@]}"; do
            if [ -f "$f" ]; then
                echo "  - $f (owner: $(stat -c '%U' "$f" 2>/dev/null))"
            fi
        done

        # Clean up automatically
        echo -e "${YELLOW}[CLEANUP] Removing root-owned temp files...${NC}"
        for f in "${TEMP_FILES[@]}"; do
            if [ -f "$f" ]; then
                sudo rm -f "$f" 2>/dev/null && echo -e "${GREEN}  ✓ Removed $f${NC}"
            fi
        done
    fi
}

#============================================================================
# Troubleshooting: Check I2C permissions
#============================================================================
check_i2c_permissions() {
    if [ ! -r /dev/i2c-1 ]; then
        echo -e "${RED}[ERROR] Cannot access /dev/i2c-1${NC}"
        echo "  Try: sudo chmod 666 /dev/i2c-1"
        echo "  Or add user to i2c group: sudo usermod -aG i2c $USER"
        return 1
    fi
    return 0
}

#============================================================================
# Run troubleshooting checks
#============================================================================
echo "[PU] Running pre-flight checks..."
cleanup_temp_files
check_i2c_permissions || exit 1
echo -e "${GREEN}[PU] Pre-flight checks passed${NC}"
echo ""

# Check for shim library
if [ ! -f "$SHIM_PATH" ]; then
    echo -e "${RED}[ERROR] libpaho_shim.so not found at: $SHIM_PATH${NC}"
    echo "[ERROR] This is required for OPTIGA Trust M + Paho MQTT TLS"
    exit 1
fi

# Set library path for bundled libraries
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# Set trustm tools path
export TRUSTM_LIB_PATH="$SCRIPT_DIR/bin/trustm"
export TRUSTM_DATA_PATH="$SCRIPT_DIR/bin/trustm/trustm_data"

echo "[PU] Using OPTIGA shim: $SHIM_PATH"
echo "[PU] ========================================"

# Run PU client with LD_PRELOAD for OPTIGA hardware key support
export LD_PRELOAD="$SHIM_PATH"
exec "$SCRIPT_DIR/bin/tesaiot_pu_client" "$@"
