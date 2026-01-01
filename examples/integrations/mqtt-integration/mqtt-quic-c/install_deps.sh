#!/bin/bash
# ============================================================
# TESAIoT MQTT over QUIC C Client - Dependencies Install Script
# ============================================================
#
# Installs NanoSDK + MsQuic for MQTT over QUIC connectivity
# Supports: Linux (Ubuntu/Debian), Raspberry Pi
#
# NOTE: macOS is NOT supported due to compiler compatibility issues
#       Use mqtt-quic-python (aioquic) for macOS instead
#
# Usage:
#   chmod +x install_deps.sh
#   ./install_deps.sh
#
# Author: TESAIoT Team
# License: Apache 2.0
# ============================================================

set -e

echo "============================================================"
echo "  TESAIoT MQTT over QUIC C - Dependencies Installer"
echo "============================================================"
echo ""

# Configuration
NANOSDK_VERSION="main"  # or specific tag
INSTALL_PREFIX="/usr/local"
BUILD_DIR="/tmp/mqtt-quic-c-build"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                if [[ "$ID" == "raspbian" ]] || [[ "$ID_LIKE" == *"debian"* && "$(uname -m)" == "arm"* ]]; then
                    echo "rpi"
                elif [[ "$ID" == "ubuntu" ]] || [[ "$ID" == "debian" ]]; then
                    echo "linux-debian"
                elif [[ "$ID" == "fedora" ]] || [[ "$ID" == "centos" ]] || [[ "$ID" == "rhel" ]]; then
                    echo "linux-rhel"
                else
                    echo "linux"
                fi
            else
                echo "linux"
            fi
            ;;
        Darwin*)
            echo "macos"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

OS=$(detect_os)
echo "[INFO] Detected OS: $OS"
echo "[INFO] Architecture: $(uname -m)"
echo ""

# Check if macOS - not supported
if [ "$OS" == "macos" ]; then
    echo "============================================================"
    echo "  ERROR: macOS is NOT supported for mqtt-quic-c"
    echo "============================================================"
    echo ""
    echo "Reason: NanoSDK + MsQuic have compiler compatibility issues"
    echo "        with macOS Apple Silicon / Clang 17"
    echo ""
    echo "Alternative: Use mqtt-quic-python with aioquic library instead"
    echo ""
    echo "Steps:"
    echo "  cd ../../../embedded-devices/entry/mqtt-quic-python/"
    echo "  ./install_deps.sh"
    echo "  python3 mqtt_quic_aioquic.py <device_id> <password>"
    echo ""
    exit 1
fi

# Install system dependencies
install_system_deps() {
    echo "[STEP 1] Installing system dependencies..."

    case $OS in
        linux-debian|rpi)
            echo "[INFO] Installing build tools for Debian/Ubuntu/RPi..."
            sudo apt-get update
            sudo apt-get install -y \
                cmake \
                gcc \
                g++ \
                make \
                git \
                libssl-dev \
                pkg-config \
                ninja-build
            ;;

        linux-rhel)
            echo "[INFO] Installing build tools for RHEL/Fedora..."
            sudo dnf install -y \
                cmake \
                gcc \
                gcc-c++ \
                make \
                git \
                openssl-devel \
                pkgconfig \
                ninja-build
            ;;

        *)
            echo "[ERROR] Unsupported OS: $OS"
            exit 1
            ;;
    esac

    echo "[STEP 1] Done."
    echo ""
}

# Build and install MsQuic
build_msquic() {
    echo "[STEP 2] Building MsQuic..."

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    if [ ! -d "NanoSDK" ]; then
        echo "[INFO] Cloning NanoSDK repository..."
        git clone --depth 1 https://github.com/nanomq/NanoSDK.git
        cd NanoSDK
        echo "[INFO] Initializing submodules (this may take a while)..."
        git submodule update --init --recursive
    else
        echo "[INFO] NanoSDK already cloned, updating..."
        cd NanoSDK
        git pull
        git submodule update --recursive
    fi

    # Build MsQuic
    echo "[INFO] Configuring MsQuic..."
    cd extern/msquic
    rm -rf build
    mkdir build && cd build

    cmake -G 'Unix Makefiles' \
        -DCMAKE_BUILD_TYPE=Release \
        -DQUIC_BUILD_SHARED=OFF \
        ..

    echo "[INFO] Building MsQuic (this may take 5-10 minutes)..."
    make -j$(nproc)

    echo "[INFO] Installing MsQuic..."
    sudo make install

    echo "[STEP 2] Done."
    echo ""
}

# Build and install NanoSDK with QUIC
build_nanosdk() {
    echo "[STEP 3] Building NanoSDK with QUIC support..."

    cd "$BUILD_DIR/NanoSDK"
    rm -rf build
    mkdir build && cd build

    echo "[INFO] Configuring NanoSDK with QUIC..."
    cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DNNG_ENABLE_QUIC=ON \
        -DCMAKE_BUILD_TYPE=Release \
        ..

    echo "[INFO] Building NanoSDK (this may take 3-5 minutes)..."
    make -j$(nproc)

    echo "[INFO] Installing NanoSDK..."
    sudo make install

    # Update library cache
    sudo ldconfig

    echo "[STEP 3] Done."
    echo ""
}

# Verify installation
verify_install() {
    echo "[STEP 4] Verifying installation..."

    # Check NNG headers
    if [ -f "$INSTALL_PREFIX/include/nng/nng.h" ]; then
        echo "[OK] NNG headers installed"
    else
        echo "[ERROR] NNG headers not found"
        exit 1
    fi

    # Check NNG library
    if ldconfig -p | grep -q libnng; then
        echo "[OK] NNG library found"
    else
        echo "[WARN] NNG library not in ldconfig - checking manually..."
        if [ -f "$INSTALL_PREFIX/lib/libnng.so" ]; then
            echo "[OK] NNG library exists at $INSTALL_PREFIX/lib/libnng.so"
        else
            echo "[ERROR] NNG library not found"
            exit 1
        fi
    fi

    echo "[STEP 4] Done."
    echo ""
}

# Build the example client
build_example() {
    echo "[STEP 5] Building MQTT QUIC client example..."

    cd "$SCRIPT_DIR"
    rm -rf build
    mkdir build && cd build

    cmake ..
    make

    if [ -f "mqtt_quic_client" ]; then
        echo "[OK] mqtt_quic_client built successfully"
        ls -la mqtt_quic_client
    else
        echo "[ERROR] Build failed - mqtt_quic_client not found"
        exit 1
    fi

    echo "[STEP 5] Done."
    echo ""
}

# Print usage instructions
print_usage() {
    echo "============================================================"
    echo "  Installation Complete!"
    echo "============================================================"
    echo ""
    echo "The MQTT QUIC client has been built at:"
    echo "  $SCRIPT_DIR/build/mqtt_quic_client"
    echo ""
    echo "To run:"
    echo "  cd $SCRIPT_DIR/build"
    echo "  ./mqtt_quic_client <device_id> <password>"
    echo ""
    echo "Example:"
    echo "  ./mqtt_quic_client 95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9 MyPassword123"
    echo ""
    echo "Make sure to copy ca-chain.pem to the same directory as the client."
    echo ""
}

# Parse arguments
SKIP_DEPS=false
SKIP_BUILD=false
REBUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --rebuild)
            REBUILD=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --skip-deps   Skip system dependency installation"
            echo "  --skip-build  Skip building the example client"
            echo "  --rebuild     Force rebuild of NanoSDK/MsQuic"
            echo "  -h, --help    Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Main
main() {
    if [ "$SKIP_DEPS" = false ]; then
        install_system_deps
    fi

    if [ "$REBUILD" = true ] || [ ! -d "$BUILD_DIR/NanoSDK/extern/msquic/build" ]; then
        build_msquic
    else
        echo "[INFO] MsQuic already built (use --rebuild to force)"
    fi

    if [ "$REBUILD" = true ] || [ ! -d "$BUILD_DIR/NanoSDK/build" ]; then
        build_nanosdk
    else
        echo "[INFO] NanoSDK already built (use --rebuild to force)"
    fi

    verify_install

    if [ "$SKIP_BUILD" = false ]; then
        build_example
    fi

    print_usage
}

main "$@"
