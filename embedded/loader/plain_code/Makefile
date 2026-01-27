
# ====================================================
# Toolchain configuration
# ====================================================
MAKEFLAGS += -r
ARCH        := arm
CROSS_COMPILE := arm-none-eabi-
CC          := $(CROSS_COMPILE)gcc -c
LD          := $(CROSS_COMPILE)ld
OBJCOPY     := $(CROSS_COMPILE)objcopy
OBJDUMP     := $(CROSS_COMPILE)objdump
SIZE        := $(CROSS_COMPILE)size

DEBUG_INFO_CFLAGS := -g -g3
OPTIMIZE_CFLAGS := -Os
ASM_CFLAGS := -mcpu=cortex-m3 -mthumb

# LDFLAGS := -r


CFLAGS := $(DEBUG_INFO_CFLAGS) \
		  $(OPTIMIZE_CFLAGS) \
		  $(ASM_CFLAGS)

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


# main.elf: main.o main.asm
# 	$(LD) $(LDFLAGS) -o $@ $<
all: $(OBJS_DIR)/main.o $(ASMS_DIR)/main.asm

$(OBJS_DIR)/%.o: %.c
	mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -o $@ $<


$(ASMS_DIR)/%.asm: $(OBJS_DIR)/%.o
	mkdir -p $(ASMS_DIR)
	$(OBJDUMP) -S $< > $@
