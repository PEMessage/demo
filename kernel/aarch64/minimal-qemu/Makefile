# @@META CONFIG START
# Thanks to https://zhuanlan.zhihu.com/p/510289859 comment
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

KDIR := linux
# KDIR = linux-5.18.2
# KSRC_URL = mirrors.tuna.tsinghua.edu.cn/kernel/v5.x/$(KDIR).tar.xz
# relative path to -C $(KDIR)
BUILD_DIR := build

# also relative to KDIR/BUILD_DIR
ROOTFS_DIR := rootfs

KIMG := $(KDIR)/$(BUILD_DIR)/arch/arm64/boot/Image
KDOTCONFIG := $(KDIR)/$(BUILD_DIR)/.config
# Come from https://www.kernel.org/doc/html/v6.9/dev-tools/gdb-kernel-debugging.html
EXTRA_DEFCONFIG := --enable CONFIG_DEBUG_INFO
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
EXTRA_DEFCONFIG += --enable CONFIG_GDB_SCRIPTS
# See: https://stackoverflow.com/a/33203642 , but may duplicate
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG_KERNEL

# KASLR disable
# or `qemu-system ... -append nokaslr` disable it from cmdline
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


# See: https://blog.csdn.net/weixin_52027058/article/details/132019367
# Usual function not founded, when compile module  _printk、kfree、kmalloc、memset
EXTRA_DEFCONFIG += --disable CONFIG_TRIM_UNUSED_KSYMS
# @@META CONFIG END

PWD := $(shell pwd)

.PHONY: default
default: modules

.PHONY: all
all: default headers

.PHONY: download
download: $(KDIR)
$(KDIR):
	@echo "||====== Download $(KDIR)"
	git clone https://github.com/torvalds/linux.git --branch v5.18-rc2 --depth 1


# 默认配置并开启调试功能
.PHONY: config
config: $(KDOTCONFIG)
# $(KDOTCONFIG) should not depend on download but $(KDIR), which will cause config rebuilt always
$(KDOTCONFIG): $(KDIR) Makefile
	@echo "||====== Config $(KDOTCONFIG)"
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) defconfig
		$(KDIR)/scripts/config --file $(KDIR)/$(BUILD_DIR)/.config \
		$(EXTRA_DEFCONFIG)
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) oldconfig
	# dump var
	cat Makefile | ./dumppart.awk -v begin_pattern="@@META CONFIG START" -v end_pattern="@@META CONFIG END" | tee $(KDIR)/$(BUILD_DIR)/dumpvar.mk
	# touch $(KDOTCONFIG)

.PHONY: menuconfig
menuconfig:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) menuconfig

# add KIMG to phony, to always run it, linux make will run fast if not change
.PHONY: build $(KIMG)
build: $(KIMG)
$(KIMG): config
	@echo "||====== build $(KIMG)"
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) Image -j8
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) scripts_gdb
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) modules_prepare
	$(KDIR)/scripts/clang-tools/gen_compile_commands.py -d $(KDIR)/$(BUILD_DIR)

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
.PHONY: modules
modules: build
	# we must built modules for generate Modules.symvar
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) modules -j8
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) modules_install INSTALL_MOD_PATH=$(ROOTFS_DIR)
	$(KDIR)/scripts/clang-tools/gen_compile_commands.py -d $(KDIR)/$(BUILD_DIR)

.PHONY: headers
headers:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUILD_DIR) headers_install INSTALL_HDR_PATH=$(ROOTFS_DIR)/usr/include/linux



QEMU_MACHINE := -M virt -cpu cortex-a53 -smp 2 -m 4096
.PHONY: qemu
qemu:
	qemu-system-aarch64 $(QEMU_MACHINE) -kernel $(KIMG) -nographic --initrd ../busybox-cpio/rootfs.cpio.gz

.PHONY: qemu-fast-ko
qemu-fast-ko:
	make -C ../busybox-cpio/ repack-nodep
	qemu-system-aarch64 $(QEMU_MACHINE) -kernel $(KIMG) -nographic --initrd ../busybox-cpio/rootfs.cpio.gz \
		-monitor telnet:localhost:11234,server,nowait

# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"
# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"  -netdev user,id=mynet -device virtio-net-device,netdev=mynet
.PHONY: debug
debug:
	@echo gdb-multiarch -ex \'add-auto-load-safe-path $(KDIR)\' -ex \'target remote localhost:1234\' $(KDIR)/$(BUILD_DIR)/vmlinux
	qemu-system-aarch64 $(QEMU_MACHINE) -kernel $(KIMG) -nographic --initrd ../busybox-cpio/rootfs.cpio.gz \
		-monitor telnet:localhost:11234,server,nowait \
		-nographic -S -s \


.PHONY: qemu-dumpdts
qemu-dumpdts:
	mkdir -p dts
	qemu-system-aarch64 $(QEMU_MACHINE) -machine dumpdtb=dts/vm.dtb
	dtc -I dtb -O dts -o dts/vm.dts dts/vm.dtb
	rm dts/vm.dtb -rf
