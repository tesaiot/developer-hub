################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M33 core project
#
################################################################################
# \copyright
# Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
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
APPNAME=proj_cm33_ns

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
COMPONENTS+=FREERTOS LWIP MBEDTLS WCM SECURE_SOCKETS OPTIGA_CYHAL

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# OPTIGA Trust M configuration
OPTIGAFLAGS=OPTIGA_LIB_EXTERNAL='"optiga_lib_config_mtb.h"'

CORE=CM33
CORE_NAME=CM33_0

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.

# OPTIGA Trust M library configuration
OPTIGA_LIB_PATH=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0

# OPTIGA Trust M source files
SOURCES+=$(OPTIGA_LIB_PATH)/src/crypt/optiga_crypt.c \
         $(OPTIGA_LIB_PATH)/src/util/optiga_util.c \
         $(OPTIGA_LIB_PATH)/src/cmd/optiga_cmd.c \
         $(OPTIGA_LIB_PATH)/src/common/optiga_lib_common.c \
         $(OPTIGA_LIB_PATH)/src/common/optiga_lib_logger.c \
         $(OPTIGA_LIB_PATH)/src/comms/optiga_comms_ifx_i2c.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_config.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_data_link_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_physical_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_presentation_layer.c \
         $(OPTIGA_LIB_PATH)/src/comms/ifx_i2c/ifx_i2c_transport_layer.c \
         $(OPTIGA_LIB_PATH)/extras/pal/pal_crypt_mbedtls.c

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
# TESAIoT License Library headers
INCLUDES+=../tesaiot/include

# Add mbedtls library folder for entropy_poll.h (needed by aws-iot-device-sdk-port)
INCLUDES+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/ifx-mbedtls/release-v3.6.400/library

# Add PSA Crypto SE source file (needed for OPTIGA Trust M PSA driver registration)
SOURCES+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/ifx-mbedtls/release-v3.6.400/library/psa_crypto_se.c

# TESAIoT License Data Binding (customer compiles with own UID + License Key)
# NOTE: tesaiot_license.c is IN library (compiled from official project)
# NOTE: tesaiot_license_data.c provides extern variable values at link time
SOURCES+=../tesaiot/src/tesaiot_license_data.c

# MQTT Device Configuration Data Binding (customer compiles with own DEVICE_ID + API_KEY)
# NOTE: mqtt_device_config_data.c provides extern variable values at link time
SOURCES+=mqtt_device_config_data.c

# OPTIGA Trust M include directories
INCLUDES+=$(OPTIGA_LIB_PATH)/include \
          $(OPTIGA_LIB_PATH)/include/common \
          $(OPTIGA_LIB_PATH)/include/pal \
          $(OPTIGA_LIB_PATH)/include/cmd \
          $(OPTIGA_LIB_PATH)/include/comms \
          $(OPTIGA_LIB_PATH)/include/ifx_i2c

# Ignore optiga-trust-m's internal mbedtls to avoid conflicts
CY_IGNORE+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0/external/mbedtls
# Ignore optiga-trust-m's PAL implementations (not for PSoC - we use COMPONENT_OPTIGA_CYHAL)
CY_IGNORE+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/optiga-trust-m/release-v5.3.0/pal

# Custom configuration of mbedtls library.
MBEDTLSFLAGS = MBEDTLS_USER_CONFIG_FILE='"mbedtls_user_config.h"'

# Add additional defines to the build process (without a leading -D).
DEFINES+=$(MBEDTLSFLAGS) $(OPTIGAFLAGS) CYBSP_WIFI_CAPABLE CY_RETARGET_IO_CONVERT_LF_TO_CRLF CY_CRYPTO_HAL_DISABLE CY_RTOS_AWARE

# MQTT configuration (default 5 slots - kept low for RAM constraints)

# Debug level: 0=NONE, 1=ERROR, 2=WARNING, 3=INFO, 4=VERBOSE
# NOTE: Level 2-3 hides user-facing output. Use VERBOSE (4) for now.
# TODO: Separate user output from debug output before using lower levels
DEFINES+=TESAIOT_DEBUG_LEVEL=4

# Default configuration of mbedtls library.
DEFINES+=MBEDTLS_CONFIG_FILE='"mbedtls/mbedtls_config.h"'
# PSA configuration of mbedtls library.
DEFINES+=MBEDTLS_PSA_CRYPTO_CONFIG_FILE='"configs/ifx_psa_crypto_config.h"'

# FIX: cy_mqtt_publish() deadlock - event thread holds mutex indefinitely
# Default CY_MQTT_RECEIVE_DATA_TIMEOUT_MS=0 means MQTT_ProcessLoop() waits forever
# Setting to 100ms allows periodic mutex release for publish operations
DEFINES+=CY_MQTT_RECEIVE_DATA_TIMEOUT_MS=100

ifeq ($(TOOLCHAIN),LLVM_ARM)
DEFINES+=CY_MQTT_EVENT_THREAD_STACK_SIZE=10*1024
endif

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
# TESAIoT License Library path
LDFLAGS+=-L../tesaiot/lib

# Additional / custom libraries to link in to the application.
# TESAIoT Library (includes all tesaiot_* functions + license check)
LDLIBS+=-ltesaiot

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
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
# directory level above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the 
# directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
