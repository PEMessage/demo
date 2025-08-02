
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-


SRC_DIR := busybox-src
# relateive to SRC
BUILD_DIR := build

BUSYBOX_CONFIG := $(SRC_DIR)/$(BUILD_DIR)/.config

EXTRA_DEFCONFIG :=
EXTRA_DEFCONFIG += --enable CONFIG_STATIC
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG
EXTRA_DEFCONFIG += --enable CONFIG_FEATURE_VI_REGEX_SEARCH
# disable TC, for some reason don't pass build on ubuntu24.04 
# but workfine on 22.04
EXTRA_DEFCONFIG += --disable CONFIG_TC

.PHONY: all
all: rootfs.cpio.gz


.PHONY: download
download: $(SRC_DIR)
$(SRC_DIR):
	git clone https://github.com/mirror/busybox.git --depth 1 --branch 1_36_1 $(SRC_DIR)


.PHONY: defconfig
defconfig: $(BUSYBOX_CONFIG)
$(BUSYBOX_CONFIG): Makefile $(SRC_DIR)
	mkdir -p $(SRC_DIR)/$(BUILD_DIR)
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) defconfig
	./config --file $(SRC_DIR)/$(BUILD_DIR)/.config \
		$(EXTRA_DEFCONFIG)
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) oldconfig


.PHONY: build
build: defconfig
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) -j8
	# create build/_install as rootfs
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) install


rootfs.cpio.gz: build
	fakeroot -s rootfs.fakeroot bash build-rootfs.sh rootfs $(SRC_DIR)/$(BUILD_DIR)/_install rootfs_overlay/*

.PHONY: list
repack-nodep:
	fakeroot -s rootfs.fakeroot bash build-rootfs.sh rootfs rootfs_overlay/*

# See: https://stackoverflow.com/questions/4219255/how-do-you-get-the-list-of-targets-in-a-makefile
.PHONY: list
list:
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'
