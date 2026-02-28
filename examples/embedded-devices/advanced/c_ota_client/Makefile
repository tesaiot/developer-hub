# TESAIoT OTA Client - Makefile
#
# Build the OTA HTTPS client for Linux/MacOS testing.
# For embedded targets, adapt the compiler and libraries.

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lcurl

# Source files
SRCS = main.c ota_client.c
OBJS = $(SRCS:.c=.o)
TARGET = ota_client

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies
main.o: main.c ota_client.h ota_config.h
ota_client.o: ota_client.c ota_client.h ota_config.h

clean:
	rm -f $(OBJS) $(TARGET)

# Test run
test: $(TARGET)
	./$(TARGET) --help

# Run with sample device
run: $(TARGET)
	./$(TARGET) --device-id "95ad6ed3-c9a7-43e3-96ba-871f25b5cfe9" \
		--server "admin.tesaiot.com" \
		--version "1.0.0" \
		--platform "PSE84" \
		--job-only

.PHONY: all clean test run
