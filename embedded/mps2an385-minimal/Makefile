# Toolchain configuration
MAKEFLAGS += -r
ARCH        := arm
CROSS_COMPILE := arm-none-eabi-
CC          := $(CROSS_COMPILE)gcc
LD          := $(CROSS_COMPILE)ld
OBJCOPY     := $(CROSS_COMPILE)objcopy
OBJDUMP     := $(CROSS_COMPILE)objdump
SIZE        := $(CROSS_COMPILE)size
OUT := out
MODULE_NAME := firmware

# Compiler flags
CFLAGS  := -mcpu=cortex-m3 -mthumb -Wall -Os -g -g3
# CFLAGS  += -lnosys -lm
CFLAGS  += -DCMSDK_CM3
CFLAGS  += -Iinclude -ICMSIS/Core/Include 
# Step2: for USART
CFLAGS  += -ICMSIS/Driver  -ICMSIS/Driver/Include

# Linker flags
LDFLAGS := -T gcc_arm.ld --entry=Reset_Handler

# Thanks to: https://www.reddit.com/r/embedded/comments/o2kuqt/linking_freertos_for_gcc/
# FreeRTOS needs a memcpy and memset implementation.
# Either you need to get them from the standard library or you need to implement them yourself.
# To use the standard library, you can use --specs=nano.spec or --specs=nosys.spec in LDFLAGS (and remove -nostdlib).
LDFLAGS += -specs=nosys.specs

# Project files
SRCS    := startup_CMSDK_CM3.c system_CMSDK_CM3.c  # Add your source files here
SRCS    += main.c
# Step2: for USART
SRCS    += CMSIS/Driver/USART_V2M-MPS2.c stdout_USART.c

OBJS    := $(addprefix $(OUT)/, $(SRCS:.c=.o))
TARGET  := $(OUT)/$(MODULE_NAME).elf
BIN     := $(OUT)/$(MODULE_NAME).bin
LSS     := $(OUT)/$(MODULE_NAME).lss

# Default target
all: $(TARGET) $(BIN) $(LSS)

# Link the final executable
$(TARGET): $(OBJS)
	mkdir -p $$(dirname $@)
	# $(LD) $(LDFLAGS) -o $@ $^
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Generate binary file for flashing
$(BIN): $(TARGET)
	mkdir -p $$(dirname $@)
	$(OBJCOPY) -O binary $< $@

$(LSS): $(TARGET)
	mkdir -p $$(dirname $@)
	$(OBJDUMP) -S $< > $@

# Compile C source files
$(OUT)/%.o: %.c
	mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/%.S.o: %.S
	mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET) $(BIN)


.PHONY: all clean size
