# Toolchain configuration
MAKEFLAGS += -r
ARCH        := arm
CROSS_COMPILE := arm-none-eabi-
CC          := $(CROSS_COMPILE)gcc
LD          := $(CROSS_COMPILE)ld
OBJCOPY     := $(CROSS_COMPILE)objcopy
OBJDUMP     := $(CROSS_COMPILE)objdump
SIZE        := $(CROSS_COMPILE)size
ifeq ($M,)
  M := hello_world
endif
	
$(info ==================================)
$(info build module "$M")
$(info ==================================)

MODULE_NAME := $(M)
OUT := out/$(MODULE_NAME)

# Compiler flags
CFLAGS  := -mcpu=cortex-m3 -mthumb -Wall -Os -g -g3


# ====================================================
# Helper function
# ====================================================
define file_exists
$(strip
  $(if $(wildcard $(strip $1)),
    true,
    false
  )
)
endef




# ====================================================
# Step 1, Basic startup file
# ====================================================
CFLAGS  += -DCMSDK_CM3
CFLAGS  += -Ilib/CMSIS/Core/Include 
LDFLAGS := -T lib/CMSDK_CM3/Source/gcc_arm.ld --entry=Reset_Handler
SRCS    := lib/CMSDK_CM3/Source/startup_CMSDK_CM3.c lib/CMSDK_CM3/Source/system_CMSDK_CM3.c  # Add your source files here

# ====================================================
# Step 2, USART Driver / clib(nosys) / and retarget printf 
# ====================================================
# Thanks to: https://www.reddit.com/r/embedded/comments/o2kuqt/linking_freertos_for_gcc/
# 	FreeRTOS needs a memcpy and memset implementation.
# 	Either you need to get them from the standard library or you need to implement them yourself.
# 	To use the standard library, you can use --specs=nano.spec or --specs=nosys.spec in LDFLAGS (and remove -nostdlib).
# Also See:
# 	Retarget using nosys.specs
# 	https://community.arm.com/support-forums/f/compilers-and-libraries-forum/44223/what-are-the-compile-option-differences-between---specs-nosys-specs-and---specs-rdimon-specs-in-arm-none-eabi-gcc
# Must Read:
# 	https://metebalci.com/blog/demystifying-arm-gnu-toolchain-specs-nano-and-nosys/
# 	nano.specs 与其他所有规格文件的一个区别是，
# 	nano.specs 除了更改链接选项外还会修改编译选项（而其他规格文件仅修改链接选项）。
# 	另一个不同点是 nano.specs 会改变标准 C 库，而其他规格文件则与标准 C 库和系统的交互有关
# 	并且你还可以选择其中一个其他规格文件（例如 nosys）或不使用（那么你的系统中必须以某种方式提供 syscalls）。
# 	CFLAGS += -specs=nano.specs
# 	CFLAGS  += -lnosys -lm
LDFLAGS += -specs=nosys.specs -lnosys
SRCS    += lib/CMSIS/Driver/USART_V2M-MPS2.c lib/CMSDK_CM3/Source/stdout_USART.c
CFLAGS  += -Ilib/CMSDK_CM3/Include/
CFLAGS  += -Ilib/CMSIS/Driver  -Ilib/CMSIS/Driver/Include

# ====================================================
# Step 3, User code
# ====================================================
ifeq ($(call file_exists,src/$(MODULE_NAME)/*.mk),false)
  CFLAGS  += -Isrc/$(MODULE_NAME)
  SRCS    += $(wildcard src/$(MODULE_NAME)/*.c)
endif

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
