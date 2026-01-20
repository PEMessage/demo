BUILD_DIR := build


.PHONY: all
all: $(BUILD_DIR)/main.wasm
	python3 -m http.server

# allow hot reload
.PHONY: dev
dev:
	npx vite

$(BUILD_DIR)/main.wasm: main.c
	mkdir -p $(BUILD_DIR)
	zig cc -target wasm32-wasi -o $@ $^

