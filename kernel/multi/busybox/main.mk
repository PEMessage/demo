

# my_arch := $(LOCAL_ARCH)
# my_cross_compile := $(LOCAL_CROSS_COMPILE)

my_src_dir := busybox-src
my_build_dir := busybox-build/$(LOCAL_ARCH)
  my_src_to_build_dir := ../$(my_build_dir)
  my_config_file := $(my_build_dir)/.config

my_make := $(MAKE) -C $(my_src_dir) ARCH=$(LOCAL_ARCH) CROSS_COMPILE=$(LOCAL_CROSS_COMPILE) O=$(my_src_to_build_dir)

# --------------------------
# Phony Targets
# --------------------------
.PHONY: all_$(LOCAL_ARCH) download_$(LOCAL_ARCH) defconfig_$(LOCAL_ARCH) \
        build_$(LOCAL_ARCH) rootfs_$(LOCAL_ARCH) clean_$(LOCAL_ARCH)

# Default target for this architecture
all_$(LOCAL_ARCH): rootfs_$(LOCAL_ARCH)

# --------------------------
# Download Target
# --------------------------
download_$(LOCAL_ARCH): $(my_src_dir)

# Use private variable to capture current LOCAL_SRC_DIR
$(my_src_dir): private_my_src_dir := $(my_src_dir)
$(my_src_dir):
	git clone https://github.com/mirror/busybox.git \
		--depth 1 --branch 1_36_1 $(private_my_src_dir)

# --------------------------
# Configuration Target
# --------------------------
defconfig_$(LOCAL_ARCH): $(my_config_file)


$(my_config_file): Makefile $(my_src_dir)

# Must using private, VARIABLE inside target are lazy eval(even if using :=)
$(my_config_file): private_my_build_dir := $(my_build_dir)
$(my_config_file): private_my_config_file := $(my_config_file)
$(my_config_file): private_my_make := $(my_make)

$(my_config_file):
	# Create build directory
	mkdir -p $(private_my_build_dir)
	
	$(private_my_make) defconfig
	
	# Apply custom configurations
	./config --file $(private_my_config_file) \
		--enable CONFIG_STATIC \
		--enable CONFIG_DEBUG \
		--enable CONFIG_FEATURE_VI_REGEX_SEARCH
	
	$(private_my_make) oldconfig

build_$(LOCAL_ARCH): defconfig_$(LOCAL_ARCH)
build_$(LOCAL_ARCH): private_my_make := $(my_make)
build_$(LOCAL_ARCH):
	$(private_my_make) -j8
	$(private_my_make) install


rootfs_$(LOCAL_ARCH): build_$(LOCAL_ARCH)
rootfs_$(LOCAL_ARCH): private_my_build_dir := $(my_build_dir)
rootfs_$(LOCAL_ARCH):
	fakeroot -s $(private_my_build_dir)/rootfs.fakeroot bash \
		build-rootfs.sh $(private_my_build_dir)/rootfs $(private_my_build_dir)/_install
