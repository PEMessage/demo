BUILD_DIR := build

EXPORTS = malloc free realloc calloc strdup printf fprintf
define export_flags
$(foreach func,$(1),-Wl,--export=$(func))
endef
FLAGS := $(call export_flags,$(EXPORTS))

$(BUILD_DIR)/main.wasm: main.c
	mkdir -p $(BUILD_DIR)
	zig cc -target wasm32-wasi $(FLAGS) -o $@ $^


.PHONY: clean
clean:
	rm -rf build

