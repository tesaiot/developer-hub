################################################################################
# \file common.mk
# \version 1.0
#
# \brief
# Settings shared across all projects.
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

MTB_TYPE=PROJECT

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make library-manager' from command line), which will also update
# Eclipse IDE launch configurations.
TARGET=APP_KIT_PSE84_AI

# Name of toolchain to use. Options include:
#
# GCC_ARM 	-- GCC is available as part of ModusToolbox Setup program
# ARM     	-- ARM Compiler (must be installed separately)
# IAR     	-- IAR Compiler (must be installed separately)
# LLVM_ARM	-- LLVM Embedded Toolchain (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate
# launch configurations for your IDE.
CONFIG=Debug

############################# Display module ###################################
# Option to choose the display module to realize the graphics application.
# Select one of them as per the required use-case.
# WF101JTYAHMNB0_DISP	- 10.1 inch 1024*600 pixel TFT DSI LCD and it's touch
#                         driver.
# WS7P0DSI_RPI_DISP     - Waveshare 7 inch Raspberry-Pi DSI LCD (C) 1024*600 pixel
#
# W4P3INCH_DISP	- Waveshare 4.3 inch Raspberry-Pi DSI LCD 800*480 pixel
# Ex:
#   CONFIG_DISPLAY = WF101JTYAHMNB0_DISP
#   or
#   CONFIG_DISPLAY = WS7P0DSI_RPI_DISP
#   or
#   CONFIG_DISPLAY = W4P3INCH_DISP
CONFIG_DISPLAY = W4P3INCH_DISP

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
COMPONENTS+=GFXSS

# NOTE: Check the JSON file for the command parameters
COMBINE_SIGN_JSON?=configs/boot_with_extended_boot.json

include ../common_app.mk
