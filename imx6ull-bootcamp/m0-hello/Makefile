# M0 - build hello for the i.MX6ULL (ARM32 hard-float) board.
#
#   make                              # uses arm-linux-gnueabihf-
#   make CROSS_COMPILE=/opt/gcc-4.9.4/bin/arm-linux-gnueabihf-
#   make TARGET_IP=192.168.1.50 deploy
#   make static                       # if the rootfs lacks the dynamic loader

CROSS_COMPILE ?= arm-linux-gnueabihf-
CC            := $(CROSS_COMPILE)gcc
READELF       := $(CROSS_COMPILE)readelf

TARGET  ?= hello
SRC     := hello.c
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=
TARGET_IP ?= 192.168.1.50

.PHONY: all clean deploy check static

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Static build: no /lib/ld-linux-armhf.so.3 needed on the board.
# Great for a first smoke test, useless for learning about shared libraries.
static: $(SRC)
	$(CC) $(CFLAGS) -static -o $(TARGET)-static $^

# Sanity check on the build host.
# NOTE: 'ldd' does NOT work on a foreign-architecture binary -> use readelf.
check: $(TARGET)
	@file $(TARGET)
	@echo "--- NEEDED / interpreter ---"
	@$(READELF) -d $(TARGET) 2>/dev/null | grep -E 'NEEDED|Shared library' || echo "(statically linked)"
	@$(READELF) -l $(TARGET) 2>/dev/null | grep interpreter || echo "(no interpreter)"

deploy: $(TARGET)
	scp $(TARGET) root@$(TARGET_IP):/root/

clean:
	rm -f $(TARGET) $(TARGET)-static
