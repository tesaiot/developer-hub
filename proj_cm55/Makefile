################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M55 core project
#
################################################################################
# \copyright
# (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG.  SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

include ../common.mk

################################################################################
# Basic Configuration
################################################################################

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=proj_cm55

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
# TESAIoT Dev Kit Master — union of sensor stack + WiFi stack
# FREERTOS RTOS_AWARE       : required by all episodes (LVGL timer task + app tasks)
# BMM350_I3C                : BMM350 magnetometer via I3C (interactive sensor episodes)
# LWIP MBEDTLS              : WiFi middleware stack (hmi-menu wifi episodes)
COMPONENTS+=FREERTOS RTOS_AWARE BMM350_I3C LWIP MBEDTLS

ifeq ($(CONFIG_DISPLAY), WF101JTYAHMNB0_DISP)
DEFINES += MTB_DISPLAY_EK79007AD3 MTB_CTP_ILI2511
# Ignore unused 7-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE += $(SEARCH_touch-ctp-gt911)
# Ignore unused 4.3-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-4-3-lcd)
CY_IGNORE += $(SEARCH_touch-ctp-ft5406)
else ifeq ($(CONFIG_DISPLAY), WS7P0DSI_RPI_DISP)
DEFINES += MTB_DISPLAY_WS7P0DSI_RPI MTB_CTP_GT911
# Ignore unused 10.1-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-tft-ek79007ad3)
CY_IGNORE += $(SEARCH_touch-ctp-ili2511)
# Ignore unused 4.3-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-4-3-lcd)
CY_IGNORE += $(SEARCH_touch-ctp-ft5406)
else ifeq ($(CONFIG_DISPLAY), W4P3INCH_DISP)
DEFINES += MTB_DISPLAY_W4P3INCH_RPI MTB_CTP_FT5406
# Ignore unused 10.1-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-tft-ek79007ad3)
CY_IGNORE += $(SEARCH_touch-ctp-ili2511)
# Ignore unused 7-inch display and touch driver files from compilation
CY_IGNORE += $(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE += $(SEARCH_touch-ctp-gt911)
endif

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

CORE=CM55
CORE_NAME=CM55_0

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES+=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES+=
# TESAIoT Dev Kit Master — include paths
# Core:        core_cfg, platform, lvgl_*
# Shared:      app_assets (app_logo and other branding shared by every episode)
# App slot:    apps/  (episode code lives here — sensor drivers, UI, audio all together)
#
# MTB make auto-discovers .c sources in the project tree, so dropping an
# episode into ./apps/ (including sub-folders) just works. For header
# discovery we recursively include every sub-directory under ./apps/ so
# episodes can organize their code however they want.
#
# app_assets/ holds assets (logo, theme icons) that every episode would
# otherwise duplicate — kept in the master to eliminate ~1.6 MB × N
# duplication of app_logo.c across episodes.
INCLUDES += ./core_cfg ./platform ./lvgl_cfg ./lvgl_port ./lvgl_override ./app_assets ./app_core ./app_sensor
INCLUDES += $(shell find ./apps -type d 2>/dev/null)
INCLUDES += $(shell find ./app_sensor -type d 2>/dev/null)

# Add additional defines to the build process (without a leading -D).
# CYBSP_WIFI_CAPABLE required by wifi-core-freertos-lwip-mbedtls middleware
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF _BAREMETAL=0 CYBSP_WIFI_CAPABLE

# Check which kit is being used
ifeq (APP_KIT_PSE84_AI, $(TARGET))
DEFINES+=USE_KIT_PSE84_AI
endif

# Ignore unused LVGL middleware files from compilation
CY_IGNORE += $(SEARCH_lvgl)/src/others/vg_lite_tvg $(SEARCH_lvgl)/src/libs/thorvg $(SEARCH_lvgl)/tests
CY_IGNORE += $(SEARCH_lvgl)/draw/nxp $(SEARCH_lvgl)/draw/renesas $(SEARCH_lvgl)/examples
CY_IGNORE += $(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite.c $(SEARCH_lvgl)/src/draw/vg_lite/lv_vg_lite_utils.c
CY_IGNORE += $(SEARCH_lvgl)/src/draw/vg_lite/lv_draw_vg_lite_img.c $(SEARCH_lvgl)/src/draw/sw/blend/helium
CY_IGNORE += $(SEARCH_lvgl)/src/draw/sw/blend/neon $(SEARCH_lvgl)/src/core/lv_refr.c

# Episodes catalog (reference copies of all 14 episodes) — NOT compiled.
# The active episode lives in ./apps/ and is compiled normally.
CY_IGNORE += ../../episodes

# TLS socket layer not required for the training episodes in this course
CY_IGNORE += $(SEARCH_secure-sockets)




# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT+=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS+=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS+=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS+=

# Additional / custom linker flags.
LDFLAGS+=

# Additional / custom libraries to link in to the application.
LDLIBS+=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
# BMM350 prebuild patch is intentionally OMITTED from the master template:
#   episodes that use the BMM350 compass (e.g. ep04-bmm350-compass,
#   ep07-sensorhub-final) vendor a pre-patched bmm350.c in their apps/
#   folder so no bash prebuild step is needed. This keeps the master
#   template portable to Windows and CI.
PREBUILD=
# Custom post-build commands to run.
POSTBUILD=


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION>
# field begins with $$ASSET_REPO$$, then the repo is deposited in the path
# specified by the CY_GETLIBS_SHARED_PATH variable. The default location is one
# directory level above the current app directory. This is used with
# CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk






