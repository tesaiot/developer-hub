# TESAIoT WSS Live Streaming - C Client Makefile

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lmosquitto -lssl -lcrypto

# Binary name
TARGET = wss_mqtt_client

# Source files
SRCS = wss_mqtt_client.c
OBJS = $(SRCS:.c=.o)

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew
    MOSQUITTO_PREFIX ?= $(shell brew --prefix mosquitto 2>/dev/null)
    OPENSSL_PREFIX ?= $(shell brew --prefix openssl 2>/dev/null)
    ifneq ($(MOSQUITTO_PREFIX),)
        CFLAGS += -I$(MOSQUITTO_PREFIX)/include
        LDFLAGS := -L$(MOSQUITTO_PREFIX)/lib $(LDFLAGS)
    endif
    ifneq ($(OPENSSL_PREFIX),)
        CFLAGS += -I$(OPENSSL_PREFIX)/include
        LDFLAGS := -L$(OPENSSL_PREFIX)/lib $(LDFLAGS)
    endif
endif

# Debug build
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2
endif

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo ""
	@echo "Build complete: ./$(TARGET)"
	@echo ""
	@echo "Usage:"
	@echo "  1. cp .env.example .env"
	@echo "  2. Edit .env with your MQTT_API_TOKEN"
	@echo "  3. ./$(TARGET)"

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Install
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Show dependencies
deps:
	@echo "Dependencies for $(UNAME_S):"
ifeq ($(UNAME_S),Linux)
	@echo "  sudo apt install -y build-essential libmosquitto-dev libssl-dev"
endif
ifeq ($(UNAME_S),Darwin)
	@echo "  brew install mosquitto openssl"
endif
	@echo ""

# Help
help:
	@echo "TESAIoT WSS Live Streaming - C Client"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the application (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  deps     - Show dependency installation commands"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1  - Build with debug symbols"
	@echo ""
	@echo "Example:"
	@echo "  make"
	@echo "  make DEBUG=1"
	@echo "  make clean && make"

.PHONY: all clean install uninstall deps help
