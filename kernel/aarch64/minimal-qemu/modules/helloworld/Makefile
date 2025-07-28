obj-m := helloworld.o
KDIR := ../../linux-5.18.2/
PWD := $(shell pwd)

ARCH ?= arm64  # or your target architecture
CROSS_COMPILE ?= aarch64-linux-gnu-  # adjust to your toolchain

all:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
