
# Thanks to https://zhuanlan.zhihu.com/p/510289859 comment
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

KDIR = linux-5.18.2
KSRC_URL = mirrors.tuna.tsinghua.edu.cn/kernel/v5.x/$(KDIR).tar.xz

KIMG = $(KDIR)/arch/arm64/boot/Image
DTB = mydtb/my.dto
DTS = $(DTB:.dto=.dts)

all: $(KDIR) $(KDIR)/.config
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) Image -j8
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) scripts_gdb

$(KIMG):
	make all

$(KDIR):
	@echo "Download $(KDIR)"
	@wget -c $(KSRC_URL) && tar -xf $(KDIR).tar.xz

$(KDIR)/.config:
	make defconfig

# Come from https://www.kernel.org/doc/html/v6.9/dev-tools/gdb-kernel-debugging.html
EXTRA_DEFCONFIG := --enable CONFIG_DEBUG_INFO 
EXTRA_DEFCONFIG += --enable CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
EXTRA_DEFCONFIG += --enable CONFIG_GDB_SCRIPTS
EXTRA_DEFCONFIG += --enable CONFIG_RANDOMIZE_BASE

# some config generate when run oldconfig
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_NONE
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_SPLIT
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_BTF
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_REDUCED
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_INFO_COMPRESSED
EXTRA_DEFCONFIG += --disable CONFIG_DEBUG_EFI

# 默认配置并开启调试功能
defconfig: Makefile
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) defconfig
		$(KDIR)/scripts/config --file $(KDIR)/.config \
		$(EXTRA_DEFCONFIG)
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) oldconfig

menuconfig:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) menuconfig

$(DTS):
	mkdir -p $(dir $@)
	qemu-system-aarch64 -M virt,dumpdtb=$(DTB) -nographic
	dtc -I dtb -O dts $(DTB) > $@

dtb: $(DTS)
	dtc -I dts -O dtb $< -o $(DTB)

$(DTB):
	make dtb

qemu: $(KIMG) $(DTB)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel $(KIMG) -nographic

# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"
# qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel linux-5.18.2/arch/arm64/boot/Image -dtb mydtb/my.dto -nographic -hda bullseye.img -append "root=/dev/vda"  -netdev user,id=mynet -device virtio-net-device,netdev=mynet
debug: $(KIMG) $(DTB)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 2048 -kernel $(KIMG) -dtb $(DTB) -nographic -S -s

PHONY += dtb all defconfig qemu debug menuconfig
.PHONY: $(PHONY)

