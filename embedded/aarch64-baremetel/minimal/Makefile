
# Base on: https://surma.dev/postits/arm64/

TOOLCHAIN_PREFIX := aarch64-linux-gnu-
CC := $(TOOLCHAIN_PREFIX)gcc
AS := $(TOOLCHAIN_PREFIX)as
LD := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

.PHONY: all

all: qemu

main.o: main.s
	$(AS) -o $@ $<
main.elf: main.o
	$(LD) -T link.ld -o $@ $<
main.bin: main.elf
	$(OBJCOPY) -O binary $< $@

.PHONY: qemu
qemu: main.bin
	qemu-system-aarch64 \
	-M virt -cpu cortex-a72 \
	-m 128M -nographic \
	-device loader,file=$<,addr=0x40100000 \
	-device loader,addr=0x40100000,cpu-num=0 	
	# The first -device directive loads a file into memory at the specific address.
	# The second -device directive sets the CPUs starting address

# verify:
# 	`Ctrl-a c` enter monitor and use the `info registers`
# 	X13 contains 0x1337
