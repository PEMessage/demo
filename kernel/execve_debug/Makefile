
.PHONY: all
all: init linux/build/arch/x86_64/boot/bzImage 

linux/build/.config: ./config-fragment
	cd linux && \
		mkdir -p build && \
		make O=build defconfig && \
		./scripts/kconfig/merge_config.sh -O build build/.config ../config-fragment


linux/build/arch/x86_64/boot/bzImage: linux/build/.config
	cd linux && \
		mkdir -p build && \
		bear -- make O=build -j4

init: init.c
	musl-gcc -static -entry main init.c -o $@

init.cpio: init
	echo $^ | cpio -o -H newc > $@ 

qemu: all
	qemu-system-x86_64 -kernel linux/build/arch/x86_64/boot/bzImage -initrd init.cpio -s -S

