MODULE_BUILD_DIR ?= build
PWD ?= $(shell pwd)
MO ?= $(PWD)/$(MODULE_BUILD_DIR) # in submake, MO will not be reassign

SRC_FILES := $(wildcard $(PWD)/*.c)
OBJ_FILES := $(notdir $(SRC_FILES:.c=.o))
$(info [MY OBJM]: $(OBJ_FILES))
obj-m := $(OBJ_FILES)

KDIR := ../../linux/

ARCH ?= arm64  # or your target architecture
CROSS_COMPILE ?= aarch64-linux-gnu-  # adjust to your toolchain

all:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(MO) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=build M=$(PWD) MO=$(MO) clean
