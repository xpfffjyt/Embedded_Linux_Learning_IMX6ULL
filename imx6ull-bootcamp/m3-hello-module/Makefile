# M3 - build the kernel module against the exact kernel that runs on the board.
#
#   make KDIR=$HOME/imx6ull/kernel/linux-imx
#   make KDIR=... clean
#
# The kernel tree must have been built at least once (so that Module.symvers,
# include/generated/autoconf.h and the prepared headers exist). Otherwise you
# get a confusing "No such file or directory: .../autoconf.h".
#
# IMPORTANT: kernel + module must be built with the SAME toolchain and the SAME
# kernel tree, or insmod fails with "Invalid module format" (vermagic mismatch).

KDIR          ?= $(HOME)/imx6ull/kernel/linux-imx
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
obj-m         := hello_module.o

MAKE_ARGS := -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: all clean info

all:
	$(MAKE) $(MAKE_ARGS) modules

clean:
	$(MAKE) $(MAKE_ARGS) clean

# Print what vermagic the module expects, to compare with the running kernel:
#   on the board: uname -r  and  cat /proc/version
info: hello_module.ko
	$(CROSS_COMPILE)modinfo hello_module.ko | grep -E 'vermagic|parm|license'
