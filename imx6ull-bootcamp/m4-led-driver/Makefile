# M4 - LED driver + device tree overlay for the board's kernel (4.19.35-imx6).
#
#   make KDIR=$HOME/imx6ull/kernel/linux-imx          # build led_drv.ko
#   make KDIR=... user                               # build led_test (user space)
#   make KDIR=... dtbo                               # compile the .dtbo overlay
#   make KDIR=... all                                # ko + dtbo + test tool
#   make KDIR=... clean
#
# Order matters on the board:
#   1. free GPIO1_IO04  (comment imx-fire-led.dtbo in /boot/uEnv.txt, reboot)
#   2. install the .dtbo + add a dtoverlay= line in /boot/uEnv.txt, reboot
#   3. insmod led_drv.ko      -> /dev/ebf_led appears
# A .ko alone can never create the device: probe() is driven by the DT node.

KDIR          ?= $(HOME)/imx6ull/kernel/linux-imx
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
DTS_DIR       ?= $(KDIR)/arch/arm/boot/dts

obj-m  := led_drv.o
DTBO   := ebf-led.dtbo
DTS    := overlay-ebf-led.dts

MAKE_ARGS := -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: all ko user dtbo clean install-hint

all: ko dtbo user

ko:
	$(MAKE) $(MAKE_ARGS) modules

# dtc with -@ is mandatory: U-Boot's "fdt apply" needs __symbols__.
dtbo: $(DTBO)

$(DTBO): $(DTS)
	dtc -@ -I dts -O dtb -o $@ $< -i $(DTS_DIR)

user: led_test

led_test: led_test.c
	$(CROSS_COMPILE)gcc -Wall -Wextra -O2 -g -o $@ $^

clean:
	$(MAKE) $(MAKE_ARGS) clean
	rm -f led_test $(DTBO)

install-hint:
	@echo "on the board:"
	@echo "  sudo cp ebf-led.dtbo /usr/lib/linux-image-4.19.35-imx6/overlays/"
	@echo "  echo 'dtoverlay=/usr/lib/linux-image-4.19.35-imx6/overlays/ebf-led.dtbo' | sudo tee -a /boot/uEnv.txt"
	@echo "  sudo reboot"
	@echo "  ls /proc/device-tree | grep ebf        # node must appear"
	@echo "  sudo insmod led_drv.ko"
	@echo "  sudo sh -c 'echo 1 > /dev/ebf_led'     # red LED on"
