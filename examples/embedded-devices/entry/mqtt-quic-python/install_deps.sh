#!/bin/bash
# ============================================================
# TESAIoT MQTT over QUIC - Python Dependencies Install Script
# ============================================================
#
# Installs aioquic library for MQTT over QUIC connectivity
# Supports: macOS, Linux, Windows (WSL), Raspberry Pi
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
echo "  TESAIoT MQTT over QUIC - Python Dependencies Installer"
echo "============================================================"
echo ""

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
        MINGW*|MSYS*|CYGWIN*)
            echo "windows"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

OS=$(detect_os)
echo "[INFO] Detected OS: $OS"
echo ""

# Install system dependencies based on OS
install_system_deps() {
    echo "[STEP 1] Installing system dependencies..."

    case $OS in
        macos)
            echo "[INFO] macOS detected - checking Homebrew..."
            if ! command -v brew &> /dev/null; then
                echo "[ERROR] Homebrew not found. Please install from https://brew.sh"
                exit 1
            fi
            echo "[INFO] Installing OpenSSL (required for aioquic)..."
            brew install openssl@3 2>/dev/null || true
            export LDFLAGS="-L$(brew --prefix openssl@3)/lib"
            export CPPFLAGS="-I$(brew --prefix openssl@3)/include"
            ;;

        linux-debian|rpi)
            echo "[INFO] Debian/Ubuntu/RPi detected - installing build tools..."
            sudo apt-get update
            sudo apt-get install -y python3-pip python3-venv libssl-dev build-essential
            ;;

        linux-rhel)
            echo "[INFO] RHEL/Fedora/CentOS detected - installing build tools..."
            sudo dnf install -y python3-pip python3-devel openssl-devel gcc
            ;;

        windows)
            echo "[INFO] Windows detected - please ensure Python 3.8+ is installed"
            echo "[INFO] OpenSSL should be bundled with Python on Windows"
            ;;

        *)
            echo "[WARN] Unknown OS - skipping system dependencies"
            ;;
    esac

    echo "[STEP 1] Done."
    echo ""
}

# Check Python version
check_python() {
    echo "[STEP 2] Checking Python version..."

    PYTHON_CMD=""
    if command -v python3 &> /dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &> /dev/null; then
        PYTHON_CMD="python"
    else
        echo "[ERROR] Python not found. Please install Python 3.8+"
        exit 1
    fi

    PYTHON_VERSION=$($PYTHON_CMD -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    echo "[INFO] Python version: $PYTHON_VERSION"

    MAJOR=$(echo $PYTHON_VERSION | cut -d. -f1)
    MINOR=$(echo $PYTHON_VERSION | cut -d. -f2)

    if [ "$MAJOR" -lt 3 ] || ([ "$MAJOR" -eq 3 ] && [ "$MINOR" -lt 8 ]); then
        echo "[ERROR] Python 3.8+ required. Found: $PYTHON_VERSION"
        exit 1
    fi

    echo "[STEP 2] Done."
    echo ""
}

# Install Python dependencies
install_python_deps() {
    echo "[STEP 3] Installing Python dependencies..."

    # Upgrade pip first
    $PYTHON_CMD -m pip install --upgrade pip

    # Install aioquic (main QUIC library)
    echo "[INFO] Installing aioquic (QUIC/HTTP3 library)..."
    $PYTHON_CMD -m pip install aioquic

    # Install additional dependencies
    echo "[INFO] Installing additional dependencies..."
    $PYTHON_CMD -m pip install python-dotenv

    echo "[STEP 3] Done."
    echo ""
}

# Verify installation
verify_install() {
    echo "[STEP 4] Verifying installation..."

    $PYTHON_CMD -c "import aioquic; print(f'[OK] aioquic version: {aioquic.__version__}')" || {
        echo "[ERROR] aioquic installation failed"
        exit 1
    }

    $PYTHON_CMD -c "from aioquic.quic.configuration import QuicConfiguration; print('[OK] QUIC Configuration available')" || {
        echo "[ERROR] QUIC Configuration not available"
        exit 1
    }

    echo "[STEP 4] Done."
    echo ""
}

# Create .env from example if not exists
setup_env() {
    echo "[STEP 5] Setting up environment..."

    if [ ! -f .env ] && [ -f .env.example ]; then
        cp .env.example .env
        echo "[INFO] Created .env from .env.example"
        echo "[WARN] Please edit .env with your device credentials"
    elif [ -f .env ]; then
        echo "[INFO] .env file already exists"
    else
        echo "[WARN] No .env.example found - please create .env manually"
    fi

    echo "[STEP 5] Done."
    echo ""
}

# Main
main() {
    install_system_deps
    check_python
    install_python_deps
    verify_install
    setup_env

    echo "============================================================"
    echo "  Installation Complete!"
    echo "============================================================"
    echo ""
    echo "Next steps:"
    echo "  1. Edit .env with your device credentials"
    echo "  2. Copy ca-chain.pem from your credential bundle"
    echo "  3. Run: python3 mqtt_quic_aioquic.py <device_id> <password>"
    echo ""
    echo "For testing without arguments (uses .env):"
    echo "  python3 mqtt_quic_aioquic.py"
    echo ""
}

main "$@"
