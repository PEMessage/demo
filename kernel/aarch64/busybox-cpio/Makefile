
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-


SRC_DIR := busybox-src
# relateive to SRC
BUILD_DIR := build

$(SRC_DIR):
	git clone https://github.com/mirror/busybox.git --depth 1 --branch 1_36_1 $(SRC_DIR)


EXTRA_DEFCONFIG :=
EXTRA_DEFCONFIG += --enable CONFIG_STATIC
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG
EXTRA_DEFCONFIG += --enable CONFIG_FEATURE_VI_REGEX_SEARCH


.PHONY: defconfig
defconfig: $(SRC_DIR)/$(BUILD_DIR)/.config
$(SRC_DIR)/$(BUILD_DIR)/.config:
	mkdir -p $(SRC_DIR)/$(BUILD_DIR)
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) defconfig
	./config --file $(SRC_DIR)/$(BUILD_DIR)/.config \
		$(EXTRA_DEFCONFIG)
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) oldconfig

.PHONY: build
build:
	make -C busybox-src ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) -j8


# See: https://stackoverflow.com/questions/4219255/how-do-you-get-the-list-of-targets-in-a-makefile
.PHONY: list
list:
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'
