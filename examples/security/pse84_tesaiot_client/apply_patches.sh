#!/bin/bash
#
# apply_patches.sh - Apply patches for library integration
#
# Usage: ./apply_patches.sh
#
# Target Versions:
#   - ifx-mbedtls: release-v3.6.400
#   - secure-sockets: release-v3.12.1
#
# PATCHES APPLIED:
#   1. ifx-mbedtls.patch      - PSA headers + mbedtls_ms_time() for FreeRTOS
#   2. secure-sockets.patch   - OPTIGA Trust M TLS integration
#   3. lwipopts.h             - LWIP_TCPIP_CORE_LOCKING_INPUT=0
#
# Compatible with: macOS, Linux, Windows (Git Bash/MSYS2/WSL)
#
# Date: 2026-01-17
# Version: 3.1
#

set -e  # Exit on error

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------

# Get script directory (works on macOS, Linux, Windows Git Bash)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# mtb_shared is one level up from project directory
MTB_SHARED="$(cd "$SCRIPT_DIR/.." && pwd)/mtb_shared"

# Patch files (in project root)
PATCH_MBEDTLS="$SCRIPT_DIR/ifx-mbedtls.patch"
PATCH_SECURE_SOCKETS="$SCRIPT_DIR/secure-sockets.patch"

# Target directories for patches
TARGET_MBEDTLS="$MTB_SHARED/ifx-mbedtls/release-v3.6.400"
TARGET_SECURE_SOCKETS="$MTB_SHARED/secure-sockets/release-v3.12.1"

# File for manual patch
LWIP_OPTS_FILE="$MTB_SHARED/wifi-core-freertos-lwip-mbedtls/release-v3.1.0/configs/lwipopts.h"

#------------------------------------------------------------------------------
# Colors (portable)
#------------------------------------------------------------------------------
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    CYAN='\033[0;36m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    CYAN=''
    NC=''
fi

#------------------------------------------------------------------------------
# Functions
#------------------------------------------------------------------------------

print_header() {
    echo ""
    echo "============================================================"
    echo "  PSE84 Library Patcher v3.0 (BETA)"
    echo "  ifx-mbedtls: v3.6.400 | secure-sockets: v3.12.1"
    echo "============================================================"
    echo ""
}

print_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

check_mtb_shared() {
    if [ ! -d "$MTB_SHARED" ]; then
        print_error "mtb_shared directory not found at: $MTB_SHARED"
        echo ""
        echo "Please run 'make getlibs' first to download the libraries."
        exit 1
    fi
    print_success "Found mtb_shared at: $MTB_SHARED"
}

check_patch_command() {
    if ! command -v patch &> /dev/null; then
        print_error "'patch' command not found!"
        echo ""
        echo "Please install patch:"
        echo "  macOS:   brew install gpatch"
        echo "  Ubuntu:  sudo apt install patch"
        echo "  Windows: Install Git Bash (includes patch)"
        exit 1
    fi
    print_success "Found 'patch' command"
}

#------------------------------------------------------------------------------
# Patch 1: ifx-mbedtls.patch
#------------------------------------------------------------------------------
#
# Creates:
#   - include/psa/error.h (PSA error types)
#   - include/psa/internal_trusted_storage.h (ITS stubs)
#
# Modifies:
#   - library/pk_internal.h (MBEDTLS_PRIVATE access fix)
#   - library/platform_util.c (mbedtls_ms_time() for FreeRTOS)
#
patch_mbedtls() {
    echo ""
    echo "--- Patch 1: ifx-mbedtls (PSA headers + FreeRTOS time) ---"

    # Check patch file exists
    if [ ! -f "$PATCH_MBEDTLS" ]; then
        print_error "ifx-mbedtls.patch not found at: $PATCH_MBEDTLS"
        return 1
    fi

    # Check target directory exists
    if [ ! -d "$TARGET_MBEDTLS" ]; then
        # Try to find alternative version
        local ALT_DIR=$(find "$MTB_SHARED/ifx-mbedtls" -maxdepth 1 -type d -name "release-*" 2>/dev/null | head -1)
        if [ -n "$ALT_DIR" ]; then
            print_warning "Expected version not found, using: $ALT_DIR"
            TARGET_MBEDTLS="$ALT_DIR"
        else
            print_error "ifx-mbedtls directory not found at: $TARGET_MBEDTLS"
            return 1
        fi
    fi

    # Check if already patched (look for created file)
    if [ -f "$TARGET_MBEDTLS/include/psa/error.h" ]; then
        print_success "Already patched (include/psa/error.h exists)"
        return 0
    fi

    # Apply patch
    print_info "Applying patch to: $TARGET_MBEDTLS"
    cd "$TARGET_MBEDTLS"

    if patch -p1 --dry-run < "$PATCH_MBEDTLS" > /dev/null 2>&1; then
        patch -p1 < "$PATCH_MBEDTLS"
        print_success "ifx-mbedtls.patch applied successfully"
    else
        # Try with --forward to skip already applied hunks
        if patch -p1 --forward < "$PATCH_MBEDTLS" 2>&1 | grep -q "Reversed"; then
            print_success "Already patched (patch detected reversed)"
        else
            print_error "Patch failed! Please apply manually:"
            echo "  cd $TARGET_MBEDTLS"
            echo "  patch -p1 < $PATCH_MBEDTLS"
            return 1
        fi
    fi

    cd "$SCRIPT_DIR"
    return 0
}

#------------------------------------------------------------------------------
# Patch 2: secure-sockets.patch
#------------------------------------------------------------------------------
#
# Creates:
#   - source/COMPONENT_MBEDTLS/cy_tls_optiga_key.c
#   - source/COMPONENT_MBEDTLS/cy_tls_optiga_key.h
#
# Modifies:
#   - source/COMPONENT_MBEDTLS/cy_tls.c (OPTIGA Trust M key binding)
#
# Provides APIs:
#   - cy_tls_set_optiga_key_id()
#   - cy_tls_get_optiga_key_id()
#
patch_secure_sockets() {
    echo ""
    echo "--- Patch 2: secure-sockets (OPTIGA Trust M TLS) ---"

    # Check patch file exists
    if [ ! -f "$PATCH_SECURE_SOCKETS" ]; then
        print_error "secure-sockets.patch not found at: $PATCH_SECURE_SOCKETS"
        return 1
    fi

    # Check target directory exists
    if [ ! -d "$TARGET_SECURE_SOCKETS" ]; then
        # Try to find alternative version
        local ALT_DIR=$(find "$MTB_SHARED/secure-sockets" -maxdepth 1 -type d -name "release-*" 2>/dev/null | head -1)
        if [ -n "$ALT_DIR" ]; then
            print_warning "Expected version not found, using: $ALT_DIR"
            TARGET_SECURE_SOCKETS="$ALT_DIR"
        else
            print_error "secure-sockets directory not found at: $TARGET_SECURE_SOCKETS"
            return 1
        fi
    fi

    # Check if already patched (look for created file)
    if [ -f "$TARGET_SECURE_SOCKETS/source/COMPONENT_MBEDTLS/cy_tls_optiga_key.c" ]; then
        print_success "Already patched (cy_tls_optiga_key.c exists)"
        return 0
    fi

    # Apply patch
    print_info "Applying patch to: $TARGET_SECURE_SOCKETS"
    cd "$TARGET_SECURE_SOCKETS"

    if patch -p1 --dry-run < "$PATCH_SECURE_SOCKETS" > /dev/null 2>&1; then
        patch -p1 < "$PATCH_SECURE_SOCKETS"
        print_success "secure-sockets.patch applied successfully"
    else
        # Try with --forward to skip already applied hunks
        if patch -p1 --forward < "$PATCH_SECURE_SOCKETS" 2>&1 | grep -q "Reversed"; then
            print_success "Already patched (patch detected reversed)"
        else
            print_error "Patch failed! Please apply manually:"
            echo "  cd $TARGET_SECURE_SOCKETS"
            echo "  patch -p1 < $PATCH_SECURE_SOCKETS"
            return 1
        fi
    fi

    cd "$SCRIPT_DIR"
    return 0
}

#------------------------------------------------------------------------------
# Patch 3: LWIP_TCPIP_CORE_LOCKING_INPUT
#------------------------------------------------------------------------------
#
# Why: With LWIP_TCPIP_CORE_LOCKING_INPUT=1 (default), incoming TCP ACKs
#      cannot be processed while netconn_write_partly() holds the mutex.
#      This causes ERR_WOULDBLOCK (-7) failures on cy_mqtt_publish().
#
# Fix: Set LWIP_TCPIP_CORE_LOCKING_INPUT=0
#
patch_lwipopts() {
    echo ""
    echo "--- Patch 3: lwIP Core Locking Fix ---"

    # Check if file exists
    if [ ! -f "$LWIP_OPTS_FILE" ]; then
        # Try to find alternative version
        local ALT_FILE=$(find "$MTB_SHARED/wifi-core-freertos-lwip-mbedtls" -name "lwipopts.h" -path "*/configs/*" 2>/dev/null | head -1)
        if [ -n "$ALT_FILE" ]; then
            print_warning "Expected file not found, using: $ALT_FILE"
            LWIP_OPTS_FILE="$ALT_FILE"
        else
            print_error "lwipopts.h not found at: $LWIP_OPTS_FILE"
            echo "Library version may have changed. Please check the path."
            return 1
        fi
    fi

    # Check current value
    local CURRENT=$(grep -E "^#define LWIP_TCPIP_CORE_LOCKING_INPUT" "$LWIP_OPTS_FILE" 2>/dev/null | awk '{print $3}')

    if [ "$CURRENT" = "0" ]; then
        print_success "Already patched (LWIP_TCPIP_CORE_LOCKING_INPUT=0)"
        return 0
    elif [ "$CURRENT" = "1" ]; then
        print_info "Current value: LWIP_TCPIP_CORE_LOCKING_INPUT=1 (needs patching)"

        # Create backup
        cp "$LWIP_OPTS_FILE" "$LWIP_OPTS_FILE.bak"
        print_success "Backup created: $LWIP_OPTS_FILE.bak"

        # Apply patch (portable sed - works on macOS, Linux, Git Bash)
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS sed requires '' after -i
            sed -i '' 's/LWIP_TCPIP_CORE_LOCKING_INPUT   1/LWIP_TCPIP_CORE_LOCKING_INPUT   0/' "$LWIP_OPTS_FILE"
        else
            # Linux/Windows Git Bash
            sed -i 's/LWIP_TCPIP_CORE_LOCKING_INPUT   1/LWIP_TCPIP_CORE_LOCKING_INPUT   0/' "$LWIP_OPTS_FILE"
        fi

        # Verify patch
        local NEW=$(grep -E "^#define LWIP_TCPIP_CORE_LOCKING_INPUT" "$LWIP_OPTS_FILE" | awk '{print $3}')
        if [ "$NEW" = "0" ]; then
            print_success "Patched: LWIP_TCPIP_CORE_LOCKING_INPUT=0"
        else
            print_error "Patch failed! Please edit manually."
            return 1
        fi
    else
        print_warning "Could not determine current value. Please check manually."
        echo "File: $LWIP_OPTS_FILE"
        echo "Expected: #define LWIP_TCPIP_CORE_LOCKING_INPUT   0"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Summary
#------------------------------------------------------------------------------

print_summary() {
    echo ""
    echo "============================================================"
    echo "  Patch Summary"
    echo "============================================================"
    echo ""
    echo "  Applied patches:"
    echo "    1. ifx-mbedtls.patch"
    echo "       - PSA error headers"
    echo "       - mbedtls_ms_time() for FreeRTOS"
    echo ""
    echo "    2. secure-sockets.patch"
    echo "       - cy_tls_optiga_key.c/h (OPTIGA Trust M TLS)"
    echo "       - Hardware key binding for TLS handshake"
    echo ""
    echo "    3. lwipopts.h"
    echo "       - LWIP_TCPIP_CORE_LOCKING_INPUT=0"
    echo "       - Fix ERR_WOULDBLOCK (-7) on MQTT publish"
    echo ""
    echo "  Project files (already in source, no patch needed):"
    echo "    - proj_cm33_ns/mbedtls_user_config.h"
    echo "      (MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)"
    echo ""
    echo "============================================================"
    echo ""
    echo -e "${GREEN}IMPORTANT:${NC} Run a clean build after patching:"
    echo ""
    echo "    make clean && make build -j8"
    echo ""
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

main() {
    print_header

    echo "Checking environment..."
    check_mtb_shared
    check_patch_command

    echo ""
    echo "Applying patches..."

    # Track patch failures
    local PATCH_FAILED=0

    # Apply all patches
    patch_mbedtls || PATCH_FAILED=1
    patch_secure_sockets || PATCH_FAILED=1
    patch_lwipopts || PATCH_FAILED=1

    # Summary
    if [ $PATCH_FAILED -eq 0 ]; then
        print_summary
        echo -e "${GREEN}All patches applied successfully!${NC}"
    else
        echo ""
        print_error "Some patches failed. Please check the errors above."
        exit 1
    fi
}

# Run main
main "$@"
