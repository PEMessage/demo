obj-m := helloworld.o
KDIR := ../../linux/
PWD := $(shell pwd)

ARCH ?= arm64  # or your target architecture
CROSS_COMPILE ?= aarch64-linux-gnu-  # adjust to your toolchain
MODULE_BUILD_DIR ?= build

all:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(PWD)/$(MODULE_BUILD_DIR) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(PWD)/$(MODULE_BUILD_DIR) clean
