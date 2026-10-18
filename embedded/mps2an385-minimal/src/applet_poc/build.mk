# This is not standalone makefile

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR := $(patsubst %/,%,$(dir $(THIS_MAKEFILE)))

APPLETS_DIR := $(THIS_DIR)/applets
APPLETS_OUT := $(OUT)/applets


$(info DEVTO)

$(shell mkdir -p $(APPLETS_OUT))

APPLETS_SRCS := $(wildcard $(APPLETS_DIR)/src/*.c)
APPLETS_GEN_SRCS := $(patsubst $(APPLETS_DIR)/src/%.c,$(APPLETS_OUT)/%.gen.c,$(APPLETS_SRCS))

SRCS += $(THIS_DIR)/main.c \
	   $(APPLETS_GEN_SRCS)
CFLAGS += -I ${THIS_DIR}/applets/applet_lib

$(info  $(CFLAGS))

$(APPLETS_OUT)/%.elf: $(APPLETS_DIR)/src/%.c
	$(APPLETS_DIR)/build_applets.sh $@ $<

$(APPLETS_OUT)/%.bin: $(APPLETS_OUT)/%.elf
	$(OBJCOPY) -O binary $< $@

$(APPLETS_OUT)/%.gen.c: $(APPLETS_OUT)/%.bin
	# it seem like this command might not return 0, `|| true` will force it return 0
	xxd -i $< > $@ || true
