
# Base on: https://surma.dev/postits/arm64/

TOOLCHAIN_PREFIX := aarch64-linux-gnu-
GCC := $(TOOLCHAIN_PREFIX)gcc
AS := $(TOOLCHAIN_PREFIX)as
LD := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

.PHONY: all

all: qemu

startup.o: startup.s
	$(AS) -o $@ $<
# main.o: main.c
# 	$(GCC) -c -o $@ $<
main.elf: startup.o
	$(LD) -T link.ld -o $@ $<
main.bin: main.elf
	$(OBJCOPY) -O binary $< $@

.PHONY: qemu
qemu: main.bin
	# using `CTRL-a x` to exit
	# The first -device directive loads a file into memory at the specific address.
	# The second -device directive sets the CPUs starting address
	qemu-system-aarch64 \
	-M virt -cpu cortex-a72 \
	-m 128M -nographic \
	-device loader,file=$<,addr=0x40100000 \
	-device loader,addr=0x40100000,cpu-num=0 	

.PHONY: clean
clean:
	rm -f *.elf *.o *.bin

# verify:
# 	`Ctrl-a c` enter monitor and use the `info registers`
# 	X13 contains 0x1337
