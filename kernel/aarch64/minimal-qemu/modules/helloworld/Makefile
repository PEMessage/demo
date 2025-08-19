obj-m := helloworld.o

# for host it would likely be `/lib/modules/$(shell uname -r)/build`
KDIR := ../../linux/
PWD := $(shell pwd)

ARCH ?= arm64  # or your target architecture
CROSS_COMPILE ?= aarch64-linux-gnu-  # adjust to your toolchain
MODULE_BUILD_DIR ?= build

all:
	# we don't need `O=build`, `-C $(KDIR)` directly set to KBUILD DIR should be enough
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(PWD)/$(MODULE_BUILD_DIR) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(PWD)/$(MODULE_BUILD_DIR) clean
