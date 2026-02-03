
# ====================================================
# Toolchain configuration
# ====================================================
# This -r will cause makefile rm all intermedia file
# MAKEFLAGS += -r
ARCH        := arm
CROSS_COMPILE := arm-none-eabi-
CC          := $(CROSS_COMPILE)gcc -c
LD          := $(CROSS_COMPILE)gcc
OBJCOPY     := $(CROSS_COMPILE)objcopy
OBJDUMP     := $(CROSS_COMPILE)objdump
SIZE        := $(CROSS_COMPILE)size

DEBUG_INFO_CFLAGS := -g -g3
OPTIMIZE_CFLAGS := -O0
ASM_CFLAGS := -mcpu=cortex-m3 -mthumb

# LDFLAGS := -r
# Also See:
# 	https://www.tockos.org/blog/2016/dynamic-loading/
# Also See(--unresolved-symbols=ignore-in-object-files): https://github.com/bogdanm/udynlink
# Also See(-nostdlib): https://cs107e.github.io/guides/gcc/
# 	-nostdlib:  implies the individual options -nodefaultlibs and -nostartfiles
# 	-nodefaultlibs: libgcc.a disable(Eg: __divdf3, __adddf3...)
# 	-nostartfiles: start.s and cstart.c
LDFLAGS := -Wl,--unresolved-symbols=ignore-in-object-files -nostartfiles


CFLAGS := $(DEBUG_INFO_CFLAGS) \
		  $(OPTIMIZE_CFLAGS) \
		  $(ASM_CFLAGS)

PIC_CFLAGS := -fPIC $(CFLAGS)

# ====================================================
# Directory configuration
# ====================================================
OBJS_DIR := obj
ASMS_DIR := asm

# ====================================================
# Object files (in obj/ directory)
# ====================================================
SRCS := $(wildcard *.c)
OBJS := $(addprefix $(OBJS_DIR)/, $(SRCS:.c=.o))
ASMS := $(addprefix $(ASM_DIR)/, $(SRCS:.c=.asm))


all: $(ASMS_DIR)/main.asm $(ASMS_DIR)/main_pic.asm

all: main.elf
main.elf: $(OBJS_DIR)/main.o
	$(LD) $(LDFLAGS) -o $@ $<

$(OBJS_DIR)/%.o: %.c
	mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(OBJS_DIR)/%_pic.o: %.c
	mkdir -p $(OBJS_DIR)
	$(CC) $(PIC_CFLAGS) -o $@ $<


$(ASMS_DIR)/%.asm: $(OBJS_DIR)/%.o
	mkdir -p $(ASMS_DIR)
	$(OBJDUMP) -S $< > $@

.PHONY: clean
clean:
	rm -rf asm obj
	rm -rf *.elf

# do not mark any target as intermedia, so make will not delete anything
.SECONDARY:
