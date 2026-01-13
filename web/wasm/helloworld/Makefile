
CC=clang
CFLAGS = -Wall -Wextra -pedantic -ggdb -I. -O2

WASM_FLAGS = -fno-builtin --target=wasm32 --no-standard-libraries
# -Wl,--allow-undefined: wasm-ld: error: /tmp/main-6f1eb4.o: undefined symbol: console_log
WASM_LDFLAGS = -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined

main.wasm: main.c
	$(CC) $(CFLAGS) $(WASM_FLAGS) $< $(WASM_LDFLAGS) -o $@

clean:
	rm -f main.wasm
