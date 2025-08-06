# Top-level Makefile: Multi-architecture build orchestration
# Uses only LOCAL_* variables as required

# Default: Build for all architectures
all: all_arm all_arm64

# --------------------------
# ARM Architecture
# --------------------------
include clear_vars.mk
LOCAL_ARCH := arm
LOCAL_CROSS_COMPILE := arm-linux-gnueabi-
include main.mk

# --------------------------
# ARM64 Architecture
# --------------------------
include clear_vars.mk
LOCAL_ARCH := arm64
LOCAL_CROSS_COMPILE := aarch64-linux-gnu-
include main.mk

# --------------------------
# Helper Targets
# --------------------------
.PHONY: clean
clean: clean_arm clean_arm64
