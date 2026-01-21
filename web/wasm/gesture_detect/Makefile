BUILD_DIR := build

EXPORTS = malloc free realloc calloc strdup printf fprintf
CFLAGS := 
define export_flags
$(foreach func,$(1),-Wl,--export=$(func))
endef
WASM_FLAGS := $(call export_flags,$(EXPORTS)) \
		 $(CFLAGS) \
		 -MJ main.o.json

COMPDB := compile_commands.json

$(BUILD_DIR)/main.wasm: main.c src/tpv3_core.c
	mkdir -p $(BUILD_DIR)
	zig cc -target wasm32-wasi $(WASM_FLAGS) -o $@ $^
	# fix compdb not work
	echo '[' > $(COMPDB)
	find . -name '*.o.json' -exec cat {} \; >> $(COMPDB)
	echo ']' >> $(COMPDB)
	rm main.o.json

.PHONY: clean
clean:
	rm -rf build

