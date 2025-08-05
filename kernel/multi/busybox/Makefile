# Top-level Makefile: Multi-architecture build orchestration
# Uses only LOCAL_* variables as required
# Also See: https://aegis.sourceforge.net/auug97.pdf
# 			Recursive Make Considered Harmful

# Default: Build for all architectures
all: all_arm all_arm64 all_x86 all_x86_64

include clear_vars.mk
LOCAL_ARCH := arm
LOCAL_CROSS_COMPILE := arm-linux-gnueabi-
include main.mk

include clear_vars.mk
LOCAL_ARCH := arm64
LOCAL_CROSS_COMPILE := aarch64-linux-gnu-
include main.mk

include clear_vars.mk
LOCAL_ARCH := x86_64
LOCAL_CROSS_COMPILE := x86_64-linux-gnu-
include main.mk

include clear_vars.mk
LOCAL_ARCH := x86
LOCAL_CROSS_COMPILE := i686-linux-gnu-
include main.mk

# --------------------------
# Helper Targets
# --------------------------
.PHONY: clean
clean: clean_arm clean_arm64

# See: https://stackoverflow.com/questions/4219255/how-do-you-get-the-list-of-targets-in-a-makefile
.PHONY: list
list:
	# -p (Print Database)
	# -R (Disable Built-in Rules/Variables)
	# -q (Question Mode) Instead of running commands, make -q checks whether targets are up to date
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'
