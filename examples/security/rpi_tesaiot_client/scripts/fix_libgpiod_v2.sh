#!/bin/bash
# fix_libgpiod_v2.sh - Automatic patch for libgpiod v2.x compatibility
#
# This script patches linux-optiga-trust-m to work with libgpiod v2.x API
# Required for: Raspberry Pi OS Bookworm (Debian 12+), Ubuntu 24.04+
#
# Author: TESAIoT Team
# Reference: TESAIoT_Mission/5-Fix_Issues_Linux_Trust_M.md

set -e

echo "============================================"
echo "libgpiod v2.x Compatibility Patch Script"
echo "============================================"
echo ""

# Check if we're in linux-optiga-trust-m directory
if [ ! -f "trustm_lib/extras/pal/linux/pal_gpio_gpiod.c" ]; then
    echo "ERROR: This script must be run from the linux-optiga-trust-m root directory."
    echo ""
    echo "Usage:"
    echo "  cd /path/to/linux-optiga-trust-m"
    echo "  /path/to/fix_libgpiod_v2.sh"
    exit 1
fi

# Check libgpiod version
echo "[1/4] Checking libgpiod version..."
LIBGPIOD_VERSION=$(apt-cache policy libgpiod-dev 2>/dev/null | grep "Installed:" | awk '{print $2}')
if [ -z "$LIBGPIOD_VERSION" ]; then
    echo "  WARNING: libgpiod-dev not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y libgpiod-dev
    LIBGPIOD_VERSION=$(apt-cache policy libgpiod-dev | grep "Installed:" | awk '{print $2}')
fi

echo "  Installed version: $LIBGPIOD_VERSION"

# Check if patch is needed
if [[ "$LIBGPIOD_VERSION" == 1.* ]]; then
    echo "  You have libgpiod v1.x - NO PATCH NEEDED!"
    echo "  You can build directly with: ./build.sh"
    exit 0
fi

echo "  You have libgpiod v2.x - PATCH REQUIRED"
echo ""

# Backup original file
echo "[2/4] Backing up original file..."
PAL_GPIO_FILE="trustm_lib/extras/pal/linux/pal_gpio_gpiod.c"
if [ ! -f "${PAL_GPIO_FILE}.original" ]; then
    cp "$PAL_GPIO_FILE" "${PAL_GPIO_FILE}.original"
    echo "  Backup created: ${PAL_GPIO_FILE}.original"
else
    echo "  Backup already exists, skipping"
fi

# Apply patch
echo "[3/4] Applying libgpiod v2.x patch..."

cat > "$PAL_GPIO_FILE" << 'PATCH_EOF'
/**
* \copyright
* MIT License
*
* Copyright (c) 2019 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
* \endcopyright
*
* \author Infineon Technologies AG
* \author TESAIoT Team (libgpiod v2.x patch)
*
* \file pal_gpio_gpiod.c
*
* \brief   This file implements the platform abstraction layer APIs for GPIO
*          using libgpiod v2.x API (compatible with Debian 12+, Ubuntu 24.04+)
*
* \ingroup  grPAL
*
* @{
*/

#include "optiga/pal/pal_gpio.h"
#include "optiga/pal/pal_ifx_i2c_config.h"
#include <gpiod.h>
#include <unistd.h>
#include <string.h>

typedef struct pal_linux_gpio_gpiod
{
    const char *gpio_device;
    uint16_t gpio_device_offset;
} pal_linux_gpio_gpiod_t;

/**
 * GPIOWrite - Write value to GPIO using libgpiod v2.x API
 *
 * This function has been rewritten for libgpiod v2.x compatibility.
 * The old gpiod_ctxless_set_value() API was removed in v2.0.
 *
 * @param pin GPIO pin configuration
 * @param value Value to write (0 or 1)
 * @return 0 on success, -1 on failure
 */
static int GPIOWrite(pal_linux_gpio_gpiod_t *pin, int value) {
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *line_config;
    struct gpiod_request_config *req_config;
    struct gpiod_line_request *request;
    unsigned int offset;
    int ret = -1;

    // Type casting from uint16_t to unsigned int
    offset = (unsigned int)pin->gpio_device_offset;

    // 1. Open GPIO chip
    chip = gpiod_chip_open(pin->gpio_device);
    if (!chip) {
        write(STDERR_FILENO, "Failed to open GPIO chip\n", 25);
        return -1;
    }

    // 2. Create line settings
    settings = gpiod_line_settings_new();
    if (!settings) {
        write(STDERR_FILENO, "Failed to create line settings\n", 31);
        gpiod_chip_close(chip);
        return -1;
    }

    // 3. Set direction to OUTPUT and initial value
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings,
        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

    // 4. Create line configuration
    line_config = gpiod_line_config_new();
    if (!line_config) {
        write(STDERR_FILENO, "Failed to create line config\n", 29);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    // 5. Add line settings to line config
    ret = gpiod_line_config_add_line_settings(line_config, &offset, 1, settings);
    if (ret) {
        write(STDERR_FILENO, "Failed to add line settings\n", 28);
        gpiod_line_config_free(line_config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    // 6. Create request configuration
    req_config = gpiod_request_config_new();
    if (!req_config) {
        write(STDERR_FILENO, "Failed to create request config\n", 32);
        gpiod_line_config_free(line_config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    // 7. Set consumer name
    gpiod_request_config_set_consumer(req_config, "trustm");

    // 8. Request GPIO lines
    request = gpiod_chip_request_lines(chip, req_config, line_config);
    if (!request) {
        write(STDERR_FILENO, "Failed to request GPIO line\n", 28);
        gpiod_request_config_free(req_config);
        gpiod_line_config_free(line_config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return -1;
    }

    // 9. Release all resources
    gpiod_line_request_release(request);
    gpiod_request_config_free(req_config);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return 0;
}

pal_status_t pal_gpio_init(const pal_gpio_t *p_gpio_context)
{
    if (p_gpio_context != NULL && p_gpio_context->p_gpio_hw != NULL)
    {
        pal_linux_gpio_gpiod_t *gpio = (pal_linux_gpio_gpiod_t *)p_gpio_context->p_gpio_hw;
        GPIOWrite(gpio, 1);
    }
    return PAL_STATUS_SUCCESS;
}

pal_status_t pal_gpio_deinit(const pal_gpio_t *p_gpio_context)
{
    return PAL_STATUS_SUCCESS;
}

void pal_gpio_set_high(const pal_gpio_t *p_gpio_context)
{
    if (p_gpio_context != NULL && p_gpio_context->p_gpio_hw != NULL)
    {
        pal_linux_gpio_gpiod_t *gpio = (pal_linux_gpio_gpiod_t *)p_gpio_context->p_gpio_hw;
        GPIOWrite(gpio, 1);
    }
}

void pal_gpio_set_low(const pal_gpio_t *p_gpio_context)
{
    if (p_gpio_context != NULL && p_gpio_context->p_gpio_hw != NULL)
    {
        pal_linux_gpio_gpiod_t *gpio = (pal_linux_gpio_gpiod_t *)p_gpio_context->p_gpio_hw;
        GPIOWrite(gpio, 0);
    }
}

/**
 * @}
 */
PATCH_EOF

echo "  Patch applied successfully!"
echo ""

# Verify patch
echo "[4/4] Verifying patch..."
if grep -q "gpiod_chip_request_lines" "$PAL_GPIO_FILE"; then
    echo "  VERIFIED: File now uses libgpiod v2.x API"
else
    echo "  ERROR: Patch verification failed!"
    exit 1
fi

echo ""
echo "============================================"
echo "Patch Complete!"
echo "============================================"
echo ""
echo "You can now build with:"
echo "  ./build.sh"
echo "  sudo make install"
echo "  sudo ldconfig"
echo ""
echo "To restore original file:"
echo "  cp ${PAL_GPIO_FILE}.original $PAL_GPIO_FILE"
echo ""
