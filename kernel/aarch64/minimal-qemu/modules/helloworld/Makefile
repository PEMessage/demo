obj-m := helloworld.o
KDIR := ../../linux/
PWD := $(shell pwd)

ARCH ?= arm64  # or your target architecture
CROSS_COMPILE ?= aarch64-linux-gnu-  # adjust to your toolchain
BUILD_DIR ?= build

all:
	make -C $(KDIR)/build ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) MO=$(BUILD_DIR) modules

clean:
	make -C $(KDIR)/build ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) MO=$(BUILD_DIR) clean
