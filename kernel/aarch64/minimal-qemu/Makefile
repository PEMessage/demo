
# Thanks to https://zhuanlan.zhihu.com/p/510289859 comment
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

KDIR = linux
# KDIR = linux-5.18.2
# KSRC_URL = mirrors.tuna.tsinghua.edu.cn/kernel/v5.x/$(KDIR).tar.xz


# relative path to -C $(KDIR)
BUILD_DIR=build

KIMG = $(KDIR)/$(BUILD_DIR)/arch/arm64/boot/Image

all: $(KDIR) $(KDIR)/.config 
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) Image -j8
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) scripts_gdb

$(KIMG):
	make all

$(KDIR):
	@echo "Download $(KDIR)"
	# @wget -c $(KSRC_URL) && tar -xf $(KDIR).tar.xz
	git clone https://github.com/torvalds/linux.git --branch v5.18-rc2 --depth 1

modules:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) modules

# $(KDIR)/.config:
# 	make defconfig

$(KDIR)/.config: defconfig
	

# Come from https://www.kernel.org/doc/html/v6.9/dev-tools/gdb-kernel-debugging.html
EXTRA_DEFCONFIG := --enable CONFIG_DEBUG_INFO 
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
EXTRA_DEFCONFIG += --enable CONFIG_GDB_SCRIPTS
# KASLR disable
EXTRA_DEFCONFIG += --disable CONFIG_RANDOMIZE_BASE

# some config generate when run oldconfig
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_NONE
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_SPLIT
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_BTF
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_REDUCED
# zlib compressed, GCC 5.0+ or Clang 5.0+
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG_INFO_COMPRESSED
# UEFI debug feature
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_EFI

# disbale watch(secure feature same ad CONFIG_RANDOMIZE_BASE)
# See: 【简单几步搭建Linux Kernel调试环境】 【精准空降到 00:41】
# https://www.bilibili.com/video/BV1UTVfzfETp/?t=41
# deepseek-v3: 在调试时，内核可能因断点、单步执行或人为暂停而无法及时喂狗，导致看门狗误判为系统故障并重启。
EXTRA_DEFCONFIG += --disable CONFIG_WATCHDOG


# wslkernel-recompile.sh
EXTRA_DEFCONFIG += --enable CONFIG_ANDROID
EXTRA_DEFCONFIG += --enable CONFIG_ANDROID_BINDER_IPC
EXTRA_DEFCONFIG += --set-str CONFIG_ANDROID_BINDER_DEVICES "binder,hwbinder,vndbinder"
EXTRA_DEFCONFIG += --enable CONFIG_ANDROID_BINDERFS
EXTRA_DEFCONFIG += --enable CONFIG_STAGING
EXTRA_DEFCONFIG += --enable CONFIG_ASHMEM
EXTRA_DEFCONFIG += --disable ANDROID_BINDER_IPC_SELFTEST



# 默认配置并开启调试功能
defconfig: Makefile $(KDIR)
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) defconfig
		$(KDIR)/scripts/config --file $(KDIR)/$(BUILD_DIR)/.config \
		$(EXTRA_DEFCONFIG)
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) oldconfig

menuconfig:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) menuconfig

# Update: QEMU should pass to kernel, not need for us to generate, but nice as a note
# 
# DTB = mydtb/my.dto
# DTS = $(DTB:.dto=.dts)
#
# $(DTS):
# 	mkdir -p $(dir $@)
# 	qemu-system-aarch64 -M virt,dumpdtb=$(DTB) -nographic
# 	dtc -I dtb -O dts $(DTB) > $@
# dtb: $(DTS)
# 	dtc -I dts -O dtb $< -o $(DTB)
# $(DTB):
# 	make dtb

qemu: $(KIMG) $(DTB)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel $(KIMG) -nographic --initrd ../busybox-cpio/rootfs.cpio.gz
	# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel $(KIMG) -nographic

# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"
# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"  -netdev user,id=mynet -device virtio-net-device,netdev=mynet
debug: $(KIMG) $(DTB)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel $(KIMG) -nographic -S -s

PHONY += dtb all defconfig qemu debug menuconfig modules
.PHONY: $(PHONY)

