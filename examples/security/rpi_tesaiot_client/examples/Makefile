# Makefile for TESAIoT Portable Deployment Examples
# Builds CSR client, PU client, and v3.0.0 crypto examples
# Date: 2026-02-08
#
# Usage:
#   make                  - Build all examples
#   make csr              - Build CSR client only
#   make pu               - Build PU client only
#   make crypto-examples  - Build v3.0.0 crypto examples
#   make clean            - Remove built binaries

CC = gcc

# Directories (relative to examples/)
INCLUDE_DIR = ../include/tesaiot
LIB_DIR = ../lib

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += -fstack-protector-strong
CFLAGS += -D_FORTIFY_SOURCE=2
CFLAGS += -fPIE

# Linker flags
LDFLAGS = -L$(LIB_DIR)
LDFLAGS += -pie
LDFLAGS += -Wl,-z,relro,-z,now
LDFLAGS += -Wl,-z,noexecstack

# Libraries
LDLIBS = -ltesaiot -ltrustm -lpaho-mqtt3cs -lssl -lcrypto -lpthread -lgpiod

# Device credentials (required by libtesaiot.a)
CREDS = device_config.c

# Targets
CSR_CLIENT = tesaiot_csr_client
PU_CLIENT = tesaiot_pu_client

# v3.0.0 crypto example executables
EXAMPLE_01 = example_01_random_and_store
EXAMPLE_02 = example_02_aes_encrypt_sensor
EXAMPLE_03 = example_03_hmac_mqtt_integrity
EXAMPLE_04 = example_04_ecdh_device_pairing
EXAMPLE_05 = example_05_sign_verify_telemetry
EXAMPLE_06 = example_06_health_monitor

CRYPTO_EXAMPLES = $(EXAMPLE_01) $(EXAMPLE_02) $(EXAMPLE_03) \
                  $(EXAMPLE_04) $(EXAMPLE_05) $(EXAMPLE_06)

.PHONY: all csr pu crypto-examples clean help

all: csr pu crypto-examples
	@echo ""
	@echo "=== All examples built ==="
	@echo "  $(CSR_CLIENT)    (CSR Workflow Client)"
	@echo "  $(PU_CLIENT)     (Protected Update Client)"
	@echo "  $(EXAMPLE_01)"
	@echo "  $(EXAMPLE_02)"
	@echo "  $(EXAMPLE_03)"
	@echo "  $(EXAMPLE_04)"
	@echo "  $(EXAMPLE_05)"
	@echo "  $(EXAMPLE_06)"
	@echo ""
	@echo "Run with:  LD_LIBRARY_PATH=$(LIB_DIR):$$LD_LIBRARY_PATH ./<binary>"

# --- Client binaries (same source as bin/tesaiot_csr_client and bin/tesaiot_pu_client) ---

csr: $(CSR_CLIENT)

$(CSR_CLIENT): csr_client_example.c $(CREDS)
	@echo "Building $(CSR_CLIENT)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@echo "  -> $(CSR_CLIENT) (same as ../bin/tesaiot_csr_client)"

pu: $(PU_CLIENT)

$(PU_CLIENT): pu_client_example.c $(CREDS)
	@echo "Building $(PU_CLIENT)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@echo "  -> $(PU_CLIENT) (same as ../bin/tesaiot_pu_client)"

# --- v3.0.0 Crypto Examples ---

crypto-examples: $(CRYPTO_EXAMPLES)

$(EXAMPLE_01): example_01_random_and_store.c $(CREDS)
	@echo "Building $(EXAMPLE_01)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(EXAMPLE_02): example_02_aes_encrypt_sensor.c $(CREDS)
	@echo "Building $(EXAMPLE_02)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(EXAMPLE_03): example_03_hmac_mqtt_integrity.c $(CREDS)
	@echo "Building $(EXAMPLE_03)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(EXAMPLE_04): example_04_ecdh_device_pairing.c $(CREDS)
	@echo "Building $(EXAMPLE_04)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(EXAMPLE_05): example_05_sign_verify_telemetry.c $(CREDS)
	@echo "Building $(EXAMPLE_05)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(EXAMPLE_06): example_06_health_monitor.c $(CREDS)
	@echo "Building $(EXAMPLE_06)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(CSR_CLIENT) $(PU_CLIENT) $(CRYPTO_EXAMPLES)

help:
	@echo "TESAIoT Portable Deployment - Example Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make                  Build all (CSR + PU + crypto examples)"
	@echo "  make csr              Build CSR client only"
	@echo "  make pu               Build PU client only"
	@echo "  make crypto-examples  Build v3.0.0 crypto examples"
	@echo "  make clean            Remove built binaries"
	@echo "  make help             Show this help"
	@echo ""
	@echo "Source structure:"
	@echo "  csr_client_example.c  -> tesaiot_csr_client  (same as ../bin/)"
	@echo "  pu_client_example.c   -> tesaiot_pu_client   (same as ../bin/)"
	@echo "  example_0*.c          -> v3.0.0 crypto utility demos"
	@echo "  device_config.c       -> Device credentials (edit for your device)"
	@echo ""
	@echo "IP Protection:"
	@echo "  Library source is in ../lib/libtesaiot.a (static archive)"
	@echo "  Public headers in ../include/tesaiot/"
	@echo "  Only the public API is exposed - internal implementation is protected"
